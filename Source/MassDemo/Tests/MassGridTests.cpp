// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Grid/MassGridTypes.h"
#include "Grid/MassGridBuilding.h"
#include "Grid/MassGridPathfinder.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassGridInitializationTest, "MassDemo.Grid.Initialization", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassGridInitializationTest::RunTest(const FString& Parameters)
{
	FWorldGridData Grid;
	Grid.Initialize(15, 10, 100.0f);

	TestEqual(TEXT("Grid width should match initialization"), Grid.Width, 15);
	TestEqual(TEXT("Grid height should match initialization"), Grid.Height, 10);
	TestEqual(TEXT("Grid cell count should be Width * Height"), Grid.Cells.Num(), 150);

	TestTrue(TEXT("Origin cell (0,0) must be valid"), Grid.IsValidCoord(0, 0));
	TestTrue(TEXT("Corner cell (14,9) must be valid"), Grid.IsValidCoord(14, 9));
	TestFalse(TEXT("Out of bounds cell (15, 10) must be invalid"), Grid.IsValidCoord(15, 10));
	TestFalse(TEXT("Negative cell (-1, 0) must be invalid"), Grid.IsValidCoord(-1, 0));

	const FGridCell* Cell = Grid.GetCell(5, 5);
	TestNotNull(TEXT("Cell (5,5) should not be null"), Cell);
	if (Cell)
	{
		TestTrue(TEXT("Default cell should be walkable"), Cell->IsWalkable());
		TestEqual(TEXT("Default cell should have Free type"), Cell->CellType, EGridCellType::Free);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassGridBuildingPlacementTest, "MassDemo.Grid.BuildingPlacement", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassGridBuildingPlacementTest::RunTest(const FString& Parameters)
{
	FWorldGridData Grid;
	Grid.Initialize(15, 10, 100.0f);

	FTetrisBuildingShape LShape = UMassGridBuildingLibrary::GetShapeTemplate(ETetrisShapeType::LShape2x3);
	FPlacedBuilding Placed;

	const bool bSuccess = UMassGridBuildingLibrary::TryPlaceBuilding(Grid, LShape, FGridCoord(2, 2), 1, Placed);
	TestTrue(TEXT("LShape should be successfully placed at (2,2)"), bSuccess);

	// Test blocked coords
	for (const FGridCoord& BlockedCoord : Placed.BlockedCoords)
	{
		const FGridCell* BlockedCell = Grid.GetCell(BlockedCoord);
		TestNotNull(TEXT("Blocked cell should exist"), BlockedCell);
		if (BlockedCell)
		{
			TestEqual(TEXT("Blocked cell must have Blocked type"), BlockedCell->CellType, EGridCellType::Blocked);
			TestFalse(TEXT("Blocked cell must not be walkable"), BlockedCell->IsWalkable());
			TestEqual(TEXT("Blocked cell BuildingId must match"), BlockedCell->BuildingId, 1);
		}
	}

	// Test entry coord
	const FGridCell* EntryCell = Grid.GetCell(Placed.EntryCoord);
	TestNotNull(TEXT("Entry cell should exist"), EntryCell);
	if (EntryCell)
	{
		TestEqual(TEXT("Entry cell must have BuildingEntry type"), EntryCell->CellType, EGridCellType::BuildingEntry);
		TestTrue(TEXT("Entry cell must be walkable"), EntryCell->IsWalkable());
		TestEqual(TEXT("Entry cell BuildingId must match"), EntryCell->BuildingId, 1);
	}

	// Try placing another building overlapping with the previous one - should fail
	FPlacedBuilding OverlappingBuilding;
	const bool bOverlap = UMassGridBuildingLibrary::TryPlaceBuilding(Grid, LShape, FGridCoord(2, 2), 2, OverlappingBuilding);
	TestFalse(TEXT("Overlapping building placement must fail"), bOverlap);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMassGridPathfindingTest, "MassDemo.Grid.Pathfinding", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMassGridPathfindingTest::RunTest(const FString& Parameters)
{
	FWorldGridData Grid;
	Grid.Initialize(10, 10, 100.0f);

	// Test straight path on empty grid
	TArray<FGridCoord> Path;
	const bool bPathFound = UMassGridPathfinderLibrary::FindPath(Grid, FGridCoord(0, 0), FGridCoord(3, 0), Path);
	TestTrue(TEXT("Path from (0,0) to (3,0) should be found"), bPathFound);
	TestEqual(TEXT("Path length should be 4 steps"), Path.Num(), 4);

	// Block cell (1, 0)
	if (FGridCell* Cell = Grid.GetCell(1, 0))
	{
		Cell->CellType = EGridCellType::Blocked;
	}

	// Path should now navigate around the obstacle
	TArray<FGridCoord> DetourPath;
	const bool bDetourFound = UMassGridPathfinderLibrary::FindPath(Grid, FGridCoord(0, 0), FGridCoord(2, 0), DetourPath);
	TestTrue(TEXT("Detour path should be found around obstacle"), bDetourFound);
	TestFalse(TEXT("Path must not contain blocked cell (1,0)"), DetourPath.Contains(FGridCoord(1, 0)));

	return true;
}

#endif
