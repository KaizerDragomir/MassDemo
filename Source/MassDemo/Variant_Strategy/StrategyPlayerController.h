// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "StrategyPlayerController.generated.h"

class AStrategyPawn;
class UInputMappingContext;
class UNiagaraSystem;
struct FInputActionValue;
class AStrategyHUD;
class UInputAction;
struct FInputActionInstance;
class AStrategyUnit;
class UStrategyTouchControls;

/**
 *  Player Controller for a top-down strategy game.
 *  Handles unit selection and commands.
 *  Implements both mouse and touch controls.
 */
UCLASS(abstract)
class AStrategyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Strategy Pawn associated with this controller */
	TObjectPtr<AStrategyPawn> ControlledCameraPawn;

	/** Strategy HUD associated with this controller */
	TObjectPtr<AStrategyHUD> StrategyHUD;

	/** Input mapping context to use with mouse input */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* MouseMappingContext;

	/** Input mapping context to use with touch input */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* TouchMappingContext;

	/** If true, the player is adding or removing units to the selected units list */
	bool bSelectionModifier = false;

	/** If true, double-tap touch select all mode is active */
	bool bDoubleTapActive = false;

	/** If true, allow the player to interact with game objects */
	bool bAllowInteraction = true;

	/** Input Action for moving the camera */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveCameraAction;

	/** Input Action for zooming the camera */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ZoomCameraAction;

	/** Input Action for resetting the camera to its default position */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ResetCameraAction;

	/** Input Action for rotating the camera */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* RotateCameraAction;

	/** Input Action for focusing the camera on selected units */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* FocusCameraAction;

	/** Input Action for fast movement modifier (e.g. Shift) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* FastMoveModifierAction;

	/** Input Action for middle mouse button drag panning */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MMBPanAction;

	/** Input Action for select click */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectClickAction;

	/** Input Action for additive select click */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectClickAdditiveAction;

	/** Input Action for select all double click */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectAllDoubleClickAction;

	/** Input Action for select press and hold */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectHoldAction;

	/** Input Action for click interaction */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractClickAction;

	/** Input Action for interaction press and hold */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractHoldAction;

	/** Input Action for modifying selection mode */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectionModifierAction;

	/** Input Action for primary touch hold */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TouchPrimaryHoldAction;

	/** Input Action for secondary touch */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TouchSecondaryAction;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UStrategyTouchControls> MobileControlsWidget;

	/** Touch controls widget class to spawn on mobile platforms */
	UPROPERTY(EditAnywhere, Category="Input")
	TSubclassOf<UStrategyTouchControls> MobileControlsWidgetClass;

	/** If true, the PC will be initialized with touchscreen controls */
	UPROPERTY(EditAnywhere, Category="Input")
	bool bForceTouchControls = false;

	/** Max distance to look for nearby units when doing a click or touch interaction */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float SelectionRadius = 250.0f;

	/** Cached starting position for camera drag scrolling */
	FVector2D StartingDragScrollPosition;

	/** Cached starting position for box select */
	FVector2D StartingBoxSelectionPosition;

	/** Last game time when a drag scroll was performed, so we can avoid spamming commands on drag scroll end */
	float LastTouchDragScrollTime = 0.0f;

	/** Time the finger needs to be held down to initiate a drag scroll */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 1, Units = "s"))
	float TouchDragScrollHoldTime = 0.15f;

	/** Current camera zoom level */
	float CameraZoom = 0.0f;

	/** Target camera zoom level for smooth interpolation */
	float TargetCameraZoom = 0.0f;

	/** Default camera zoom level */
	float DefaultZoom = 1000.0f;

	/** Minimum allowed camera zoom level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = 0, ClampMax = 10000))
	float MinZoomLevel = 1000.0f;

	/** Maximum allowed camera zoom level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = 0, ClampMax = 10000))
	float MaxZoomLevel = 2500.0f;

	/** Scales zoom inputs by this value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = 0, ClampMax = 1000))
	float ZoomScaling = 100.0f;

	/** If true, camera zoom interpolates smoothly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	bool bEnableSmoothZoom = true;

	/** Speed of smooth zoom interpolation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = 0.1, ClampMax = 100))
	float ZoomInterpSpeed = 10.0f;

	/** Affects how fast the camera moves while dragging with the mouse */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|DragPan", meta = (ClampMin = 0, ClampMax = 10000))
	float DragMultiplier = 0.1f;

	/** Sensitivity for middle mouse button drag panning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|DragPan", meta = (ClampMin = 0.1, ClampMax = 20))
	float MMBPanSensitivity = 1.5f;

	/** True if currently panning with middle mouse button */
	bool bIsMMBPanning = false;

	/** Last mouse position during MMB panning */
	FVector2D LastMMBMousePosition;

	/** If true, edge scrolling is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|EdgeScrolling")
	bool bEnableEdgeScrolling = true;

	/** Margin from viewport edges (in pixels) that triggers edge scrolling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|EdgeScrolling", meta = (ClampMin = 1, ClampMax = 200))
	float EdgeScrollMargin = 15.0f;

	/** Movement speed when edge scrolling (units per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|EdgeScrolling", meta = (ClampMin = 100, ClampMax = 20000))
	float EdgeScrollSpeed = 1500.0f;

	/** Speed boost multiplier when fast movement modifier is held */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement", meta = (ClampMin = 1, ClampMax = 10))
	float FastPanMultiplier = 2.0f;

	/** True if fast movement modifier (Shift) is active */
	bool bIsFastPanActive = false;

	/** If true, camera rotation is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation")
	bool bEnableCameraRotation = true;

	/** Rotation speed when using keyboard controls (degrees per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = 1, ClampMax = 720))
	float KeyboardRotationSpeed = 90.0f;

	/** Sensitivity for mouse-based camera rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = 0.1, ClampMax = 10))
	float MouseRotationSensitivity = 1.0f;

	/** Speed of smooth rotation interpolation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = 0.1, ClampMax = 100))
	float RotationInterpSpeed = 12.0f;

	/** Minimum allowed camera pitch angle (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = -89, ClampMax = 0))
	float MinPitchAngle = -80.0f;

	/** Maximum allowed camera pitch angle (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = -89, ClampMax = 0))
	float MaxPitchAngle = -15.0f;

	/** Current yaw rotation */
	float CurrentYaw = -45.0f;

	/** Target yaw rotation for smooth interpolation */
	float TargetYaw = -45.0f;

	/** Default yaw rotation */
	float DefaultYaw = -45.0f;

	/** Current pitch rotation */
	float CurrentPitch = -45.0f;

	/** Target pitch rotation for smooth interpolation */
	float TargetPitch = -45.0f;

	/** Default pitch rotation */
	float DefaultPitch = -45.0f;

	/** If true, camera is restricted within world bounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Bounds")
	bool bConstrainCameraToBounds = false;

	/** Minimum world coordinates for camera bounds (X, Y) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Bounds")
	FVector2D CameraBoundsMin = FVector2D(-10000.0f, -10000.0f);

	/** Maximum world coordinates for camera bounds (X, Y) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Bounds")
	FVector2D CameraBoundsMax = FVector2D(10000.0f, 10000.0f);

	/** Speed of camera interpolation when focusing on target/units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus", meta = (ClampMin = 0.1, ClampMax = 50))
	float FocusInterpSpeed = 8.0f;

	/** True if camera is currently smoothly panning to focus target */
	bool bIsFocusing = false;

	/** Target location for camera focus */
	FVector TargetFocusLocation = FVector::ZeroVector;

	/** Trace channel to use for selection trace checks */
	UPROPERTY(EditAnywhere, Category = "Selection")
	TEnumAsByte<ETraceTypeQuery> SelectionTraceChannel;

	/** Currently selected unit list */
	TArray<AStrategyUnit*> ControlledUnits;

