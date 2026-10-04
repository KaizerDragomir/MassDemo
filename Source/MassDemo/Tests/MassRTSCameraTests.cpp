// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "StrategyPawn.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

// 1. Camera Zoom Calculations Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassRTSCameraZoomTest, "MassDemo.RTSCamera.ZoomMath", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassRTSCameraZoomTest::RunTest(const FString& Parameters)
{
	const float MinZoom = 500.0f;
	const float MaxZoom = 3000.0f;
	const float DefaultZoom = 1500.0f;

	float TargetZoom = DefaultZoom;

	// Modify zoom with positive delta (zoom out)
	TargetZoom = FMath::Clamp(TargetZoom + 500.0f, MinZoom, MaxZoom);
	TestEqual(TEXT("Zoom should increase by 500"), TargetZoom, 2000.0f);

	// Modify zoom past max (should clamp)
	TargetZoom = FMath::Clamp(TargetZoom + 2000.0f, MinZoom, MaxZoom);
	TestEqual(TEXT("Zoom should clamp to MaxZoom"), TargetZoom, MaxZoom);

	// Modify zoom past min (should clamp)
	TargetZoom = FMath::Clamp(TargetZoom - 5000.0f, MinZoom, MaxZoom);
	TestEqual(TEXT("Zoom should clamp to MinZoom"), TargetZoom, MinZoom);

	// Percentage zoom
	const float Pct50 = FMath::Lerp(MinZoom, MaxZoom, 0.5f);
	TestEqual(TEXT("50% zoom should be halfway between min and max"), Pct50, 1750.0f);

	const float Pct0 = FMath::Lerp(MinZoom, MaxZoom, 0.0f);
	TestEqual(TEXT("0% zoom should be MinZoom"), Pct0, MinZoom);

	const float Pct100 = FMath::Lerp(MinZoom, MaxZoom, 1.0f);
	TestEqual(TEXT("100% zoom should be MaxZoom"), Pct100, MaxZoom);

	return true;
}

// 2. Camera Rotation and Pitch Clamping Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassRTSCameraRotationTest, "MassDemo.RTSCamera.RotationMath", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassRTSCameraRotationTest::RunTest(const FString& Parameters)
{
	const float MinPitch = -80.0f;
	const float MaxPitch = -15.0f;
	const float DefaultPitch = -45.0f;
	const float DefaultYaw = -45.0f;

	float TargetYaw = DefaultYaw;
	float TargetPitch = DefaultPitch;

	// Rotate Yaw by +90 deg
	TargetYaw = FRotator::NormalizeAxis(TargetYaw + 90.0f);
	TestEqual(TEXT("Yaw should be +45 deg"), TargetYaw, 45.0f);

	// Rotate Yaw past 180 (should normalize)
	TargetYaw = FRotator::NormalizeAxis(TargetYaw + 200.0f);
	TestTrue(TEXT("Yaw should remain within [-180, 180] range"), TargetYaw >= -180.0f && TargetYaw <= 180.0f);

	// Rotate Pitch up past MaxPitch (should clamp)
	TargetPitch = FMath::Clamp(TargetPitch + 50.0f, MinPitch, MaxPitch);
	TestEqual(TEXT("Pitch should clamp to MaxPitch"), TargetPitch, MaxPitch);

	// Rotate Pitch down past MinPitch (should clamp)
	TargetPitch = FMath::Clamp(TargetPitch - 100.0f, MinPitch, MaxPitch);
	TestEqual(TEXT("Pitch should clamp to MinPitch"), TargetPitch, MinPitch);

	return true;
}

