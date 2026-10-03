// Copyright Epic Games, Inc. All Rights Reserved.

#include "Grid/MassGridBuilding.h"

FTetrisBuildingShape UMassGridBuildingLibrary::GetShapeTemplate(ETetrisShapeType ShapeType)
{
	FTetrisBuildingShape Shape;
	Shape.ShapeType = ShapeType;

	switch (ShapeType)
	{
	case ETetrisShapeType::Single1x1:
		Shape.SizeX = 1;
		Shape.SizeY = 1;
		Shape.BlockedOffsets = { FGridCoord(0, 0) };
		Shape.EntryOffset = FGridCoord(0, 0); // single cell acts as entry/interactable
		break;

	case ETetrisShapeType::Square2x2:
		// 2x2: (0,0), (1,0), (0,1) blocked; (1,1) is entry point
		Shape.SizeX = 2;
		Shape.SizeY = 2;
		Shape.BlockedOffsets = { FGridCoord(0, 0), FGridCoord(1, 0), FGridCoord(0, 1) };
		Shape.EntryOffset = FGridCoord(1, 1);
		break;

	case ETetrisShapeType::Line3x1:
		// 3x1 Horizontal: (0,0), (1,0) blocked; (2,0) is entry point
		Shape.SizeX = 3;
		Shape.SizeY = 1;
		Shape.BlockedOffsets = { FGridCoord(0, 0), FGridCoord(1, 0) };
		Shape.EntryOffset = FGridCoord(2, 0);
		break;

	case ETetrisShapeType::Line1x3:
		// 1x3 Vertical: (0,0), (0,1) blocked; (0,2) is entry point
		Shape.SizeX = 1;
		Shape.SizeY = 3;
		Shape.BlockedOffsets = { FGridCoord(0, 0), FGridCoord(0, 1) };
		Shape.EntryOffset = FGridCoord(0, 2);
		break;

	case ETetrisShapeType::LShape2x3:
		// L-Shape 2x3: (0,0), (0,1), (0,2), (1,0) blocked; (1,2) is entry point
		Shape.SizeX = 2;
		Shape.SizeY = 3;
		Shape.BlockedOffsets = { FGridCoord(0, 0), FGridCoord(0, 1), FGridCoord(0, 2), FGridCoord(1, 0) };
		Shape.EntryOffset = FGridCoord(1, 2);
		break;

	case ETetrisShapeType::TShape3x2:
		// T-Shape 3x2: (0,0), (1,0), (2,0), (1,1) blocked; (0,1) is entry point
		Shape.SizeX = 3;
		Shape.SizeY = 2;
		Shape.BlockedOffsets = { FGridCoord(0, 0), FGridCoord(1, 0), FGridCoord(2, 0), FGridCoord(1, 1) };
		Shape.EntryOffset = FGridCoord(0, 1);
		break;

	case ETetrisShapeType::ZShape3x2:
		// Z-Shape 3x2: (0,0), (1,0), (1,1), (2,1) blocked; (0,1) is entry point
		Shape.SizeX = 3;
		Shape.SizeY = 2;
		Shape.BlockedOffsets = { FGridCoord(0, 0), FGridCoord(1, 0), FGridCoord(1, 1), FGridCoord(2, 1) };
		Shape.EntryOffset = FGridCoord(0, 1);
		break;

	case ETetrisShapeType::BigBlock3x3:
		// 3x3 Block with central entrance: 8 perimeter cells blocked, center (1,1) entry
		Shape.SizeX = 3;
		Shape.SizeY = 3;
		Shape.BlockedOffsets = {
			FGridCoord(0, 0), FGridCoord(1, 0), FGridCoord(2, 0),
			FGridCoord(0, 1),                   FGridCoord(2, 1),
			FGridCoord(0, 2), FGridCoord(1, 2), FGridCoord(2, 2)
		};
		Shape.EntryOffset = FGridCoord(1, 1);
		break;

	case ETetrisShapeType::Custom3x4:
		// 3x4 Complex Building: 
		Shape.SizeX = 3;
		Shape.SizeY = 4;
		Shape.BlockedOffsets = {
			FGridCoord(0, 0), FGridCoord(1, 0), FGridCoord(2, 0),
			FGridCoord(0, 1),                   FGridCoord(2, 1),
			FGridCoord(0, 2),                   FGridCoord(2, 2),
			FGridCoord(0, 3), FGridCoord(1, 3)
		};
		Shape.EntryOffset = FGridCoord(1, 1);
		break;

	default:
		Shape.SizeX = 1;
		Shape.SizeY = 1;
		Shape.BlockedOffsets = { FGridCoord(0, 0) };
		Shape.EntryOffset = FGridCoord(0, 0);
		break;
	}

	return Shape;
}

