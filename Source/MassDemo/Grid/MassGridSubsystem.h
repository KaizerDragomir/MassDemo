// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassGridTypes.h"
#include "MassGridSubsystem.generated.h"

class UWorldGridComponent;

UCLASS()
class MASSDEMO_API UMassGridSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Registers a grid component with an assigned GridID */
	int32 RegisterGrid(UWorldGridComponent* GridComponent);

	/** Unregisters a grid component */
	void UnregisterGrid(int32 GridId);

	/** Retrieves the grid component by GridID */
	UFUNCTION(BlueprintPure, Category = "MassGrid|Subsystem")
	UWorldGridComponent* GetGridComponent(int32 GridId) const;

	/** Gets a pointer to the FWorldGridData directly by GridID */
	FWorldGridData* GetGridData(int32 GridId);
	const FWorldGridData* GetGridData(int32 GridId) const;

	/** Returns all registered grid IDs */
	UFUNCTION(BlueprintPure, Category = "MassGrid|Subsystem")
	TArray<int32> GetAllGridIds() const;

private:
	int32 NextGridId = 1;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UWorldGridComponent>> RegisteredGrids;
};
