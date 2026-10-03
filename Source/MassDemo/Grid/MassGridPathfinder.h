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

	/** Converts grid coordinates path to actor local 3D positions */
	UFUNCTION(BlueprintCallable, Category = "MassGrid|Pathfinding")
	static void ConvertPathToLocalPositions(
		const FWorldGridData& Grid, 
		const TArray<FGridCoord>& Path, 
		TArray<FVector>& OutLocalPositions, 
		float ZOffset = 0.0f);
};
