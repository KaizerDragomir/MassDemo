// Copyright Epic Games, Inc. All Rights Reserved.

#include "Grid/MassGridSubsystem.h"
#include "Grid/WorldGridComponent.h"

void UMassGridSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	NextGridId = 1;
	RegisteredGrids.Empty();
}

void UMassGridSubsystem::Deinitialize()
{
	RegisteredGrids.Empty();
	Super::Deinitialize();
}

int32 UMassGridSubsystem::RegisterGrid(UWorldGridComponent* GridComponent)
{
	if (!GridComponent)
	{
		return INDEX_NONE;
	}

	const int32 AssignedId = NextGridId++;
	RegisteredGrids.Add(AssignedId, GridComponent);
	return AssignedId;
}

void UMassGridSubsystem::UnregisterGrid(int32 GridId)
{
	RegisteredGrids.Remove(GridId);
}

UWorldGridComponent* UMassGridSubsystem::GetGridComponent(int32 GridId) const
{
	const TObjectPtr<UWorldGridComponent>* Found = RegisteredGrids.Find(GridId);
	return Found ? Found->Get() : nullptr;
}

FWorldGridData* UMassGridSubsystem::GetGridData(int32 GridId)
{
	UWorldGridComponent* Comp = GetGridComponent(GridId);
	return Comp ? &Comp->GetGridData() : nullptr;
}

const FWorldGridData* UMassGridSubsystem::GetGridData(int32 GridId) const
{
	const UWorldGridComponent* Comp = GetGridComponent(GridId);
	return Comp ? &Comp->GetGridData() : nullptr;
}

TArray<int32> UMassGridSubsystem::GetAllGridIds() const
{
	TArray<int32> OutIds;
	RegisteredGrids.GetKeys(OutIds);
	return OutIds;
}
