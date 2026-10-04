// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "StrategyPawn.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "StrategyHUD.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "StrategyUnit.h"
#include "NavigationSystem.h"
#include "Engine/OverlapResult.h"
#include "InputAction.h"
#include "StrategyTouchControls.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "MassDemo.h"

AStrategyPlayerController::AStrategyPlayerController()
{
	// mouse cursor should always be shown
	bShowMouseCursor = true;
}

void AStrategyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UStrategyTouchControls>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

			// set the PC pointer on the mobile controls widget
			MobileControlsWidget->SetPlayerController(this);

		} else {

			UE_LOG(LogMassDemo, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

}

void AStrategyPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalPlayerController())
	{
		// 1. Smooth Camera Zoom Interpolation
		if (bEnableSmoothZoom && ControlledCameraPawn)
		{
			CameraZoom = FMath::FInterpTo(CameraZoom, TargetCameraZoom, DeltaTime, ZoomInterpSpeed);
			ControlledCameraPawn->SetZoomModifier(CameraZoom);
		}

		// 2. Smooth Camera Rotation Interpolation
		if (bEnableCameraRotation && ControlledCameraPawn)
		{
			CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, RotationInterpSpeed);
			CurrentPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, RotationInterpSpeed);
			ControlledCameraPawn->SetCameraRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
		}

		// 3. Smooth Camera Focus on Selection / Target
		if (bIsFocusing && ControlledCameraPawn)
		{
			FVector CurrentLoc = ControlledCameraPawn->GetActorLocation();
			FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetFocusLocation, DeltaTime, FocusInterpSpeed);
			ControlledCameraPawn->SetActorLocation(NewLoc);

			if (FVector::DistSquared2D(NewLoc, TargetFocusLocation) < 25.0f)
			{
				bIsFocusing = false;
			}
		}

		// 4. Edge Scrolling
		ProcessEdgeScrolling(DeltaTime);

		// 5. Constrain Camera Position within Map Bounds
		ClampCameraToBounds();
	}
}

void AStrategyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only set up input on local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			// choose the context based on the input mode
			UInputMappingContext* ChosenContext = nullptr;

			if (ShouldUseTouchControls())
			{
				ChosenContext = TouchMappingContext;
			}
			else
			{
				ChosenContext = MouseMappingContext;
			}

			Subsystem->AddMappingContext(ChosenContext, 0);
		}

		// bind the input mappings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// Camera Pan & Zoom
			EnhancedInputComponent->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::MoveCamera);
			EnhancedInputComponent->BindAction(ZoomCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ZoomCamera);
			EnhancedInputComponent->BindAction(ResetCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ResetCamera);

			// Camera Rotation & Focus & Speed
			EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::RotateCamera);
			EnhancedInputComponent->BindAction(FocusCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::FocusCamera);
			EnhancedInputComponent->BindAction(FastMoveModifierAction, ETriggerEvent::Started, this, &AStrategyPlayerController::FastMoveStarted);
			EnhancedInputComponent->BindAction(FastMoveModifierAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::FastMoveCompleted);
			EnhancedInputComponent->BindAction(FastMoveModifierAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::FastMoveCompleted);

			// Middle Mouse Button Drag Panning
			EnhancedInputComponent->BindAction(MMBPanAction, ETriggerEvent::Started, this, &AStrategyPlayerController::MMBPanStarted);
			EnhancedInputComponent->BindAction(MMBPanAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::MMBPanTriggered);
			EnhancedInputComponent->BindAction(MMBPanAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::MMBPanCompleted);
			EnhancedInputComponent->BindAction(MMBPanAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::MMBPanCompleted);

			// Mouse Interaction
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::SelectHoldStarted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::SelectHoldTriggered);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectHoldCompleted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::SelectHoldCompleted);

			EnhancedInputComponent->BindAction(SelectClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClick);

			EnhancedInputComponent->BindAction(SelectClickAdditiveAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClickAdditive);

			EnhancedInputComponent->BindAction(SelectAllDoubleClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectAllDoubleClick);

			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::InteractHoldStarted);
			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::InteractHoldTriggered);

			EnhancedInputComponent->BindAction(InteractClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::InteractClick);

			// Touch Interaction
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::TouchPrimaryHoldStarted);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchPrimaryHoldTriggered);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchPrimaryHoldCompleted);

			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchSecondaryTriggered);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchSecondaryCompleted);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::TouchSecondaryCompleted);

		}
	}
}

void AStrategyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// ensure we have the right pawn type
	ControlledCameraPawn = Cast<AStrategyPawn>(InPawn);
	check(ControlledCameraPawn);

	// set the zoom level from the pawn's camera
	DefaultZoom = CameraZoom = TargetCameraZoom = ControlledCameraPawn->GetCamera()->OrthoWidth;

	// set rotation from pawn camera
	if (ControlledCameraPawn->GetCamera())
	{
		const FRotator CamRot = ControlledCameraPawn->GetCamera()->GetRelativeRotation();
		DefaultYaw = CurrentYaw = TargetYaw = CamRot.Yaw;
		DefaultPitch = CurrentPitch = TargetPitch = CamRot.Pitch;
	}

	// cast the HUD pointer
	StrategyHUD = Cast<AStrategyHUD>(GetHUD());

	// if we have a touch controls widget, sync the camera zoom
	if (MobileControlsWidget)
	{
		MobileControlsWidget->BP_SetZoomPercentage(GetDefaultZoomPercentage());
	}
}

void AStrategyPlayerController::DragSelectUnits(const TArray<AStrategyUnit*>& Units)
{
	// do we have units in the list?
	if (Units.Num() > 0)
	{
		// ensure any previous units are deselected
		DoDeselectAllUnitsCommand();

		// select each new unit
		for (AStrategyUnit* CurrentUnit : Units)
		{
			// add the unit to the selection list
			ControlledUnits.Add(CurrentUnit);

			// select the unit
			CurrentUnit->UnitSelected();
		}

	}
	else
	{

		// release any currently selected units since nothing is on the box
		if (ControlledUnits.Num() > 0)
		{
			DoDeselectAllUnitsCommand();
		}

	}
}

const TArray<AStrategyUnit*>& AStrategyPlayerController::GetSelectedUnits()
{
	return ControlledUnits;
}

float AStrategyPlayerController::GetDefaultZoomPercentage() const
{
	float ZoomPct = (DefaultZoom - MinZoomLevel) / (MaxZoomLevel - MinZoomLevel);
	return FMath::Clamp(ZoomPct, 0.0f, 1.0f);
}

bool AStrategyPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AStrategyPlayerController::MoveCamera(const FInputActionValue& Value)
{
	FVector2D InputVector = Value.Get<FVector2D>();

	if (ControlledCameraPawn)
	{
		bIsFocusing = false;

		const FRotator YawRot(0.0f, CurrentYaw, 0.0f);
		const FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

		const float Multiplier = bIsFastPanActive ? FastPanMultiplier : 1.0f;

		ControlledCameraPawn->AddMovementInput(ForwardDir, InputVector.Y * Multiplier);
		ControlledCameraPawn->AddMovementInput(RightDir, InputVector.X * Multiplier);
	}
}

void AStrategyPlayerController::ZoomCamera(const FInputActionValue& Value)
{
	DoCameraModifyZoomCommand(Value.Get<float>() * ZoomScaling);
}

void AStrategyPlayerController::ResetCamera(const FInputActionValue& Value)
{
	DoCameraResetAllCommand();
}

void AStrategyPlayerController::RotateCamera(const FInputActionValue& Value)
{
	const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
	const float YawDelta = Value.Get<float>() * KeyboardRotationSpeed * DeltaTime;
	DoCameraRotateYawCommand(YawDelta);
}

void AStrategyPlayerController::FocusCamera(const FInputActionValue& Value)
{
	DoCameraFocusOnSelectionCommand();
}

void AStrategyPlayerController::FastMoveStarted(const FInputActionValue& Value)
{
	bIsFastPanActive = true;
}

