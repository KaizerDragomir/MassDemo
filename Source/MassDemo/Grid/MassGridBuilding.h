// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassGridTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MassGridBuilding.generated.h"

UENUM(BlueprintType)
enum class ETetrisShapeType : uint8
{
	Single1x1 = 0,
	Square2x2,
	Line3x1,
	Line1x3,
	LShape2x3,
	TShape3x2,
	ZShape3x2,
	BigBlock3x3,
	Custom3x4
};

USTRUCT(BlueprintType)
struct MASSDEMO_API FTetrisBuildingShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	ETetrisShapeType ShapeType = ETetrisShapeType::Single1x1;

	// Blocked cells relative to root (0, 0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TArray<FGridCoord> BlockedOffsets;

	// Entry cell relative to root (0, 0) - this cell is walkable and acts as the entry point
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FGridCoord EntryOffset = FGridCoord(0, 0);

	// Bounding size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 SizeX = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 SizeY = 1;
};

USTRUCT(BlueprintType)
struct MASSDEMO_API FPlacedBuilding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 BuildingId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	ETetrisShapeType ShapeType = ETetrisShapeType::Single1x1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FGridCoord RootCoord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FGridCoord EntryCoord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TArray<FGridCoord> BlockedCoords;
};

UCLASS()
class MASSDEMO_API UMassGridBuildingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns predefined standard Tetris-like shape templates with entry points */
	UFUNCTION(BlueprintPure, Category = "MassGrid|Building")
	static FTetrisBuildingShape GetShapeTemplate(ETetrisShapeType ShapeType);

	/** Returns a random shape template */
	UFUNCTION(BlueprintPure, Category = "MassGrid|Building")
	static FTetrisBuildingShape GetRandomShapeTemplate();

	/** Attempts to place a building at the specified root coordinate on the grid */
	UFUNCTION(BlueprintCallable, Category = "MassGrid|Building")
	static bool TryPlaceBuilding(
		UPARAM(ref) FWorldGridData& Grid, 
		const FTetrisBuildingShape& Shape, 
		const FGridCoord& RootCoord, 
		int32 BuildingId, 
		FPlacedBuilding& OutPlacedBuilding);

	/** Randomly populates the grid with a specified number of non-overlapping Tetris buildings */
	UFUNCTION(BlueprintCallable, Category = "MassGrid|Building")
	static int32 PopulateGridWithRandomBuildings(
		UPARAM(ref) FWorldGridData& Grid, 
		int32 NumberOfBuildings, 
		TArray<FPlacedBuilding>& OutPlacedBuildings,
		int32 MaxPlacementAttempts = 50);
};
