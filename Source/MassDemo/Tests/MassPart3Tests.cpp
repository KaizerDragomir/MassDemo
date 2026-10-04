// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Grid/MassGridTypes.h"
#include "Grid/MassGridFragments.h"
#include "Grid/MassGridPathfinder.h"
#include "Grid/MassGridBuilding.h"
#include "Grid/WorldGridComponent.h"
#include "HAL/PlatformTime.h"

#if WITH_DEV_AUTOMATION_TESTS

// 1. Dynamic Building Spawning Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassPart3DynamicBuildingSpawningTest, "MassDemo.Part3.DynamicBuildingSpawning", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassPart3DynamicBuildingSpawningTest::RunTest(const FString& Parameters)
{
	FWorldGridData Grid;
	Grid.Initialize(15, 15, 100.0f);

	TArray<FPlacedBuilding> Buildings;
	const int32 InitialPlaced = UMassGridBuildingLibrary::PopulateGridWithRandomBuildings(Grid, 2, Buildings);
	TestEqual(TEXT("Initial buildings placed should be 2"), InitialPlaced, 2);
	TestEqual(TEXT("Buildings array size should match"), Buildings.Num(), 2);

	// Dynamically spawn additional building
	const FTetrisBuildingShape NewShape = UMassGridBuildingLibrary::GetShapeTemplate(ETetrisShapeType::TShape3x2);
	FPlacedBuilding DynamicBuilding;
	bool bSpawned = false;

	for (int32 X = 0; X <= Grid.Width - NewShape.SizeX; ++X)
	{
		for (int32 Y = 0; Y <= Grid.Height - NewShape.SizeY; ++Y)
		{
			if (UMassGridBuildingLibrary::TryPlaceBuilding(Grid, NewShape, FGridCoord(X, Y), 3, DynamicBuilding))
			{
				bSpawned = true;
				break;
			}
		}
		if (bSpawned)
		{
			break;
		}
	}

	TestTrue(TEXT("Dynamic building should be successfully placed"), bSpawned);
	TestEqual(TEXT("Dynamic building ID should be 3"), DynamicBuilding.BuildingId, 3);

	// Verify all blocked cells are impassable
	for (const FGridCoord& BlockedCoord : DynamicBuilding.BlockedCoords)
	{
		const FGridCell* Cell = Grid.GetCell(BlockedCoord);
		TestNotNull(TEXT("Blocked cell must exist"), Cell);
		if (Cell)
		{
			TestEqual(TEXT("Cell must be Blocked"), Cell->CellType, EGridCellType::Blocked);
			TestFalse(TEXT("Blocked cell must not be walkable"), Cell->IsWalkable());
			TestEqual(TEXT("Building ID must match"), Cell->BuildingId, 3);
		}
	}

	// Verify entrance is walkable
	const FGridCell* EntryCell = Grid.GetCell(DynamicBuilding.EntryCoord);
	TestNotNull(TEXT("Entry cell must exist"), EntryCell);
	if (EntryCell)
	{
		TestEqual(TEXT("Entry cell must be BuildingEntry"), EntryCell->CellType, EGridCellType::BuildingEntry);
		TestTrue(TEXT("Entry cell must be walkable"), EntryCell->IsWalkable());
	}

	return true;
}

// 2. Dynamic Path Rerouting Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassPart3DynamicReroutingTest, "MassDemo.Part3.DynamicRerouting", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassPart3DynamicReroutingTest::RunTest(const FString& Parameters)
{
	FWorldGridData Grid;
	Grid.Initialize(10, 10, 100.0f);

	const FGridCoord Start(0, 0);
	const FGridCoord Goal(4, 0);

	TArray<FGridCoord> InitialPath;
	const bool bInitialFound = UMassGridPathfinderLibrary::FindPath(Grid, Start, Goal, InitialPath);
	TestTrue(TEXT("Initial path should be found"), bInitialFound);
	TestTrue(TEXT("Initial path should be valid"), UMassGridPathfinderLibrary::IsPathValid(Grid, InitialPath));

	// Simulate dynamic building placement that blocks cell (2, 0)
	if (FGridCell* Cell = Grid.GetCell(2, 0))
	{
		Cell->CellType = EGridCellType::Blocked;
	}

	// Initial path must now be invalid
	TestFalse(TEXT("Initial path must now be invalid due to blockage"), UMassGridPathfinderLibrary::IsPathValid(Grid, InitialPath));

	// Re-route from current entity position (1, 0) to goal (4, 0)
	TArray<FGridCoord> ReroutedPath;
	const bool bRerouteFound = UMassGridPathfinderLibrary::FindPathWithAvoidance(Grid, FGridCoord(1, 0), Goal, ReroutedPath);
	TestTrue(TEXT("Rerouted path should be successfully found"), bRerouteFound);
	TestTrue(TEXT("Rerouted path must be valid"), UMassGridPathfinderLibrary::IsPathValid(Grid, ReroutedPath));
	TestFalse(TEXT("Rerouted path must not contain blocked cell (2, 0)"), ReroutedPath.Contains(FGridCoord(2, 0)));

	return true;
}