void AStrategyPlayerController::FastMoveCompleted(const FInputActionValue& Value)
{
	bIsFastPanActive = false;
}

void AStrategyPlayerController::MMBPanStarted(const FInputActionValue& Value)
{
	bIsMMBPanning = true;
	bIsFocusing = false;
	LastMMBMousePosition = GetMouseLocationForPlayer();
}

void AStrategyPlayerController::MMBPanTriggered(const FInputActionValue& Value)
{
	if (bIsMMBPanning && ControlledCameraPawn)
	{
		const FVector2D CurrentMouse = GetMouseLocationForPlayer();
		if (!CurrentMouse.IsNearlyZero() && !LastMMBMousePosition.IsNearlyZero())
		{
			const FVector2D MouseDelta = CurrentMouse - LastMMBMousePosition;
			const FRotator YawRot(0.0f, CurrentYaw, 0.0f);
			const FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
			const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

			// MMB pan moves camera in opposite direction of mouse drag
			const FVector WorldDelta = (RightDir * (-MouseDelta.X) + ForwardDir * MouseDelta.Y) * MMBPanSensitivity;
			ControlledCameraPawn->AddActorWorldOffset(WorldDelta);
			ClampCameraToBounds();
		}
		LastMMBMousePosition = CurrentMouse;
	}
}

void AStrategyPlayerController::MMBPanCompleted(const FInputActionValue& Value)
{
	bIsMMBPanning = false;
}

void AStrategyPlayerController::SelectHoldStarted(const FInputActionValue& Value)
{
	// save the box selection start position
	StartingBoxSelectionPosition = GetMouseLocationForPlayer();

}

void AStrategyPlayerController::SelectHoldTriggered(const FInputActionValue& Value)
{
	// get the current mouse position
	FVector2D SelectionPosition = GetMouseLocationForPlayer();

	// calculate the size of the selection box
	FVector2D SelectionSize = SelectionPosition - StartingBoxSelectionPosition;

	// update the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(StartingBoxSelectionPosition, SelectionSize, SelectionPosition, true);
	}	
}

void AStrategyPlayerController::SelectHoldCompleted(const FInputActionValue& Value)
{
	// reset the drag box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

void AStrategyPlayerController::SelectClick(const FInputActionValue& Value)
{
	// get the cursor location
	FVector CursorLocation;

	if (GetLocationUnderCursor(CursorLocation))
	{
		// select at the cursor
		DoSelectCommand(CursorLocation, false);
	}
}

void AStrategyPlayerController::SelectClickAdditive(const FInputActionValue& Value)
{
	// get the cursor location
	FVector CursorLocation;

	if (GetLocationUnderCursor(CursorLocation))
	{
		// additive select at the cursor
		DoSelectCommand(CursorLocation, true);
	}
}

void AStrategyPlayerController::SelectAllDoubleClick(const FInputActionValue& Value)
{
	DoSelectAllUnitsOnScreenCommand();
}

void AStrategyPlayerController::InteractHoldStarted(const FInputActionValue& Value)
{

	// save the starting interaction position
	StartingDragScrollPosition = GetMouseLocationForPlayer();
}

void AStrategyPlayerController::InteractHoldTriggered(const FInputActionValue& Value)
{
	// do a drag scroll 
	DoCameraDragScrollCommand(GetMouseLocationForPlayer());
}

void AStrategyPlayerController::InteractClick(const FInputActionValue& Value)
{
	// get the cursor location
	FVector CursorLocation;

	// do we have a valid interaction location under the cursor?
	if (GetLocationUnderCursor(CursorLocation))
	{
		// move the selected units to the target location
		DoMoveUnitsCommand(CursorLocation);
	}
}

void AStrategyPlayerController::TouchPrimaryHoldStarted(const FInputActionValue& Value)
{
	// save the camera drag screen coords
	StartingDragScrollPosition = Value.Get<FVector2D>();

	
}

void AStrategyPlayerController::TouchPrimaryHoldTriggered(const FInputActionInstance& Instance)
{
	FVector2D InputVector = Instance.GetValue().Get<FVector2D>();

	// update the box select start position
	StartingBoxSelectionPosition = InputVector;

	if (Instance.GetElapsedTime() > TouchDragScrollHoldTime)
	{
		DoCameraDragScrollCommand(InputVector);

		// save the game time
		LastTouchDragScrollTime = GetWorld()->GetTimeSeconds();
	}
}

void AStrategyPlayerController::TouchPrimaryHoldCompleted(const FInputActionValue& Value)
{
	// ensure we don't trigger a tap input right after we finish a drag scroll
	if (GetWorld()->GetTimeSeconds() - LastTouchDragScrollTime > 0.1f)
	{
		// get the touch location in world space
		FVector TouchLocation = ProjectTouchPointToWorldSpace();

		// try to do a select command
		if (!DoSelectCommand(TouchLocation, true))
		{
			// if nothing was selected, do a move units command instead
			DoMoveUnitsCommand(TouchLocation);
		}
	}
}

void AStrategyPlayerController::TouchSecondaryTriggered(const FInputActionValue& Value)
{
	// get the touch 2 screen coords
	FVector2D SelectionPosition = Value.Get<FVector2D>();

	// calculate the size of the selection box
	FVector2D SelectionSize = SelectionPosition - StartingBoxSelectionPosition;

	// update the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(StartingBoxSelectionPosition, SelectionSize, SelectionPosition, true);
	}
		
}