// 3. Camera World Bounds Constraint Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassRTSCameraBoundsTest, "MassDemo.RTSCamera.BoundsClamping", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassRTSCameraBoundsTest::RunTest(const FString& Parameters)
{
	const FVector2D BoundsMin(-5000.0f, -3000.0f);
	const FVector2D BoundsMax(5000.0f, 3000.0f);

	// Location inside bounds
	FVector LocInside(1000.0f, -500.0f, 1500.0f);
	LocInside.X = FMath::Clamp(LocInside.X, BoundsMin.X, BoundsMax.X);
	LocInside.Y = FMath::Clamp(LocInside.Y, BoundsMin.Y, BoundsMax.Y);
	TestEqual(TEXT("Inside location X should remain unchanged"), LocInside.X, 1000.0);
	TestEqual(TEXT("Inside location Y should remain unchanged"), LocInside.Y, -500.0);

	// Location outside bounds
	FVector LocOutside(10000.0f, -8000.0f, 1500.0f);
	LocOutside.X = FMath::Clamp(LocOutside.X, BoundsMin.X, BoundsMax.X);
	LocOutside.Y = FMath::Clamp(LocOutside.Y, BoundsMin.Y, BoundsMax.Y);
	TestEqual(TEXT("Outside location X should be clamped to BoundsMax.X"), LocOutside.X, BoundsMax.X);
	TestEqual(TEXT("Outside location Y should be clamped to BoundsMin.Y"), LocOutside.Y, BoundsMin.Y);

	return true;
}

// 4. Camera Edge Scrolling Direction Calculation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassRTSCameraEdgeScrollTest, "MassDemo.RTSCamera.EdgeScrollMath", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassRTSCameraEdgeScrollTest::RunTest(const FString& Parameters)
{
	const int32 ViewportWidth = 1920;
	const int32 ViewportHeight = 1080;
	const float Margin = 20.0f;

	auto CalcScrollDir = [Margin, ViewportWidth, ViewportHeight](float MouseX, float MouseY) -> FVector2D
	{
		FVector2D Dir = FVector2D::ZeroVector;
		if (MouseX <= Margin && Margin > 0.0f)
		{
			Dir.X -= 1.0f - FMath::Clamp(MouseX / Margin, 0.0f, 1.0f);
		}
		else if (MouseX >= ViewportWidth - Margin && Margin > 0.0f)
		{
			Dir.X += 1.0f - FMath::Clamp((ViewportWidth - MouseX) / Margin, 0.0f, 1.0f);
		}

		if (MouseY <= Margin && Margin > 0.0f)
		{
			Dir.Y += 1.0f - FMath::Clamp(MouseY / Margin, 0.0f, 1.0f);
		}
		else if (MouseY >= ViewportHeight - Margin && Margin > 0.0f)
		{
			Dir.Y -= 1.0f - FMath::Clamp((ViewportHeight - MouseY) / Margin, 0.0f, 1.0f);
		}
		return Dir;
	};

	// Center screen: no scroll
	FVector2D CenterDir = CalcScrollDir(960.0f, 540.0f);
	TestTrue(TEXT("Center cursor should have zero scroll direction"), CenterDir.IsNearlyZero());

	// Left edge: scroll left (Dir.X < 0)
	FVector2D LeftDir = CalcScrollDir(5.0f, 540.0f);
	TestTrue(TEXT("Left cursor should have negative X direction"), LeftDir.X < 0.0f);
	TestTrue(TEXT("Left cursor should have zero Y direction"), FMath::IsNearlyZero(LeftDir.Y));

	// Right edge: scroll right (Dir.X > 0)
	FVector2D RightDir = CalcScrollDir(1915.0f, 540.0f);
	TestTrue(TEXT("Right cursor should have positive X direction"), RightDir.X > 0.0f);
	TestTrue(TEXT("Right cursor should have zero Y direction"), FMath::IsNearlyZero(RightDir.Y));

	// Top edge: scroll up/forward (Dir.Y > 0)
	FVector2D TopDir = CalcScrollDir(960.0f, 5.0f);
	TestTrue(TEXT("Top cursor should have positive Y direction"), TopDir.Y > 0.0f);
	TestTrue(TEXT("Top cursor should have zero X direction"), FMath::IsNearlyZero(TopDir.X));

	// Bottom edge: scroll down/backward (Dir.Y < 0)
	FVector2D BottomDir = CalcScrollDir(960.0f, 1075.0f);
	TestTrue(TEXT("Bottom cursor should have negative Y direction"), BottomDir.Y < 0.0f);
	TestTrue(TEXT("Bottom cursor should have zero X direction"), FMath::IsNearlyZero(BottomDir.X));

	// Top-Left corner: diagonal
	FVector2D TopLeftDir = CalcScrollDir(0.0f, 0.0f);
	TestTrue(TEXT("Top-Left cursor should have negative X and positive Y"), TopLeftDir.X < 0.0f && TopLeftDir.Y > 0.0f);

	return true;
}

