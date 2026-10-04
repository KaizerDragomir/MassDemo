// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassEntityTypes.h"
#include "MassGridTypes.h"
#include "WanderingWorldActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UWorldGridComponent;
class UMaterialInterface;
struct FPlacedBuilding;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWorldCollisionEvent, AWanderingWorldActor*, ThisWorld, AWanderingWorldActor*, OtherWorld);

/**
 * WanderingWorldActor represents an independent floating world/cube in 3D space.
 * It owns a local 2D grid, procedural obstacle buildings, and hosts wandering Mass entities.
 */
UCLASS()
class MASSDEMO_API AWanderingWorldActor : public AActor
{
	GENERATED_BODY()

public:
	AWanderingWorldActor();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

public:
	/** Root Scene Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Base visual mesh (platform / cube) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMeshComponent;

	/** Building visual instances (obstacle cubes) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> BuildingVisualComponent;

	/** Entry visual instances (walkable entrance cubes/markers) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> EntryVisualComponent;

	/** Unit visual instances (sphere primitives) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> UnitVisualComponent;

	/** Collision Box for detecting world-to-world impacts */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBoxComponent;

	/** Local World Grid Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWorldGridComponent> WorldGridComponent;

	// --- Visual Settings ---

	/** Static mesh used for building blocks (default Engine Cube) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	TObjectPtr<UStaticMesh> BuildingMesh;

	/** Static mesh used for building entries (default Engine Cube) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	TObjectPtr<UStaticMesh> EntryMesh;

	/** Material override for building blocks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	TObjectPtr<UMaterialInterface> BuildingMaterial;

	/** Material override for building entries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	TObjectPtr<UMaterialInterface> EntryMaterial;

	/** Material override for the base floating platform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	TObjectPtr<UMaterialInterface> BaseMeshMaterial;

	/** Static mesh used for units (default Engine Sphere) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	TObjectPtr<UStaticMesh> UnitMesh;

	/** Material override for normal units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	TObjectPtr<UMaterialInterface> UnitMaterial;

	/** Material override for stunned units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	TObjectPtr<UMaterialInterface> StunnedUnitMaterial;

	/** Unit visual sphere scale relative to standard mesh size (default 0.35 gives 35cm diameter sphere) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	float UnitScale = 0.35f;

	/** Height offset for unit spheres above the grid floor in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	float UnitZOffset = 25.0f;

	/** Whether to automatically visualize units with sphere primitives */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	bool bVisualizeUnits = true;

	/** Height of building obstacle cubes in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	float BuildingHeight = 100.0f;

	/** Height of building entry markers in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	float EntryHeight = 20.0f;

	/** Block scale factor relative to CellSize (e.g. 0.95 gives subtle grid gaps) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	float BuildingBlockScale = 0.95f;

	/** Entry scale factor relative to CellSize */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	float EntryBlockScale = 0.8f;

	/** Whether to automatically visualize building obstacles with cube geometry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	bool bVisualizeBuildings = true;

	/** Whether to visualize building entry points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Visual")
	bool bVisualizeEntries = true;

	// --- Movement Settings ---

	/** Whether this world drifts/moves in 3D space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Movement")
	bool bEnableMovement = true;

	/** Linear drifting velocity in cm/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Movement")
	FVector MovementVelocity = FVector(100.0f, 0.0f, 0.0f);

	/** Movement bounding box min (for bounce behavior) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Movement")
	FVector MovementBoundsMin = FVector(-3000.0f, -3000.0f, 0.0f);

	/** Movement bounding box max (for bounce behavior) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Movement")
	FVector MovementBoundsMax = FVector(3000.0f, 3000.0f, 1000.0f);

	// --- Initialization & Spawning Settings ---

	/** Number of initial units to spawn on this world */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Spawning")
	int32 InitialEntityCount = 15;

	/** Whether to automatically spawn Mass entities on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Spawning")
	bool bAutoSpawnEntitiesOnBeginPlay = true;

	/** Whether to automatically populate buildings on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Spawning")
	bool bAutoPopulateBuildings = true;

	/** Number of obstacle buildings to generate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Spawning")
	int32 InitialBuildingCount = 4;

	/** Grid width in cells */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Grid")
	int32 GridWidth = 10;