// 3. Unit Occupancy and Avoidance Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassPart3UnitOccupancyAndAvoidanceTest, "MassDemo.Part3.UnitOccupancyAndAvoidance", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassPart3UnitOccupancyAndAvoidanceTest::RunTest(const FString& Parameters)
{
	FWorldGridData Grid;
	Grid.Initialize(5, 5, 100.0f);

	const FGridCoord CenterCoord(2, 2);
	TestEqual(TEXT("Initial occupancy should be 0"), Grid.GetOccupancy(CenterCoord), 0);

	// Test occupancy increment and decrement
	TestTrue(TEXT("Increment occupancy should succeed"), Grid.TryIncrementOccupancy(CenterCoord, 2));
	TestEqual(TEXT("Occupancy should be 1"), Grid.GetOccupancy(CenterCoord), 1);

	TestTrue(TEXT("Second increment occupancy should succeed"), Grid.TryIncrementOccupancy(CenterCoord, 2));
	TestEqual(TEXT("Occupancy should be 2"), Grid.GetOccupancy(CenterCoord), 2);

	TestFalse(TEXT("Third increment exceeding MaxOccupancy should fail"), Grid.TryIncrementOccupancy(CenterCoord, 2));
	TestEqual(TEXT("Occupancy should remain 2"), Grid.GetOccupancy(CenterCoord), 2);

	Grid.DecrementOccupancy(CenterCoord);
	TestEqual(TEXT("Occupancy after decrement should be 1"), Grid.GetOccupancy(CenterCoord), 1);

	// Test Avoidance Pathfinding: heavily occupy cell (1, 0) on straight line from (0,0) to (2,0)
	if (FGridCell* BusyCell = Grid.GetCell(1, 0))
	{
		BusyCell->OccupancyCount = 5;
	}

	TArray<FGridCoord> AvoidancePath;
	const bool bAvoidFound = UMassGridPathfinderLibrary::FindPathWithAvoidance(Grid, FGridCoord(0, 0), FGridCoord(2, 0), AvoidancePath, 3.0f, 4);
	TestTrue(TEXT("Avoidance path should be found"), bAvoidFound);
	TestFalse(TEXT("Path should detour around heavily occupied cell (1, 0)"), AvoidancePath.Contains(FGridCoord(1, 0)));

	return true;
}

// 4. Building Entry Interaction Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassPart3BuildingEntryInteractionTest, "MassDemo.Part3.BuildingEntryInteraction", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassPart3BuildingEntryInteractionTest::RunTest(const FString& Parameters)
{
	FMassUnitBehaviorFragment BehaviorFrag;
	FMassUnitInteractionFragment InteractionFrag;

	TestEqual(TEXT("Initial behavior state should be Idle"), BehaviorFrag.State, EUnitBehaviorState::Idle);
	TestFalse(TEXT("Initial interaction state should be inactive"), InteractionFrag.IsInteracting());
	TestEqual(TEXT("Initial cargo count should be 0"), InteractionFrag.CargoCount, 0);

	// 1. Transition to Interacting at a building entry
	BehaviorFrag.State = EUnitBehaviorState::Interacting;
	InteractionFrag.StartInteraction(2.0f, 10);

	TestTrue(TEXT("Unit should now be interacting"), InteractionFrag.IsInteracting());
	TestEqual(TEXT("Interaction duration should match"), InteractionFrag.InteractionTimeRemaining, 2.0f);
	TestEqual(TEXT("Target building ID should match"), InteractionFrag.InteractingBuildingId, 10);
	TestEqual(TEXT("Behavior target building ID should be assignable"), BehaviorFrag.TargetBuildingId, INDEX_NONE);
	BehaviorFrag.TargetBuildingId = 10;
	TestEqual(TEXT("Behavior target building ID should match entry target"), BehaviorFrag.TargetBuildingId, 10);

	// 2. Step interaction halfway
	const bool bMidStep = InteractionFrag.StepInteraction(1.0f);
	TestFalse(TEXT("Interaction should not yet be complete after 1.0s"), bMidStep);
	TestEqual(TEXT("Remaining time should be 1.0s"), InteractionFrag.InteractionTimeRemaining, 1.0f);
	TestEqual(TEXT("Cargo count should still be 0"), InteractionFrag.CargoCount, 0);

	// 3. Step interaction to completion
	const bool bCompleteStep = InteractionFrag.StepInteraction(1.0f);
	TestTrue(TEXT("Interaction should be complete after 2.0s"), bCompleteStep);
	TestFalse(TEXT("Unit should no longer be interacting"), InteractionFrag.IsInteracting());
	TestEqual(TEXT("Cargo count should now be 1"), InteractionFrag.CargoCount, 1);
	TestEqual(TEXT("Total interactions should now be 1"), InteractionFrag.TotalInteractions, 1);

	// 4. Reset behavior back to Idle
	BehaviorFrag.ResetIdleTimer();
	TestEqual(TEXT("Behavior state should be reset to Idle"), BehaviorFrag.State, EUnitBehaviorState::Idle);

	return true;
}