FTetrisBuildingShape UMassGridBuildingLibrary::GetRandomShapeTemplate()
{
	const int32 NumTypes = static_cast<int32>(ETetrisShapeType::Custom3x4) + 1;
	const ETetrisShapeType RandomType = static_cast<ETetrisShapeType>(FMath::RandRange(0, NumTypes - 1));
	return GetShapeTemplate(RandomType);
}

bool UMassGridBuildingLibrary::TryPlaceBuilding(
	FWorldGridData& Grid, 
	const FTetrisBuildingShape& Shape, 
	const FGridCoord& RootCoord, 
	int32 BuildingId, 
	FPlacedBuilding& OutPlacedBuilding)
{
	// 1. Verify bounds and non-overlap for all blocked cells
	TArray<FGridCoord> AbsoluteBlocked;
	AbsoluteBlocked.Reserve(Shape.BlockedOffsets.Num());

	for (const FGridCoord& Offset : Shape.BlockedOffsets)
	{
		const FGridCoord Coord(RootCoord.X + Offset.X, RootCoord.Y + Offset.Y);
		if (!Grid.IsValidCoord(Coord))
		{
			return false;
		}

		const FGridCell* Cell = Grid.GetCell(Coord);
		if (!Cell || Cell->CellType != EGridCellType::Free)
		{
			return false; // Cell is already occupied or blocked
		}

		AbsoluteBlocked.Add(Coord);
	}

	// 2. Verify entry cell
	const FGridCoord AbsoluteEntry(RootCoord.X + Shape.EntryOffset.X, RootCoord.Y + Shape.EntryOffset.Y);
	if (!Grid.IsValidCoord(AbsoluteEntry))
	{
		return false;
	}

	const FGridCell* EntryCell = Grid.GetCell(AbsoluteEntry);
	if (!EntryCell || EntryCell->CellType != EGridCellType::Free)
	{
		return false;
	}

	// 3. Apply changes to grid
	for (const FGridCoord& Coord : AbsoluteBlocked)
	{
		FGridCell* Cell = Grid.GetCell(Coord);
		if (Cell)
		{
			Cell->CellType = EGridCellType::Blocked;
			Cell->BuildingId = BuildingId;
		}
	}

	FGridCell* EntryGridCell = Grid.GetCell(AbsoluteEntry);
	if (EntryGridCell)
	{
		EntryGridCell->CellType = EGridCellType::BuildingEntry;
		EntryGridCell->BuildingId = BuildingId;
	}

	OutPlacedBuilding.BuildingId = BuildingId;
	OutPlacedBuilding.ShapeType = Shape.ShapeType;
	OutPlacedBuilding.RootCoord = RootCoord;
	OutPlacedBuilding.EntryCoord = AbsoluteEntry;
	OutPlacedBuilding.BlockedCoords = AbsoluteBlocked;

	return true;
}

int32 UMassGridBuildingLibrary::PopulateGridWithRandomBuildings(
	FWorldGridData& Grid, 
	int32 NumberOfBuildings, 
	TArray<FPlacedBuilding>& OutPlacedBuildings,
	int32 MaxPlacementAttempts)
{
	OutPlacedBuildings.Empty(NumberOfBuildings);
	int32 PlacedCount = 0;

	for (int32 i = 0; i < NumberOfBuildings; ++i)
	{
		bool bPlaced = false;
		for (int32 Attempt = 0; Attempt < MaxPlacementAttempts; ++Attempt)
		{
			const FTetrisBuildingShape Shape = GetRandomShapeTemplate();
			
			// Choose a valid random root within potential range
			const int32 MaxRootX = Grid.Width - Shape.SizeX;
			const int32 MaxRootY = Grid.Height - Shape.SizeY;

			if (MaxRootX < 0 || MaxRootY < 0)
			{
				continue;
			}

			const FGridCoord RootCoord(FMath::RandRange(0, MaxRootX), FMath::RandRange(0, MaxRootY));
			FPlacedBuilding PlacedBuilding;
			if (TryPlaceBuilding(Grid, Shape, RootCoord, PlacedCount + 1, PlacedBuilding))
			{
				OutPlacedBuildings.Add(PlacedBuilding);
				PlacedCount++;
				bPlaced = true;
				break;
			}
		}
	}

	return PlacedCount;
}
