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

	EntityQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& ChunkContext)
	{
		const UMassGridSubsystem* GridSubsystem = ChunkContext.GetSubsystem<UMassGridSubsystem>();
		if (!GridSubsystem)
		{
			return;
		}

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

			FMassUnitBehaviorFragment& BehaviorFrag = BehaviorList[i];
			if (BehaviorFrag.State == EUnitBehaviorState::Interacting)
			{
				continue;
			}

			const FMassWorldReferenceFragment& WorldRefFrag = WorldRefList[i];
			const FWorldGridData* GridData = GridSubsystem->GetGridData(WorldRefFrag.GridId);
			if (!GridData)
			{
				continue;
			}

			FMassPathFollowingFragment& PathFrag = PathList[i];
			FMassGridLocationFragment& LocFrag = LocationList[i];

			// Handle dynamic reroute request (e.g. active path became blocked by a newly spawned building)
			if (PathFrag.bNeedsReroute)
			{
				TArray<FGridCoord> CoordPath;
				bool bRerouteFound = false;

				// First attempt: try reaching previous TargetCoord if it is still walkable
				if (GridData->IsWalkable(LocFrag.TargetCoord))
				{
					bRerouteFound = UMassGridPathfinderLibrary::FindPathWithAvoidance(*GridData, LocFrag.CurrentCoord, LocFrag.TargetCoord, CoordPath);
				}

				// If previous target is now blocked or unreachable, choose a new walkable cell
				if (!bRerouteFound)
				{
					FGridCoord NewTarget;
					if (GridData->GetRandomWalkableCell(NewTarget))
					{
						LocFrag.TargetCoord = NewTarget;
						bRerouteFound = UMassGridPathfinderLibrary::FindPathWithAvoidance(*GridData, LocFrag.CurrentCoord, NewTarget, CoordPath);
					}
				}

				if (bRerouteFound && CoordPath.Num() > 1)
				{
					TArray<FVector> CalculatedWaypoints;
					UMassGridPathfinderLibrary::ConvertPathToLocalPositions(*GridData, CoordPath, CalculatedWaypoints, 50.0f);
					const FGridCell* TargetCell = GridData->GetCell(LocFrag.TargetCoord);
					const bool bIsEntry = TargetCell && TargetCell->CellType == EGridCellType::BuildingEntry;
					if (PathFrag.SetPath(CalculatedWaypoints, bIsEntry))
					{
						BehaviorFrag.State = EUnitBehaviorState::Wandering;
						BehaviorFrag.bTargetingBuildingEntry = bIsEntry;
						BehaviorFrag.TargetBuildingId = bIsEntry && TargetCell ? TargetCell->BuildingId : INDEX_NONE;
					}
				}
				else
				{
					PathFrag.ClearPath();
					BehaviorFrag.ResetIdleTimer();
				}
				continue;
			}

			if (PathFrag.bHasActivePath)
			{
				continue;
			}

			// Countdown idle timer
			BehaviorFrag.IdleTimeRemaining -= DeltaTime;

			if (BehaviorFrag.IdleTimeRemaining <= 0.0f)
			{
				FGridCoord TargetCoord;
				bool bTargetFound = false;
				bool bIsBuildingEntry = false;

				// 40% chance to target a building entry if available
				const bool bTryBuilding = FMath::RandRange(1, 100) <= 40;
				if (bTryBuilding && GridData->GetRandomBuildingEntry(TargetCoord))
				{
					bTargetFound = true;
					bIsBuildingEntry = true;
				}
				else if (GridData->GetRandomWalkableCell(TargetCoord))
				{
					bTargetFound = true;
					bIsBuildingEntry = false;
				}

				if (bTargetFound)
				{
					TArray<FGridCoord> CoordPath;
					if (UMassGridPathfinderLibrary::FindPathWithAvoidance(*GridData, LocFrag.CurrentCoord, TargetCoord, CoordPath))
					{
						if (CoordPath.Num() > 1)
						{
							TArray<FVector> CalculatedWaypoints;
							UMassGridPathfinderLibrary::ConvertPathToLocalPositions(*GridData, CoordPath, CalculatedWaypoints, 50.0f);
							if (PathFrag.SetPath(CalculatedWaypoints, bIsBuildingEntry))
							{
								const FGridCell* TargetCell = GridData->GetCell(TargetCoord);
								LocFrag.TargetCoord = TargetCoord;
								BehaviorFrag.State = EUnitBehaviorState::Wandering;
								BehaviorFrag.bTargetingBuildingEntry = bIsBuildingEntry;
								BehaviorFrag.TargetBuildingId = bIsBuildingEntry && TargetCell ? TargetCell->BuildingId : INDEX_NONE;
							}
						}
						else
						{
							BehaviorFrag.ResetIdleTimer();
						}
					}
					else
					{
						BehaviorFrag.ResetIdleTimer();
					}
				}
				else
				{
					BehaviorFrag.ResetIdleTimer();
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
	EntityQuery.AddRequirement<FMassUnitBehaviorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassUnitInteractionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWorldReferenceFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassStunFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<UMassGridSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UMassGridMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& ChunkContext)
	{
		UMassGridSubsystem* GridSubsystem = ChunkContext.GetMutableSubsystem<UMassGridSubsystem>();
		if (!GridSubsystem)
		{
			return;
		}

		const int32 NumEntities = ChunkContext.GetNumEntities();
		const TArrayView<FMassPathFollowingFragment> PathList = ChunkContext.GetMutableFragmentView<FMassPathFollowingFragment>();
		const TArrayView<FMassGridLocationFragment> LocationList = ChunkContext.GetMutableFragmentView<FMassGridLocationFragment>();
		const TArrayView<FTransformFragment> TransformList = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassUnitBehaviorFragment> BehaviorList = ChunkContext.GetMutableFragmentView<FMassUnitBehaviorFragment>();
		const TArrayView<FMassUnitInteractionFragment> InteractionList = ChunkContext.GetMutableFragmentView<FMassUnitInteractionFragment>();
		const TConstArrayView<FMassWorldReferenceFragment> WorldRefList = ChunkContext.GetFragmentView<FMassWorldReferenceFragment>();
		const TConstArrayView<FMassStunFragment> StunList = ChunkContext.GetFragmentView<FMassStunFragment>();

		for (int32 i = 0; i < NumEntities; ++i)
		{
			const FMassStunFragment& StunFrag = StunList[i];
			const FMassWorldReferenceFragment& WorldRefFrag = WorldRefList[i];
			FMassPathFollowingFragment& PathFrag = PathList[i];
			FMassGridLocationFragment& LocFrag = LocationList[i];
			FTransformFragment& TransformFrag = TransformList[i];
			FMassUnitBehaviorFragment& BehaviorFrag = BehaviorList[i];
			FMassUnitInteractionFragment& InteractionFrag = InteractionList[i];

			AActor* WorldActor = WorldRefFrag.OwningWorldActor.Get();
			const FTransform WorldActorTransform = WorldActor ? WorldActor->GetActorTransform() : FTransform::Identity;

			// If stunned, interacting, or no active path, keep world position synced with moving world actor
			if (StunFrag.bIsStunned || BehaviorFrag.State == EUnitBehaviorState::Interacting || !PathFrag.bHasActivePath || PathFrag.NumWaypoints == 0)
			{
				const FVector WorldPos = WorldActorTransform.TransformPosition(LocFrag.LocalPosition);
				TransformFrag.GetMutableTransform().SetTranslation(WorldPos);
				continue;
			}

			FWorldGridData* GridData = GridSubsystem ? GridSubsystem->GetGridData(WorldRefFrag.GridId) : nullptr;

			// Traverse waypoints
			if (PathFrag.CurrentWaypointIndex < PathFrag.NumWaypoints)
			{
				const FVector TargetLocalPos = PathFrag.LocalWaypoints[PathFrag.CurrentWaypointIndex];

				// Dynamic Obstacle Check: ensure all remaining waypoint cells are still walkable
				if (GridData)
				{
					bool bRemainingPathWalkable = true;
					for (int32 WaypointIndex = PathFrag.CurrentWaypointIndex; WaypointIndex < PathFrag.NumWaypoints; ++WaypointIndex)
					{
						const FGridCoord WaypointCoord = GridData->LocalPositionToCell(PathFrag.LocalWaypoints[WaypointIndex]);
						if (!GridData->IsWalkable(WaypointCoord))
						{
							bRemainingPathWalkable = false;
							break;
						}
					}

					if (!bRemainingPathWalkable)
					{
						// Dynamic obstacle spawned on our path! Trigger reroute
						PathFrag.InvalidateForReroute();
						continue;
					}
				}

				const FVector ToTarget = TargetLocalPos - LocFrag.LocalPosition;
				const float Distance = ToTarget.Size();
				const float StepDistance = PathFrag.MovementSpeed * DeltaTime;

				if (Distance <= StepDistance || Distance <= PathFrag.AcceptanceRadius)
				{
					LocFrag.LocalPosition = TargetLocalPos;
					PathFrag.CurrentWaypointIndex++;

					if (GridData)
					{
						const FGridCoord OldCoord = LocFrag.CurrentCoord;
						const FGridCoord NewCoord = GridData->LocalPositionToCell(LocFrag.LocalPosition);

						if (OldCoord != NewCoord)
						{
							GridData->DecrementOccupancy(OldCoord);
							GridData->TryIncrementOccupancy(NewCoord, 8);
							LocFrag.CurrentCoord = NewCoord;
						}
					}

					// Reached final waypoint
					if (PathFrag.CurrentWaypointIndex >= PathFrag.NumWaypoints)
					{
						if (PathFrag.bDestinationIsBuildingEntry)
						{
							BehaviorFrag.State = EUnitBehaviorState::Interacting;
							InteractionFrag.StartInteraction(2.0f, BehaviorFrag.TargetBuildingId);
						}
						else
						{
							BehaviorFrag.ResetIdleTimer();
						}

						PathFrag.ClearPath();
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
// UMassGridInteractionProcessor
// ---------------------------------------------------------------------------

UMassGridInteractionProcessor::UMassGridInteractionProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);
}

void UMassGridInteractionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassUnitBehaviorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassUnitInteractionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassStunFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassGridInteractionProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& ChunkContext)
	{
		const int32 NumEntities = ChunkContext.GetNumEntities();
		const TArrayView<FMassUnitBehaviorFragment> BehaviorList = ChunkContext.GetMutableFragmentView<FMassUnitBehaviorFragment>();
		const TArrayView<FMassUnitInteractionFragment> InteractionList = ChunkContext.GetMutableFragmentView<FMassUnitInteractionFragment>();
		const TConstArrayView<FMassStunFragment> StunList = ChunkContext.GetFragmentView<FMassStunFragment>();

		for (int32 i = 0; i < NumEntities; ++i)
		{
			const FMassStunFragment& StunFrag = StunList[i];
			if (StunFrag.bIsStunned)
			{
				continue;
			}

			FMassUnitBehaviorFragment& BehaviorFrag = BehaviorList[i];
			if (BehaviorFrag.State == EUnitBehaviorState::Interacting)
			{
				FMassUnitInteractionFragment& InteractionFrag = InteractionList[i];
				const bool bCompleted = InteractionFrag.StepInteraction(DeltaTime);
				if (bCompleted)
				{
					BehaviorFrag.ResetIdleTimer();
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