public:

	/** Constructor */
	AStrategyPlayerController();

	/** BeginPlay initialization */
	virtual void BeginPlay() override;

	/** PlayerTick frame update */
	virtual void PlayerTick(float DeltaTime) override;

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	/** Pawn initialization */
	virtual void OnPossess(APawn* InPawn);

public:

	/** Updates selected units from the HUD's drag select box */
	void DragSelectUnits(const TArray<AStrategyUnit*>& Units);

	/** Passes the list of selected units */
	const TArray<AStrategyUnit*>& GetSelectedUnits();

	/** Returns the default camera zoom percentage value */
	float GetDefaultZoomPercentage() const;

protected:

	/** Returns true if the PC should run using touchscreen controls */
	bool ShouldUseTouchControls() const;

	// mouse + keyboard input

protected:

	/** Moves the camera by the given input */
	void MoveCamera(const FInputActionValue& Value);

	/** Changes the camera zoom level by the given input */
	void ZoomCamera(const FInputActionValue& Value);

	/** Resets the camera to its initial value */
	void ResetCamera(const FInputActionValue& Value);

	/** Rotates the camera based on input */
	void RotateCamera(const FInputActionValue& Value);

	/** Centers the camera on selected units */
	void FocusCamera(const FInputActionValue& Value);

	/** Starts fast pan mode */
	void FastMoveStarted(const FInputActionValue& Value);

	/** Ends fast pan mode */
	void FastMoveCompleted(const FInputActionValue& Value);

	/** Starts middle mouse button drag panning */
	void MMBPanStarted(const FInputActionValue& Value);

	/** Middle mouse button drag panning triggered */
	void MMBPanTriggered(const FInputActionValue& Value);

	/** Ends middle mouse button drag panning */
	void MMBPanCompleted(const FInputActionValue& Value);

	/** Start a select and hold input */
	void SelectHoldStarted(const FInputActionValue& Value);
	
	/** Select and hold input triggered */
	void SelectHoldTriggered(const FInputActionValue& Value);

	/** Select and hold input completed */
	void SelectHoldCompleted(const FInputActionValue& Value);

	/** Select click action */
	void SelectClick(const FInputActionValue& Value);

	/** Select Click Additive Action */
	void SelectClickAdditive(const FInputActionValue& Value);

	/** Select All Double Click Action */
	void SelectAllDoubleClick(const FInputActionValue& Value);

	/** Starts an interaction hold input */
	void InteractHoldStarted(const FInputActionValue& Value);

	/** Interaction hold input triggered */
	void InteractHoldTriggered(const FInputActionValue& Value);

	/** Interaction click input started */
	void InteractClick(const FInputActionValue& Value);

	/** Touch primary finger hold started */
	void TouchPrimaryHoldStarted(const FInputActionValue& Value);

	/** Touch primary finger hold triggered */
	void TouchPrimaryHoldTriggered(const FInputActionInstance& Instance);

	/** Touch primary finger hold completed */
	void TouchPrimaryHoldCompleted(const FInputActionValue& Value);

	/** Touch secondary finger triggered */
	void TouchSecondaryTriggered(const FInputActionValue& Value);

	/** Touch secondary finger completed */
	void TouchSecondaryCompleted(const FInputActionValue& Value);

	// commands

