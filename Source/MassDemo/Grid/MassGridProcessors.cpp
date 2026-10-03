// Copyright Epic Games, Inc. All Rights Reserved.

#include "Grid/MassGridProcessors.h"
#include "Grid/MassGridFragments.h"
#include "Grid/MassGridSubsystem.h"
#include "Grid/MassGridPathfinder.h"
#include "Grid/WanderingWorldActor.h"
#include "MassCommonTypes.h"
#include "MassMovementTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassSignalSubsystem.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// UMassGridWanderBehaviorProcessor
// ---------------------------------------------------------------------------

UMassGridWanderBehaviorProcessor::UMassGridWanderBehaviorProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);
}

void UMassGridWanderBehaviorProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassUnitBehaviorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassPathFollowingFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassGridLocationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWorldReferenceFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassStunFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<UMassGridSubsystem>(EMassFragmentAccess::ReadOnly);
}

void UMassGridWanderBehaviorProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	UMassGridSubsystem* GridSubsystem = Context.GetMutableSubsystem<UMassGridSubsystem>();

	if (!GridSubsystem)
	{
		return;
	}

	EntityQuery.ForEachEntityChunk(Context, [this, DeltaTime, GridSubsystem](FMassExecutionContext& ChunkContext)
	{
		const int32 NumEntities = ChunkContext.GetNumEntities();
		const TArrayView<FMassUnitBehaviorFragment> BehaviorList = ChunkContext.GetMutableFragmentView<FMassUnitBehaviorFragment>();
		const TArrayView<FMassPathFollowingFragment> PathList = ChunkContext.GetMutableFragmentView<FMassPathFollowingFragment>();
		const TArrayView<FMassGridLocationFragment> LocationList = ChunkContext.GetMutableFragmentView<FMassGridLocationFragment>();
		const TConstArrayView<FMassWorldReferenceFragment> WorldRefList = ChunkContext.GetFragmentView<FMassWorldReferenceFragment>();
		const TConstArrayView<FMassStunFragment> StunList = ChunkContext.GetFragmentView<FMassStunFragment>();

		for (int32 i = 0; i < NumEntities; ++i)
		{
			const FMassStunFragment& StunFrag = StunList[i];
			if (StunFrag.bIsStunned)
			{
				continue;
			}

			FMassPathFollowingFragment& PathFrag = PathList[i];
			if (PathFrag.bHasActivePath)
			{
				continue;
			}

			FMassUnitBehaviorFragment& BehaviorFrag = BehaviorList[i];
			BehaviorFrag.IdleTimeRemaining -= DeltaTime;

			if (BehaviorFrag.IdleTimeRemaining <= 0.0f)
			{
				BehaviorFrag.ResetIdleTimer();

				const FMassWorldReferenceFragment& WorldRefFrag = WorldRefList[i];
				const FWorldGridData* GridData = GridSubsystem->GetGridData(WorldRefFrag.GridId);
				if (!GridData)
				{
					continue;
				}

				FMassGridLocationFragment& LocFrag = LocationList[i];
				FGridCoord TargetCoord;
				if (GridData->GetRandomWalkableCell(TargetCoord))
				{
					TArray<FGridCoord> CoordPath;
					if (UMassGridPathfinderLibrary::FindPath(*GridData, LocFrag.CurrentCoord, TargetCoord, CoordPath))
					{
						if (CoordPath.Num() > 1)
						{
							TArray<FVector> CalculatedWaypoints;
							UMassGridPathfinderLibrary::ConvertPathToLocalPositions(*GridData, CoordPath, CalculatedWaypoints, 50.0f);
							PathFrag.SetPath(CalculatedWaypoints);
							LocFrag.TargetCoord = TargetCoord;
						}
					}
				}
			}
		}
	});
}

// ---------------------------------------------------------------------------
// UMassGridMovementProcessor
// ---------------------------------------------------------------------------

UMassGridMovementProcessor::UMassGridMovementProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void UMassGridMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassPathFollowingFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassGridLocationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWorldReferenceFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassStunFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<UMassGridSubsystem>(EMassFragmentAccess::ReadOnly);
}

void UMassGridMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	UMassGridSubsystem* GridSubsystem = Context.GetMutableSubsystem<UMassGridSubsystem>();

	EntityQuery.ForEachEntityChunk(Context, [DeltaTime, GridSubsystem](FMassExecutionContext& ChunkContext)
	{
		const int32 NumEntities = ChunkContext.GetNumEntities();
		const TArrayView<FMassPathFollowingFragment> PathList = ChunkContext.GetMutableFragmentView<FMassPathFollowingFragment>();
		const TArrayView<FMassGridLocationFragment> LocationList = ChunkContext.GetMutableFragmentView<FMassGridLocationFragment>();
		const TArrayView<FTransformFragment> TransformList = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FMassWorldReferenceFragment> WorldRefList = ChunkContext.GetFragmentView<FMassWorldReferenceFragment>();
		const TConstArrayView<FMassStunFragment> StunList = ChunkContext.GetFragmentView<FMassStunFragment>();

		for (int32 i = 0; i < NumEntities; ++i)
		{
			const FMassStunFragment& StunFrag = StunList[i];
			const FMassWorldReferenceFragment& WorldRefFrag = WorldRefList[i];
			FMassPathFollowingFragment& PathFrag = PathList[i];
			FMassGridLocationFragment& LocFrag = LocationList[i];
			FTransformFragment& TransformFrag = TransformList[i];

			AActor* WorldActor = WorldRefFrag.OwningWorldActor.Get();
			const FTransform WorldActorTransform = WorldActor ? WorldActor->GetActorTransform() : FTransform::Identity;

			// If stunned or no active path, keep world position synced with moving world actor
			if (StunFrag.bIsStunned || !PathFrag.bHasActivePath || PathFrag.NumWaypoints == 0)
			{
				const FVector WorldPos = WorldActorTransform.TransformPosition(LocFrag.LocalPosition);
				TransformFrag.GetMutableTransform().SetTranslation(WorldPos);
				continue;
			}

			// Traverse waypoints
			if (PathFrag.CurrentWaypointIndex < PathFrag.NumWaypoints)
			{
				const FVector TargetLocalPos = PathFrag.LocalWaypoints[PathFrag.CurrentWaypointIndex];
				const FVector ToTarget = TargetLocalPos - LocFrag.LocalPosition;
				const float Distance = ToTarget.Size();
				const float StepDistance = PathFrag.MovementSpeed * DeltaTime;

				if (Distance <= StepDistance || Distance <= PathFrag.AcceptanceRadius)
				{
					LocFrag.LocalPosition = TargetLocalPos;
					PathFrag.CurrentWaypointIndex++;

					if (GridSubsystem)
					{
						if (const FWorldGridData* GridData = GridSubsystem->GetGridData(WorldRefFrag.GridId))
						{
							LocFrag.CurrentCoord = GridData->LocalPositionToCell(LocFrag.LocalPosition);
						}
					}

					if (PathFrag.CurrentWaypointIndex >= PathFrag.NumWaypoints)
					{
						PathFrag.ClearPath();
						LocFrag.CurrentCoord = LocFrag.TargetCoord;
					}
				}
				else
				{
					LocFrag.LocalPosition += ToTarget.GetSafeNormal() * StepDistance;
				}
			}

			// Update world transform
			const FVector WorldPos = WorldActorTransform.TransformPosition(LocFrag.LocalPosition);
			TransformFrag.GetMutableTransform().SetTranslation(WorldPos);
		}
	});
}

// ---------------------------------------------------------------------------
// UMassStunProcessor
// ---------------------------------------------------------------------------

UMassStunProcessor::UMassStunProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);
}

void UMassStunProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassStunFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassStunProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& ChunkContext)
	{
		const int32 NumEntities = ChunkContext.GetNumEntities();
		const TArrayView<FMassStunFragment> StunList = ChunkContext.GetMutableFragmentView<FMassStunFragment>();

		for (int32 i = 0; i < NumEntities; ++i)
		{
			FMassStunFragment& StunFrag = StunList[i];
			if (StunFrag.bIsStunned)
			{
				StunFrag.StunTimeRemaining -= DeltaTime;
				if (StunFrag.StunTimeRemaining <= 0.0f)
				{
					StunFrag.ClearStun();
				}
			}
		}
	});
}

// ---------------------------------------------------------------------------
// UMassWorldCollisionSignalProcessor
// ---------------------------------------------------------------------------

UMassWorldCollisionSignalProcessor::UMassWorldCollisionSignalProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
}

void UMassWorldCollisionSignalProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::InitializeInternal(Owner, EntityManager);

	if (UWorld* World = Owner.GetWorld())
	{
		if (UMassSignalSubsystem* SignalSubsystem = World->GetSubsystem<UMassSignalSubsystem>())
		{
			SubscribeToSignal(*SignalSubsystem, FName(TEXT("WorldCollision")));
		}
	}
}

void UMassWorldCollisionSignalProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassStunFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassWorldCollisionSignalProcessor::SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context, FMassSignalNameLookup& /*InSignalNameLookup*/)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& ChunkContext)
	{
		const int32 NumEntities = ChunkContext.GetNumEntities();
		const TArrayView<FMassStunFragment> StunList = ChunkContext.GetMutableFragmentView<FMassStunFragment>();

		for (int32 i = 0; i < NumEntities; ++i)
		{
			StunList[i].ApplyStun(2.0f);
		}
	});
}