	/** Grid height in cells */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Grid")
	int32 GridHeight = 15;

	/** Grid cell size in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|Grid")
	float GridCellSize = 100.0f;

	// --- Dynamic Building Spawning Settings ---

	/** Whether to dynamically spawn additional obstacle buildings over time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|DynamicBuildings")
	bool bEnableDynamicBuildingSpawning = false;

	/** Interval in seconds between dynamic building spawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|DynamicBuildings")
	float DynamicBuildingSpawnInterval = 10.0f;

	/** Maximum total buildings allowed when dynamically spawning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWorld|DynamicBuildings")
	int32 MaxDynamicBuildings = 8;

	/** Attempts to spawn a new building at runtime and update visual instances */
	UFUNCTION(BlueprintCallable, Category = "MassWorld|DynamicBuildings")
	bool SpawnRuntimeBuilding(FPlacedBuilding& OutBuilding);

	// --- Collision Notification ---

	UPROPERTY(BlueprintAssignable, Category = "MassWorld|Events")
	FOnWorldCollisionEvent OnWorldCollision;

	/** Manually triggers collision stun on all entities belonging to this world */
	UFUNCTION(BlueprintCallable, Category = "MassWorld")
	void StunAllEntities(float StunDuration = 2.0f);

	/** Spawns Mass entities on this world's grid */
	UFUNCTION(BlueprintCallable, Category = "MassWorld")
	void SpawnGridEntities(int32 Count);

	/** Gets the unique GridID managed by this actor's WorldGridComponent */
	UFUNCTION(BlueprintPure, Category = "MassWorld")
	int32 GetGridId() const;

	/** Handles collision impact with another WanderingWorldActor */
	UFUNCTION(BlueprintCallable, Category = "MassWorld")
	void HandleWorldCollision(AWanderingWorldActor* OtherWorld);

	/** Handles overlap begin on the collision box */
	UFUNCTION()
	void OnCollisionBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// --- Visual Building Methods ---

	/** Rebuilds all building cube visual instances from the WorldGridComponent placed buildings */
	UFUNCTION(BlueprintCallable, Category = "MassWorld|Visual")
	void RebuildBuildingVisuals();

	/** Adds cube instances for a single placed building */
	UFUNCTION(BlueprintCallable, Category = "MassWorld|Visual")
	void AddBuildingVisual(const FPlacedBuilding& Building);

	/** Clears all building cube visual instances */
	UFUNCTION(BlueprintCallable, Category = "MassWorld|Visual")
	void ClearBuildingVisuals();

	/** Rebuilds all unit sphere visual instances */
	UFUNCTION(BlueprintCallable, Category = "MassWorld|Visual")
	void RebuildUnitVisuals();

	/** Updates unit sphere instance transforms from current Mass entity locations */
	UFUNCTION(BlueprintCallable, Category = "MassWorld|Visual")
	void UpdateUnitVisuals();

	/** Clears all unit sphere visual instances */
	UFUNCTION(BlueprintCallable, Category = "MassWorld|Visual")
	void ClearUnitVisuals();

	/** Updates the base platform mesh scale and position to match current grid dimensions */
	UFUNCTION(BlueprintCallable, Category = "MassWorld|Visual")
	void UpdateBaseMeshVisual();

	/** Callback when WorldGridComponent completes grid initialization */
	UFUNCTION()
	void HandleGridInitialized();

	/** Callback when a new building is dynamically placed on WorldGridComponent */
	UFUNCTION()
	void HandleBuildingAdded(const FPlacedBuilding& NewBuilding);

	/** Gets all entity handles belonging to this world */
	const TArray<FMassEntityHandle>& GetSpawnedEntities() const { return SpawnedEntities; }

private:
	UPROPERTY()
	TArray<FMassEntityHandle> SpawnedEntities;

	float CollisionCooldownTimer = 0.0f;
	float DynamicBuildingTimer = 0.0f;
};
