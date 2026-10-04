// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MassGridTypes.h"
#include "MassGridBuilding.h"
#include "WorldGridComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldGridInitialized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldGridBuildingAdded, const FPlacedBuilding&, NewBuilding);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MASSDEMO_API UWorldGridComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWorldGridComponent();

	/** Broadcast when grid is initialized or re-initialized */
	UPROPERTY(BlueprintAssignable, Category = "MassGrid|Events")
	FOnWorldGridInitialized OnGridInitialized;

	/** Broadcast when a new building is dynamically added to the grid */
	UPROPERTY(BlueprintAssignable, Category = "MassGrid|Events")
	FOnWorldGridBuildingAdded OnBuildingAdded;

	/** Grid width in cells (randomized or configured, e.g. 15) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 GridWidth = 15;

	/** Grid height in cells (randomized or configured, e.g. 10) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 GridHeight = 10;

	/** Cell size in Unreal units (e.g. 100 cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	float CellSize = 100.0f;

	/** Number of initial Tetris buildings to place */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 InitialBuildingCount = 4;

	/** Whether this component initializes itself on BeginPlay; actors can disable this and initialize explicitly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bAutoInitializeOnBeginPlay = false;

	/** Automatically randomize grid dimensions near 10x15 on start if true */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bRandomizeDimensionsOnStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FIntPoint MinRandomDimension = FIntPoint(10, 10);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FIntPoint MaxRandomDimension = FIntPoint(16, 16);

	/** Z offset of the grid plane relative to the owning actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	float GridZOffset = 50.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** Re-initializes the grid with the specified parameters and generates buildings */
	UFUNCTION(BlueprintCallable, Category = "MassGrid")
	void InitializeGrid(int32 InWidth, int32 InHeight, float InCellSize, int32 InBuildingCount);

	/** Spawns a new random building on the grid dynamically at runtime */
	UFUNCTION(BlueprintCallable, Category = "MassGrid")
	bool SpawnAdditionalBuilding(FPlacedBuilding& OutBuilding);

	UFUNCTION(BlueprintPure, Category = "MassGrid")
	int32 GetGridId() const { return GridId; }

	FWorldGridData& GetGridData() { return GridData; }
	const FWorldGridData& GetGridData() const { return GridData; }

	const TArray<FPlacedBuilding>& GetPlacedBuildings() const { return PlacedBuildings; }

	/** Converts grid coordinate to local position relative to the owning actor */
	UFUNCTION(BlueprintPure, Category = "MassGrid")
	FVector CellToLocalPosition(const FGridCoord& Coord) const;

	/** Converts grid coordinate to world position */
	UFUNCTION(BlueprintPure, Category = "MassGrid")
	FVector CellToWorldPosition(const FGridCoord& Coord) const;

	/** Converts actor local position to grid coordinate */
	UFUNCTION(BlueprintPure, Category = "MassGrid")
	FGridCoord LocalPositionToCell(const FVector& LocalPos) const;

	/** Converts world position to grid coordinate */
	UFUNCTION(BlueprintPure, Category = "MassGrid")
	FGridCoord WorldPositionToCell(const FVector& WorldPos) const;

	/** Finds path between two grid coords in this grid */
	UFUNCTION(BlueprintCallable, Category = "MassGrid")
	bool FindPath(const FGridCoord& Start, const FGridCoord& Goal, TArray<FGridCoord>& OutPath) const;

	/** Finds path and returns list of actor local positions */
	UFUNCTION(BlueprintCallable, Category = "MassGrid")
	bool FindPathLocalPositions(const FGridCoord& Start, const FGridCoord& Goal, TArray<FVector>& OutLocalPositions) const;

	/** Gets a random walkable cell on this grid */
	UFUNCTION(BlueprintPure, Category = "MassGrid")
	bool GetRandomWalkableCell(FGridCoord& OutCoord) const;

	/** Gets all cube block transforms for all placed buildings on this grid */
	UFUNCTION(BlueprintPure, Category = "MassGrid")
	void GetAllBuildingBlockTransforms(
		TArray<FTransform>& OutBlockTransforms, 
		float InBuildingHeight = 100.0f, 
		float InBlockScale = 0.95f) const;

	/** Gets all cube entry transforms for all placed buildings on this grid */
	UFUNCTION(BlueprintPure, Category = "MassGrid")
	void GetAllBuildingEntryTransforms(
		TArray<FTransform>& OutEntryTransforms, 
		float InEntryHeight = 20.0f, 
		float InEntryScale = 0.8f) const;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	int32 GridId = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	FWorldGridData GridData;

	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	TArray<FPlacedBuilding> PlacedBuildings;
};