void AStrategyPlayerController::TouchSecondaryCompleted(const FInputActionValue& Value)
{
	if (StrategyHUD)
	{
		// hide the selection box
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

bool AStrategyPlayerController::DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection)
{
	// deselect any units unless this is an additive selection
	if (!bAdditiveSelection)
	{
		DoDeselectAllUnitsCommand();
	}

	// do an overlap test at the cursor location
	TArray<FOverlapResult> OutOverlaps;

	FCollisionShape CollisionSphere;
	CollisionSphere.SetSphere(SelectionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;

	if (GetWorld()->OverlapMultiByObjectType(OutOverlaps, SelectLocation, FQuat::Identity, ObjectParams, CollisionSphere, QueryParams))
	{
		// find the first unit we've overlapped
		for (const FOverlapResult& CurrentOverlap : OutOverlaps)
		{
			if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentOverlap.GetActor()))
			{
				// is this unit already selected?
				if (ControlledUnits.Contains(CurrentUnit))
				{
					// deselect the unit
					ControlledUnits.Remove(CurrentUnit);

					CurrentUnit->UnitDeselected();
				}
				else
				{
					// select the unit
					ControlledUnits.Add(CurrentUnit);

					CurrentUnit->UnitSelected();
				}

				// found a unit
				return true;
			}
		}
	}

	// didn't find a unit
	return false;
}

void AStrategyPlayerController::DoSelectAllUnitsOnScreenCommand()
{
	// get all units on the level
	TArray<AActor*> Units;

	UGameplayStatics::GetAllActorsOfClass(this, AStrategyUnit::StaticClass(), Units);

	// process each unit
	for (AActor* CurrentActor : Units)
	{
		if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentActor))
		{
			// is the unit is not already selected, and is on screen?
			if (!ControlledUnits.Contains(CurrentUnit) && CurrentUnit->WasRecentlyRendered(0.2f))
			{
				// select the unit
				ControlledUnits.Add(CurrentUnit);

				CurrentUnit->UnitSelected();
			}
		}
		
	}
}

void AStrategyPlayerController::DoDeselectAllUnitsCommand()
{
	// deselect each unit
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (IsValid(CurrentUnit))
		{
			CurrentUnit->UnitDeselected();
		}
	}

	// clear the selection list
	ControlledUnits.Empty();
}

void AStrategyPlayerController::DoToggleSelectAllUnitsCommand()
{
	// do we have units selected?
	if (ControlledUnits.Num() > 0)
	{
		// deselect all units
		DoDeselectAllUnitsCommand();
	}
	else
	{
		// select all units on screen
		DoSelectAllUnitsOnScreenCommand();
	}
}

