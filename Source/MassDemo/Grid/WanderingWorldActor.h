// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassEntityTypes.h"
#include "MassGridTypes.h"
#include "WanderingWorldActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UWorldGridComponent;

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

	/** Collision Box for detecting world-to-world impacts */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBoxComponent;

	/** Local World Grid Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWorldGridComponent> WorldGridComponent;

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

	/** Gets all entity handles belonging to this world */
	const TArray<FMassEntityHandle>& GetSpawnedEntities() const { return SpawnedEntities; }

private:
	UPROPERTY()
	TArray<FMassEntityHandle> SpawnedEntities;

	float CollisionCooldownTimer = 0.0f;
};
