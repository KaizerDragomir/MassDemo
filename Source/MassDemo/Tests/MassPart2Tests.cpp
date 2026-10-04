// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Grid/MassGridTypes.h"
#include "Grid/MassGridFragments.h"
#include "Grid/WanderingWorldActor.h"
#include "Grid/WorldGridComponent.h"
#include "Grid/MassGridPathfinder.h"
#include "Grid/MassGridBuilding.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassPart2FragmentsTest, "MassDemo.Part2.Fragments", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassPart2FragmentsTest::RunTest(const FString& Parameters)
{
	// 1. Test FMassGridLocationFragment
	FMassGridLocationFragment LocFrag;
	LocFrag.CurrentCoord = FGridCoord(3, 4);
	LocFrag.TargetCoord = FGridCoord(7, 8);
	LocFrag.LocalPosition = FVector(300.0f, 400.0f, 50.0f);
	TestEqual(TEXT("CurrentCoord should match"), LocFrag.CurrentCoord, FGridCoord(3, 4));
	TestEqual(TEXT("TargetCoord should match"), LocFrag.TargetCoord, FGridCoord(7, 8));

	// 2. Test FMassPathFollowingFragment
	FMassPathFollowingFragment PathFrag;
	TestFalse(TEXT("Newly created path fragment should not have active path"), PathFrag.bHasActivePath);
	TestTrue(TEXT("Empty path fragment should report destination reached"), PathFrag.HasReachedDestination());

	const TArray<FVector> TestWaypoints = {
		FVector(0.0f, 0.0f, 50.0f),
		FVector(100.0f, 0.0f, 50.0f),
		FVector(200.0f, 0.0f, 50.0f)
	};
	TestTrue(TEXT("SetPath should accept waypoints within fixed buffer"), PathFrag.SetPath(TestWaypoints));
	TestTrue(TEXT("SetPath should activate path"), PathFrag.bHasActivePath);
	TestEqual(TEXT("Waypoint count should match"), PathFrag.NumWaypoints, 3);
	TestEqual(TEXT("CurrentWaypointIndex should start at 1"), PathFrag.CurrentWaypointIndex, 1);
	TestFalse(TEXT("Active path should not be reached"), PathFrag.HasReachedDestination());

	PathFrag.ClearPath();
	TestFalse(TEXT("ClearPath should deactivate path"), PathFrag.bHasActivePath);
	TestEqual(TEXT("NumWaypoints after clear should be 0"), PathFrag.NumWaypoints, 0);

	TArray<FVector> TooManyWaypoints;
	TooManyWaypoints.SetNum(FMassPathFollowingFragment::MaxWaypoints + 1);
	TestFalse(TEXT("SetPath should reject paths that exceed fixed buffer"), PathFrag.SetPath(TooManyWaypoints));
	TestFalse(TEXT("Rejected oversized path should not activate path"), PathFrag.bHasActivePath);
	TestTrue(TEXT("Rejected oversized path should request reroute"), PathFrag.bNeedsReroute);

	// 3. Test FMassStunFragment
	FMassStunFragment StunFrag;
	TestFalse(TEXT("Default stun state should be false"), StunFrag.bIsStunned);
	TestEqual(TEXT("Default stun time should be 0"), StunFrag.StunTimeRemaining, 0.0f);

	StunFrag.ApplyStun(2.5f);
	TestTrue(TEXT("ApplyStun should set bIsStunned to true"), StunFrag.bIsStunned);
	TestEqual(TEXT("Stun duration should match applied duration"), StunFrag.StunTimeRemaining, 2.5f);

	StunFrag.ClearStun();
	TestFalse(TEXT("ClearStun should reset bIsStunned to false"), StunFrag.bIsStunned);
	TestEqual(TEXT("ClearStun should reset StunTimeRemaining to 0"), StunFrag.StunTimeRemaining, 0.0f);

	// 4. Test FMassUnitBehaviorFragment
	FMassUnitBehaviorFragment BehaviorFrag;
	BehaviorFrag.MinIdleTime = 1.0f;
	BehaviorFrag.MaxIdleTime = 3.0f;
	BehaviorFrag.ResetIdleTimer();
	TestTrue(TEXT("Idle time should be >= MinIdleTime"), BehaviorFrag.IdleTimeRemaining >= 1.0f);
	TestTrue(TEXT("Idle time should be <= MaxIdleTime"), BehaviorFrag.IdleTimeRemaining <= 3.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassPart2WanderingActorLogicTest, "MassDemo.Part2.WanderingActorLogic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassPart2WanderingActorLogicTest::RunTest(const FString& Parameters)
{
	// Test grid initialization and coordinate transforms in local & world space
	FWorldGridData Grid;
	Grid.Initialize(10, 15, 100.0f);

	const FGridCoord CenterCoord(5, 7);
	const FVector LocalPos = Grid.CellToLocalPosition(CenterCoord, 50.0f);

	const FTransform WorldActorTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(1000.0f, 2000.0f, 300.0f));
	const FVector WorldPos = WorldActorTransform.TransformPosition(LocalPos);

	// Transform back
	const FVector InvertedLocalPos = WorldActorTransform.InverseTransformPosition(WorldPos);
	const FGridCoord RecoveredCoord = Grid.LocalPositionToCell(InvertedLocalPos);

	TestEqual(TEXT("Round-trip world-to-local grid coordinate must match original"), RecoveredCoord, CenterCoord);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassPart2PathfindingToWaypointsTest, "MassDemo.Part2.PathfindingToWaypoints", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassPart2PathfindingToWaypointsTest::RunTest(const FString& Parameters)
{
	FWorldGridData Grid;
	Grid.Initialize(10, 10, 100.0f);

	const FGridCoord Start(0, 0);
	const FGridCoord Goal(3, 3);

	TArray<FGridCoord> Path;
	const bool bFound = UMassGridPathfinderLibrary::FindPath(Grid, Start, Goal, Path);
	TestTrue(TEXT("Path should be found on free grid"), bFound);
	TestTrue(TEXT("Path should contain multiple coordinates"), Path.Num() > 1);

	TArray<FVector> LocalWaypoints;
	UMassGridPathfinderLibrary::ConvertPathToLocalPositions(Grid, Path, LocalWaypoints, 50.0f);
	TestEqual(TEXT("Waypoint count should match path coordinate count"), LocalWaypoints.Num(), Path.Num());

	FMassPathFollowingFragment PathFrag;
	TestTrue(TEXT("PathFrag should accept generated waypoints"), PathFrag.SetPath(LocalWaypoints));

	TestTrue(TEXT("PathFrag should have active path"), PathFrag.bHasActivePath);
	TestEqual(TEXT("PathFrag should have correct number of waypoints"), PathFrag.NumWaypoints, LocalWaypoints.Num());
	TestEqual(TEXT("PathFrag should start from waypoint index 1"), PathFrag.CurrentWaypointIndex, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassPart2UnitSphereVisualizationTest, "MassDemo.Part2.UnitSphereVisualization", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassPart2UnitSphereVisualizationTest::RunTest(const FString& Parameters)
{
	const FVector UnitLocalPos(150.0f, 250.0f, 0.0f);
	const float CustomZOffset = 30.0f;
	const float CustomScale = 0.4f;

	// 1. Test normal (non-stunned) unit transform
	const FTransform NormalTransform = UMassGridBuildingLibrary::CalculateUnitTransform(UnitLocalPos, CustomZOffset, CustomScale, false);
	TestEqual(TEXT("Unit X position should be unchanged"), NormalTransform.GetLocation().X, 150.0);
	TestEqual(TEXT("Unit Y position should be unchanged"), NormalTransform.GetLocation().Y, 250.0);
	TestEqual(TEXT("Unit Z position should include UnitZOffset"), NormalTransform.GetLocation().Z, 30.0);
	TestEqual(TEXT("Unit scale should match uniform scale"), NormalTransform.GetScale3D(), FVector(0.4, 0.4, 0.4));

	// 2. Test stunned unit transform (squash visual feedback)
	const FTransform StunnedTransform = UMassGridBuildingLibrary::CalculateUnitTransform(UnitLocalPos, CustomZOffset, CustomScale, true);
	TestEqual(TEXT("Stunned unit X position should be unchanged"), StunnedTransform.GetLocation().X, 150.0);
	TestEqual(TEXT("Stunned unit Y position should be unchanged"), StunnedTransform.GetLocation().Y, 250.0);
	TestEqual(TEXT("Stunned unit Z position should include UnitZOffset"), StunnedTransform.GetLocation().Z, 30.0);
	TestNearlyEqual(TEXT("Stunned unit scale X should be expanded"), static_cast<float>(StunnedTransform.GetScale3D().X), 0.4f * 1.2f);
	TestNearlyEqual(TEXT("Stunned unit scale Y should be expanded"), static_cast<float>(StunnedTransform.GetScale3D().Y), 0.4f * 1.2f);
	TestNearlyEqual(TEXT("Stunned unit scale Z should be squashed"), static_cast<float>(StunnedTransform.GetScale3D().Z), 0.4f * 0.6f);

	return true;
}

#endif