public:

	/** Attempt to select or deselect a unit near the given location. Supports additive selection modifier. */
	bool DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection);

	/** Attempts to select all units on screen */
	void DoSelectAllUnitsOnScreenCommand();

	/** Deselects any selected units */
	void DoDeselectAllUnitsCommand();

	/** Toggles between selecting all units on screen and deselecting units */
	void DoToggleSelectAllUnitsCommand();

	/** Scrolls the camera based on a new screen coordinate */
	void DoCameraDragScrollCommand(const FVector2D& CurrentCursorPosition);

	/** Attempts to move all selected units to the given location */
	void DoMoveUnitsCommand(const FVector& GoalLocation);

	/** Applies a zoom change to the camera */
	void DoCameraModifyZoomCommand(float ZoomDelta);

	/** Resets the camera zoom to default */
	void DoCameraResetZoomCommand();

	/** Sets the camera zoom to a percentage between min and max zoom */
	void DoCameraSetZoomPercentageCommand(float Percentage);

	/** Rotates camera yaw by DeltaAngle (degrees) */
	UFUNCTION(BlueprintCallable, Category = "Camera|Commands")
	void DoCameraRotateYawCommand(float DeltaAngle);

	/** Rotates camera pitch by DeltaAngle (degrees) */
	UFUNCTION(BlueprintCallable, Category = "Camera|Commands")
	void DoCameraRotatePitchCommand(float DeltaAngle);

	/** Resets camera rotation to default */
	UFUNCTION(BlueprintCallable, Category = "Camera|Commands")
	void DoCameraResetRotationCommand();

	/** Resets all camera settings (zoom + rotation) */
	UFUNCTION(BlueprintCallable, Category = "Camera|Commands")
	void DoCameraResetAllCommand();

	/** Focuses camera on currently selected units */
	UFUNCTION(BlueprintCallable, Category = "Camera|Commands")
	void DoCameraFocusOnSelectionCommand();

	/** Focuses camera on target location (optionally instant) */
	UFUNCTION(BlueprintCallable, Category = "Camera|Commands")
	void DoCameraFocusOnLocationCommand(const FVector& TargetLocation, bool bInstant = false);

	/** Returns current camera zoom value */
	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetCameraZoom() const { return CameraZoom; }

	/** Returns target camera zoom value */
	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetTargetCameraZoom() const { return TargetCameraZoom; }

	/** Returns current camera yaw */
	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetCurrentYaw() const { return CurrentYaw; }

	/** Returns target camera yaw */
	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetTargetYaw() const { return TargetYaw; }

	/** Returns current camera pitch */
	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetCurrentPitch() const { return CurrentPitch; }

	/** Returns target camera pitch */
	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetTargetPitch() const { return TargetPitch; }

	/** Returns true if edge scrolling is enabled */
	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsEdgeScrollingEnabled() const { return bEnableEdgeScrolling; }

	/** Sets whether edge scrolling is enabled */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetEdgeScrollingEnabled(bool bEnable) { bEnableEdgeScrolling = bEnable; }

	/** Sets camera world bounds */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraBounds(const FVector2D& InMin, const FVector2D& InMax) { CameraBoundsMin = InMin; CameraBoundsMax = InMax; bConstrainCameraToBounds = true; }

	/** Enables or disables camera bounds constraint */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraBoundsEnabled(bool bEnable) { bConstrainCameraToBounds = bEnable; }

	/** Returns true if fast pan is active */
	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsFastPanActive() const { return bIsFastPanActive; }

	/** Sets fast pan active state */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetFastPanActive(bool bActive) { bIsFastPanActive = bActive; }

	/** Returns true if currently middle mouse panning */
	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsMiddleMousePanning() const { return bIsMMBPanning; }

protected:

	/** Processes mouse edge scrolling per frame */
	void ProcessEdgeScrolling(float DeltaTime);

	/** Restricts camera pawn location within bounds if enabled */
	void ClampCameraToBounds();

protected:

	/** Sorts all controlled units based on their distance to the provided world location */
	AStrategyUnit* GetClosestSelectedUnitToLocation(FVector TargetLocation);

	/** Calculates and returns the current mouse location */
	FVector2D GetMouseLocationForPlayer();

	/** Attempts to get the world location under the cursor, returns true if successful */
	bool GetLocationUnderCursor(FVector& Location);

	/** Attempts to get the world location under the Touch 1 finger, returns true if successful */
	bool GetLocationUnderFinger(FVector& Location);

	/** Projects the current touch location into world space */
	FVector ProjectTouchPointToWorldSpace();

protected:

	/** Spawns the positive cursor effect */
	UFUNCTION(BlueprintImplementableEvent, Category="Cursor", meta = (DisplayName="Cursor Feedback"))
	void BP_CursorFeedback(FVector Location, bool bPositive);
};
