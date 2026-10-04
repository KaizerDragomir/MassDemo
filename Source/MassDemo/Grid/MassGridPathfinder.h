// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassGridTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MassGridPathfinder.generated.h"

UCLASS()
class MASSDEMO_API UMassGridPathfinderLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Finds a 4-directional shortest path on the local grid using A* */
	UFUNCTION(BlueprintCallable, Category = "MassGrid|Pathfinding")
	static bool FindPath(
		const FWorldGridData& Grid, 
		const FGridCoord& Start, 
		const FGridCoord& Goal, 
		TArray<FGridCoord>& OutPath);

	/** Finds a 4-directional path penalizing or avoiding heavily occupied cells */
	UFUNCTION(BlueprintCallable, Category = "MassGrid|Pathfinding")
	static bool FindPathWithAvoidance(
		const FWorldGridData& Grid, 
		const FGridCoord& Start, 
		const FGridCoord& Goal, 
		TArray<FGridCoord>& OutPath,
		float OccupancyWeight = 2.0f,
		int32 MaxCellOccupancy = 4);

	/** Checks if all coordinates in a path are currently valid and walkable */
	UFUNCTION(BlueprintPure, Category = "MassGrid|Pathfinding")
	static bool IsPathValid(const FWorldGridData& Grid, const TArray<FGridCoord>& Path);

	/** Converts grid coordinates path to actor local 3D positions */
	UFUNCTION(BlueprintCallable, Category = "MassGrid|Pathfinding")
	static void ConvertPathToLocalPositions(
		const FWorldGridData& Grid, 
		const TArray<FGridCoord>& Path, 
		TArray<FVector>& OutLocalPositions, 
		float ZOffset = 0.0f);
};
