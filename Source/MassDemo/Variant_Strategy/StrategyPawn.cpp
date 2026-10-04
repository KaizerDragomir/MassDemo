// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyPawn.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

AStrategyPawn::AStrategyPawn()
{
 	PrimaryActorTick.bCanEverTick = true;

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);

	// create the movement component
	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Floating Pawn Movement"));

	// configure the camera
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->OrthoWidth = 1500.0f;
	Camera->AutoPlaneShift = 1.0f;
	Camera->bUpdateOrthoPlanes = false;

	// configure the movement comp
	FloatingPawnMovement->bConstrainToPlane = true;
	FloatingPawnMovement->SetPlaneConstraintNormal(FVector::UpVector);
	FloatingPawnMovement->SetPlaneConstraintOrigin(FVector::UpVector * 1500.0f);
}

void AStrategyPawn::SetZoomModifier(float Value)
{
	if (Camera)
	{
		if (Camera->ProjectionMode == ECameraProjectionMode::Orthographic)
		{
			// set the ortho width on the camera
			Camera->SetOrthoWidth(Value);
		}
		else
		{
			// In perspective projection, adjust relative height / distance
			FVector CamLoc = Camera->GetRelativeLocation();
			CamLoc.Z = Value;
			Camera->SetRelativeLocation(CamLoc);
		}
	}
}

void AStrategyPawn::SetCameraRotation(const FRotator& NewRotation)
{
	if (Camera)
	{
		Camera->SetRelativeRotation(NewRotation);
	}
}

void AStrategyPawn::AddCameraYaw(float AngleDelta)
{
	if (Camera)
	{
		FRotator Rot = Camera->GetRelativeRotation();
		Rot.Yaw = FRotator::NormalizeAxis(Rot.Yaw + AngleDelta);
		Camera->SetRelativeRotation(Rot);
	}
}

void AStrategyPawn::AddCameraPitch(float AngleDelta)
{
	if (Camera)
	{
		FRotator Rot = Camera->GetRelativeRotation();
		Rot.Pitch = FMath::Clamp(Rot.Pitch + AngleDelta, -85.0f, -10.0f);
		Camera->SetRelativeRotation(Rot);
	}
}

void AStrategyPawn::ResetCameraRotation()
{
	if (Camera)
	{
		Camera->SetRelativeRotation(FRotator(-45.0f, -45.0f, 0.0f));
	}
}