void AStrategyPlayerController::DoCameraDragScrollCommand(const FVector2D& CurrentCursorPosition)
{
	// subtract the starting position from the cursor to find the on-screen movement delta
	FVector2D MoveDelta = StartingDragScrollPosition - CurrentCursorPosition;
	StartingDragScrollPosition = CurrentCursorPosition;

	const FRotator YawRot(0.0f, CurrentYaw, 0.0f);
	const FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	FVector RotatedDelta = (RightDir * MoveDelta.X - ForwardDir * MoveDelta.Y) * DragMultiplier;

	// apply the offset to the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->AddActorWorldOffset(RotatedDelta);
		ClampCameraToBounds();
	}
}

void AStrategyPlayerController::DoMoveUnitsCommand(const FVector& GoalLocation)
{
	if (ControlledUnits.Num() > 0)
	{
		// find the closest unit to the goal
		AStrategyUnit* ClosestUnit = GetClosestSelectedUnitToLocation(GoalLocation);

		// tell each unit to move to the location
		for (AStrategyUnit* CurrentUnit : ControlledUnits)
		{
			if (IsValid(CurrentUnit))
			{
				CurrentUnit->MoveToLocation(GoalLocation, CurrentUnit == ClosestUnit, ControlledUnits);
			}
		}

		// show positive cursor feedback
		BP_CursorFeedback(GoalLocation, true);

	}
	else
	{
		// no units selected, so just show negative cursor feedback
		BP_CursorFeedback(GoalLocation, false);
	}
}

void AStrategyPlayerController::DoCameraModifyZoomCommand(float ZoomDelta)
{
	// add the delta to target zoom
	TargetCameraZoom = FMath::Clamp(TargetCameraZoom + ZoomDelta, MinZoomLevel, MaxZoomLevel);

	if (!bEnableSmoothZoom)
	{
		CameraZoom = TargetCameraZoom;
		if (ControlledCameraPawn)
		{
			ControlledCameraPawn->SetZoomModifier(CameraZoom);
		}
	}
}

void AStrategyPlayerController::DoCameraResetZoomCommand()
{
	// reset to default zoom
	TargetCameraZoom = DefaultZoom;

	if (!bEnableSmoothZoom)
	{
		CameraZoom = TargetCameraZoom;
		if (ControlledCameraPawn)
		{
			ControlledCameraPawn->SetZoomModifier(CameraZoom);
		}
	}
}

void AStrategyPlayerController::DoCameraSetZoomPercentageCommand(float Percentage)
{
	// lerp between min and max zoom
	TargetCameraZoom = FMath::Lerp(MinZoomLevel, MaxZoomLevel, FMath::Clamp(Percentage, 0.0f, 1.0f));

	if (!bEnableSmoothZoom)
	{
		CameraZoom = TargetCameraZoom;
		if (ControlledCameraPawn)
		{
			ControlledCameraPawn->SetZoomModifier(CameraZoom);
		}
	}
}

void AStrategyPlayerController::DoCameraRotateYawCommand(float DeltaAngle)
{
	TargetYaw = FRotator::NormalizeAxis(TargetYaw + DeltaAngle);

	if (!bEnableCameraRotation || RotationInterpSpeed <= 0.0f)
	{
		CurrentYaw = TargetYaw;
		if (ControlledCameraPawn)
		{
			ControlledCameraPawn->SetCameraRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
		}
	}
}

void AStrategyPlayerController::DoCameraRotatePitchCommand(float DeltaAngle)
{
	TargetPitch = FMath::Clamp(TargetPitch + DeltaAngle, MinPitchAngle, MaxPitchAngle);

	if (!bEnableCameraRotation || RotationInterpSpeed <= 0.0f)
	{
		CurrentPitch = TargetPitch;
		if (ControlledCameraPawn)
		{
			ControlledCameraPawn->SetCameraRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
		}
	}
}

