// Copyright Epic Games, Inc. All Rights Reserved.

#include "Grid/WorldGridComponent.h"
#include "Grid/MassGridSubsystem.h"
#include "Grid/MassGridPathfinder.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UWorldGridComponent::UWorldGridComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWorldGridComponent::BeginPlay()
{
	Super::BeginPlay();

	int32 ChosenWidth = GridWidth;
	int32 ChosenHeight = GridHeight;

	if (bRandomizeDimensionsOnStart)
	{
		ChosenWidth = FMath::RandRange(MinRandomDimension.X, MaxRandomDimension.X);
		ChosenHeight = FMath::RandRange(MinRandomDimension.Y, MaxRandomDimension.Y);
	}

	InitializeGrid(ChosenWidth, ChosenHeight, CellSize, InitialBuildingCount);

	if (UWorld* World = GetWorld())
	{
		if (UMassGridSubsystem* GridSubsystem = World->GetSubsystem<UMassGridSubsystem>())
		{
			GridId = GridSubsystem->RegisterGrid(this);
		}
	}
}

void UWorldGridComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GridId != INDEX_NONE)
	{
		if (UWorld* World = GetWorld())
		{
			if (UMassGridSubsystem* GridSubsystem = World->GetSubsystem<UMassGridSubsystem>())
			{
				GridSubsystem->UnregisterGrid(GridId);
			}
		}
		GridId = INDEX_NONE;
	}

	Super::EndPlay(EndPlayReason);
}

void UWorldGridComponent::InitializeGrid(int32 InWidth, int32 InHeight, float InCellSize, int32 InBuildingCount)
{
	GridWidth = InWidth;
	GridHeight = InHeight;
	CellSize = InCellSize;

	GridData.Initialize(GridWidth, GridHeight, CellSize);
	PlacedBuildings.Empty();

	if (InBuildingCount > 0)
	{
		UMassGridBuildingLibrary::PopulateGridWithRandomBuildings(GridData, InBuildingCount, PlacedBuildings);
	}

	OnGridInitialized.Broadcast();
}

bool UWorldGridComponent::SpawnAdditionalBuilding(FPlacedBuilding& OutBuilding)
{
	const FTetrisBuildingShape Shape = UMassGridBuildingLibrary::GetRandomShapeTemplate();
	const int32 MaxRootX = GridData.Width - Shape.SizeX;
	const int32 MaxRootY = GridData.Height - Shape.SizeY;

	if (MaxRootX < 0 || MaxRootY < 0)
	{
		return false;
	}

	for (int32 Attempt = 0; Attempt < 30; ++Attempt)
	{
		const FGridCoord RootCoord(FMath::RandRange(0, MaxRootX), FMath::RandRange(0, MaxRootY));
		const int32 NextBuildingId = PlacedBuildings.Num() + 1;
		if (UMassGridBuildingLibrary::TryPlaceBuilding(GridData, Shape, RootCoord, NextBuildingId, OutBuilding))
		{
			PlacedBuildings.Add(OutBuilding);
			OnBuildingAdded.Broadcast(OutBuilding);
			return true;
		}
	}

	return false;
}

FVector UWorldGridComponent::CellToLocalPosition(const FGridCoord& Coord) const
{
	return GridData.CellToLocalPosition(Coord, GridZOffset);
}

FVector UWorldGridComponent::CellToWorldPosition(const FGridCoord& Coord) const
{
	const FVector LocalPos = CellToLocalPosition(Coord);
	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorTransform().TransformPosition(LocalPos);
	}
	return LocalPos;
}

FGridCoord UWorldGridComponent::LocalPositionToCell(const FVector& LocalPos) const
{
	return GridData.LocalPositionToCell(LocalPos);
}

FGridCoord UWorldGridComponent::WorldPositionToCell(const FVector& WorldPos) const
{
	if (const AActor* Owner = GetOwner())
	{
		const FVector LocalPos = Owner->GetActorTransform().InverseTransformPosition(WorldPos);
		return LocalPositionToCell(LocalPos);
	}
	return LocalPositionToCell(WorldPos);
}

bool UWorldGridComponent::FindPath(const FGridCoord& Start, const FGridCoord& Goal, TArray<FGridCoord>& OutPath) const
{
	return UMassGridPathfinderLibrary::FindPath(GridData, Start, Goal, OutPath);
}

bool UWorldGridComponent::FindPathLocalPositions(const FGridCoord& Start, const FGridCoord& Goal, TArray<FVector>& OutLocalPositions) const
{
	TArray<FGridCoord> CoordPath;
	if (FindPath(Start, Goal, CoordPath))
	{
		UMassGridPathfinderLibrary::ConvertPathToLocalPositions(GridData, CoordPath, OutLocalPositions, GridZOffset);
		return true;
	}
	return false;
}

bool UWorldGridComponent::GetRandomWalkableCell(FGridCoord& OutCoord) const
{
	return GridData.GetRandomWalkableCell(OutCoord);
}

void UWorldGridComponent::GetAllBuildingBlockTransforms(
	TArray<FTransform>& OutBlockTransforms, 
	float InBuildingHeight, 
	float InBlockScale) const
{
	OutBlockTransforms.Empty();
	for (const FPlacedBuilding& Building : PlacedBuildings)
	{
		TArray<FTransform> BuildingTransforms = UMassGridBuildingLibrary::CalculateBuildingBlockTransforms(
			GridData, Building, GridZOffset, InBuildingHeight, InBlockScale);
		OutBlockTransforms.Append(BuildingTransforms);
	}
}

void UWorldGridComponent::GetAllBuildingEntryTransforms(
	TArray<FTransform>& OutEntryTransforms, 
	float InEntryHeight, 
	float InEntryScale) const
{
	OutEntryTransforms.Empty(PlacedBuildings.Num());
	for (const FPlacedBuilding& Building : PlacedBuildings)
	{
		FTransform EntryTransform = UMassGridBuildingLibrary::CalculateBuildingEntryTransform(
			GridData, Building, GridZOffset, InEntryHeight, InEntryScale);
		OutEntryTransforms.Add(EntryTransform);
	}
}