// 5. Camera Focus Centroid Calculation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassRTSCameraFocusCentroidTest, "MassDemo.RTSCamera.FocusCentroid", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassRTSCameraFocusCentroidTest::RunTest(const FString& Parameters)
{
	TArray<FVector> UnitPositions = {
		FVector(100.0f, 200.0f, 0.0f),
		FVector(300.0f, 400.0f, 0.0f),
		FVector(500.0f, 600.0f, 0.0f)
	};

	FVector Centroid = FVector::ZeroVector;
	for (const FVector& Pos : UnitPositions)
	{
		Centroid += Pos;
	}
	Centroid /= UnitPositions.Num();

	TestEqual(TEXT("Centroid X should be average of all unit X positions"), Centroid.X, 300.0);
	TestEqual(TEXT("Centroid Y should be average of all unit Y positions"), Centroid.Y, 400.0);
	TestEqual(TEXT("Centroid Z should be average of all unit Z positions"), Centroid.Z, 0.0);

	return true;
}

// 6. Camera Movement Rotation and Speed Boost Math Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassRTSCameraMovementMathTest, "MassDemo.RTSCamera.MovementMath", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassRTSCameraMovementMathTest::RunTest(const FString& Parameters)
{
	// At Yaw = 0, Forward is +X, Right is +Y
	{
		const FRotator YawRot0(0.0f, 0.0f, 0.0f);
		const FVector Fwd0 = FRotationMatrix(YawRot0).GetUnitAxis(EAxis::X);
		const FVector Right0 = FRotationMatrix(YawRot0).GetUnitAxis(EAxis::Y);
		TestNearlyEqual(TEXT("Yaw 0 Forward X"), static_cast<float>(Fwd0.X), 1.0f);
		TestNearlyEqual(TEXT("Yaw 0 Forward Y"), static_cast<float>(Fwd0.Y), 0.0f);
		TestNearlyEqual(TEXT("Yaw 0 Right X"), static_cast<float>(Right0.X), 0.0f);
		TestNearlyEqual(TEXT("Yaw 0 Right Y"), static_cast<float>(Right0.Y), 1.0f);
	}

	// At Yaw = 90, Forward is +Y, Right is -X
	{
		const FRotator YawRot90(0.0f, 90.0f, 0.0f);
		const FVector Fwd90 = FRotationMatrix(YawRot90).GetUnitAxis(EAxis::X);
		const FVector Right90 = FRotationMatrix(YawRot90).GetUnitAxis(EAxis::Y);
		TestNearlyEqual(TEXT("Yaw 90 Forward X"), static_cast<float>(Fwd90.X), 0.0f);
		TestNearlyEqual(TEXT("Yaw 90 Forward Y"), static_cast<float>(Fwd90.Y), 1.0f);
		TestNearlyEqual(TEXT("Yaw 90 Right X"), static_cast<float>(Right90.X), -1.0f);
		TestNearlyEqual(TEXT("Yaw 90 Right Y"), static_cast<float>(Right90.Y), 0.0f);
	}

	// Fast Pan Multiplier check
	const float BaseSpeed = 1000.0f;
	const float FastMultiplier = 2.5f;
	const bool bFastActive = true;
	const float EffectiveSpeed = BaseSpeed * (bFastActive ? FastMultiplier : 1.0f);
	TestEqual(TEXT("Effective speed with fast pan should be multiplied"), EffectiveSpeed, 2500.0f);

	return true;
}

#endif