void AStrategyPlayerController::DoCameraResetRotationCommand()
{
	TargetYaw = DefaultYaw;
	TargetPitch = DefaultPitch;

	if (!bEnableCameraRotation || RotationInterpSpeed <= 0.0f)
	{
		CurrentYaw = TargetYaw;
		CurrentPitch = TargetPitch;
		if (ControlledCameraPawn)
		{
			ControlledCameraPawn->SetCameraRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
		}
	}
}

void AStrategyPlayerController::DoCameraResetAllCommand()
{
	DoCameraResetZoomCommand();
	DoCameraResetRotationCommand();
}

void AStrategyPlayerController::DoCameraFocusOnSelectionCommand()
{
	if (ControlledUnits.Num() == 0 || !ControlledCameraPawn)
	{
		return;
	}

	FVector Centroid = FVector::ZeroVector;
	int32 ValidCount = 0;
	for (AStrategyUnit* Unit : ControlledUnits)
	{
		if (IsValid(Unit))
		{
			Centroid += Unit->GetActorLocation();
			ValidCount++;
		}
	}

	if (ValidCount > 0)
	{
		Centroid /= ValidCount;
		DoCameraFocusOnLocationCommand(Centroid);
	}
}

void AStrategyPlayerController::DoCameraFocusOnLocationCommand(const FVector& TargetLocation, bool bInstant)
{
	if (!ControlledCameraPawn)
	{
		return;
	}

	FVector DesiredLoc = TargetLocation;
	DesiredLoc.Z = ControlledCameraPawn->GetActorLocation().Z;

	if (bInstant)
	{
		ControlledCameraPawn->SetActorLocation(DesiredLoc);
		ClampCameraToBounds();
		bIsFocusing = false;
	}
	else
	{
		TargetFocusLocation = DesiredLoc;
		bIsFocusing = true;
	}
}

void AStrategyPlayerController::ProcessEdgeScrolling(float DeltaTime)
{
	if (!bEnableEdgeScrolling || bIsMMBPanning || !ControlledCameraPawn)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return;
	}

	// Check if cursor is within viewport bounds
	if (MouseX < 0.0f || MouseX > ViewportSizeX || MouseY < 0.0f || MouseY > ViewportSizeY)
	{
		return;
	}

	FVector2D ScrollDir = FVector2D::ZeroVector;

	if (MouseX <= EdgeScrollMargin && EdgeScrollMargin > 0.0f)
	{
		ScrollDir.X -= 1.0f - FMath::Clamp(MouseX / EdgeScrollMargin, 0.0f, 1.0f);
	}
	else if (MouseX >= ViewportSizeX - EdgeScrollMargin && EdgeScrollMargin > 0.0f)
	{
		ScrollDir.X += 1.0f - FMath::Clamp((ViewportSizeX - MouseX) / EdgeScrollMargin, 0.0f, 1.0f);
	}

	if (MouseY <= EdgeScrollMargin && EdgeScrollMargin > 0.0f)
	{
		ScrollDir.Y += 1.0f - FMath::Clamp(MouseY / EdgeScrollMargin, 0.0f, 1.0f);
	}
	else if (MouseY >= ViewportSizeY - EdgeScrollMargin && EdgeScrollMargin > 0.0f)
	{
		ScrollDir.Y -= 1.0f - FMath::Clamp((ViewportSizeY - MouseY) / EdgeScrollMargin, 0.0f, 1.0f);
	}

	if (!ScrollDir.IsNearlyZero())
	{
		bIsFocusing = false;
		ScrollDir.Normalize();

		const FRotator YawRot(0.0f, CurrentYaw, 0.0f);
		const FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

		const float Speed = EdgeScrollSpeed * (bIsFastPanActive ? FastPanMultiplier : 1.0f);
		const FVector MoveDelta = (RightDir * ScrollDir.X + ForwardDir * ScrollDir.Y) * Speed * DeltaTime;

		ControlledCameraPawn->AddActorWorldOffset(MoveDelta);
		ClampCameraToBounds();
	}
}

