// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassSignalProcessorBase.h"
#include "MassGridProcessors.generated.h"

/**
 * Processor responsible for autonomous wander behavior:
 * Counts down idle timers, selects random reachable cells on the local grid,
 * and generates A* paths into FMassPathFollowingFragment.
 */
UCLASS()
class MASSDEMO_API UMassGridWanderBehaviorProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassGridWanderBehaviorProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

/**
 * Processor responsible for stepping entities along their local grid waypoints
 * and converting local coordinates to world transforms relative to the moving WanderingWorldActor.
 */
UCLASS()
class MASSDEMO_API UMassGridMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassGridMovementProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

/**
 * Processor responsible for updating stun countdowns on stunned entities.
 */
UCLASS()
class MASSDEMO_API UMassStunProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassStunProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

/**
 * Processor responsible for building entrance interactions (cargo handling, service timer).
 */
UCLASS()
class MASSDEMO_API UMassGridInteractionProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassGridInteractionProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

/**
 * Signal processor handling WorldCollision signals to apply stun to all affected entities.
 */
UCLASS()
class MASSDEMO_API UMassWorldCollisionSignalProcessor : public UMassSignalProcessorBase
{
	GENERATED_BODY()

public:
	UMassWorldCollisionSignalProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context, FMassSignalNameLookup& SignalNameLookup) override;

private:
	FMassEntityQuery EntityQuery;
};