// 5. Density and Scale Performance Benchmark Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassPart3DensityBenchmarkTest, "MassDemo.Part3.DensityAndScaleBenchmark", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassPart3DensityBenchmarkTest::RunTest(const FString& Parameters)
{
	// Benchmark Scenario A: 20 units on 10x15 grid (Standard Density)
	FWorldGridData SmallGrid;
	SmallGrid.Initialize(10, 15, 100.0f);
	TArray<FPlacedBuilding> SmallBuildings;
	UMassGridBuildingLibrary::PopulateGridWithRandomBuildings(SmallGrid, 3, SmallBuildings);

	const double StartTimeA = FPlatformTime::Seconds();
	int32 SuccessfulPathsA = 0;
	const int32 UnitsA = 20;

	for (int32 i = 0; i < UnitsA; ++i)
	{
		FGridCoord StartCoord, GoalCoord;
		if (SmallGrid.GetRandomWalkableCell(StartCoord) && SmallGrid.GetRandomWalkableCell(GoalCoord))
		{
			TArray<FGridCoord> Path;
			if (UMassGridPathfinderLibrary::FindPathWithAvoidance(SmallGrid, StartCoord, GoalCoord, Path))
			{
				SuccessfulPathsA++;
			}
		}
	}
	const double DurationA = (FPlatformTime::Seconds() - StartTimeA) * 1000.0; // ms

	TestTrue(TEXT("Scenario A: Most paths should succeed"), SuccessfulPathsA >= UnitsA * 0.8);
	UE_LOG(LogTemp, Display, TEXT("[BENCHMARK] Scenario A (20 units on 10x15 grid): %d/%d paths calculated in %.3f ms"),
		SuccessfulPathsA, UnitsA, DurationA);

	// Benchmark Scenario B: 100 units on 25x25 grid (High Scale Density)
	FWorldGridData LargeGrid;
	LargeGrid.Initialize(25, 25, 100.0f);
	TArray<FPlacedBuilding> LargeBuildings;
	UMassGridBuildingLibrary::PopulateGridWithRandomBuildings(LargeGrid, 12, LargeBuildings);

	const double StartTimeB = FPlatformTime::Seconds();
	int32 SuccessfulPathsB = 0;
	const int32 UnitsB = 100;

	for (int32 i = 0; i < UnitsB; ++i)
	{
		FGridCoord StartCoord, GoalCoord;
		if (LargeGrid.GetRandomWalkableCell(StartCoord) && LargeGrid.GetRandomWalkableCell(GoalCoord))
		{
			TArray<FGridCoord> Path;
			if (UMassGridPathfinderLibrary::FindPathWithAvoidance(LargeGrid, StartCoord, GoalCoord, Path))
			{
				SuccessfulPathsB++;
			}
		}
	}
	const double DurationB = (FPlatformTime::Seconds() - StartTimeB) * 1000.0; // ms

	TestTrue(TEXT("Scenario B: Most paths should succeed"), SuccessfulPathsB >= UnitsB * 0.8);
	UE_LOG(LogTemp, Display, TEXT("[BENCHMARK] Scenario B (100 units on 25x25 grid): %d/%d paths calculated in %.3f ms"),
		SuccessfulPathsB, UnitsB, DurationB);

	return true;
}

#endif