void AStrategyPlayerController::ClampCameraToBounds()
{
	if (!bConstrainCameraToBounds || !ControlledCameraPawn)
	{
		return;
	}

	FVector CurrentLoc = ControlledCameraPawn->GetActorLocation();
	const float ClampedX = FMath::Clamp(CurrentLoc.X, CameraBoundsMin.X, CameraBoundsMax.X);
	const float ClampedY = FMath::Clamp(CurrentLoc.Y, CameraBoundsMin.Y, CameraBoundsMax.Y);

	if (!FMath::IsNearlyEqual(ClampedX, CurrentLoc.X) || !FMath::IsNearlyEqual(ClampedY, CurrentLoc.Y))
	{
		CurrentLoc.X = ClampedX;
		CurrentLoc.Y = ClampedY;
		ControlledCameraPawn->SetActorLocation(CurrentLoc);
	}
}

AStrategyUnit* AStrategyPlayerController::GetClosestSelectedUnitToLocation(FVector TargetLocation)
{
	// closest unit and distance
	AStrategyUnit* OutUnit = nullptr;
	float Closest = 0.0f;

	// process each unit on the list
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (IsValid(CurrentUnit))
		{
			// have we selected a unit already?
			if (OutUnit != nullptr)
			{
				// calculate the squared distance to the target location
				float Dist = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());

				// is this unit closer?
				if (Dist < Closest)
				{
					// update the closest unit and distance
					OutUnit = CurrentUnit;
					Closest = Dist;
				}

			}
			else
			{

				// no previously selected unit, so use this one
				OutUnit = CurrentUnit;

				// initialize the closest distance
				Closest = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());
			}
		}
		
	}

	// return the selected unit
	return OutUnit;
}

FVector2D AStrategyPlayerController::GetMouseLocationForPlayer()
{
	// attempt to get the mouse position from this PC
	float MouseX, MouseY;

	if (GetMousePosition(MouseX, MouseY))
	{
		return FVector2D(MouseX, MouseY);
	}

	// return an invalid vector
	return FVector2D::ZeroVector;
}

bool AStrategyPlayerController::GetLocationUnderCursor(FVector& Location)
{
	// trace the visibility channel at the cursor location
	FHitResult OutHit;

	GetHitResultUnderCursorByChannel(SelectionTraceChannel, false, OutHit);

	// if there was a blocking hit, return the hit location
	if (OutHit.bBlockingHit)
	{
		Location = OutHit.Location;
		return true;
	}

	return OutHit.bBlockingHit;
}

bool AStrategyPlayerController::GetLocationUnderFinger(FVector& Location)
{
	// trace the visibility channel at Touch 1 location
	FHitResult OutHit;

	GetHitResultUnderFingerByChannel(ETouchIndex::Touch1, SelectionTraceChannel, false, OutHit);

	// if there was a blocking hit, return the hit location
	if (OutHit.bBlockingHit)
	{
		Location = OutHit.Location;
		return true;
	}

	return OutHit.bBlockingHit;
}

FVector AStrategyPlayerController::ProjectTouchPointToWorldSpace()
{
	// get the touch coordinates for the first finger
	float TouchX, TouchY = 0.0f;
	bool bPressed = false;

	GetInputTouchState(ETouchIndex::Touch1, TouchX, TouchY, bPressed);

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;

	// deproject the coords into world space
	if (DeprojectScreenPositionToWorld(TouchX, TouchY, WorldLocation, WorldDirection))
	{
		// run a line trace down the camera
		FHitResult OutHit;

		GetWorld()->LineTraceSingleByChannel(OutHit, WorldLocation, WorldLocation + WorldDirection * 10000.0f, ECC_Visibility);

		// if we hit something, return the impact point
		if (OutHit.bBlockingHit)
		{
			return OutHit.ImpactPoint;
		}
		

		// intersect with a horizontal plane and return the resulting point
		const FPlane IntersectPlane(FVector::ZeroVector, FVector::UpVector);
		return FMath::LinePlaneIntersection(WorldLocation, WorldLocation + (WorldDirection * 100000.0f), IntersectPlane);
	}

	// failed to deproject, return a zero vector
	return FVector::ZeroVector;
}