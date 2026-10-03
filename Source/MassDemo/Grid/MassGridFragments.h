// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "MassGridTypes.h"
#include "MassGridFragments.generated.h"

class AWanderingWorldActor;

/**
 * Fragment storing grid coordinates and local positions for Mass entities on a grid.
 */
USTRUCT()
struct MASSDEMO_API FMassGridLocationFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	FGridCoord CurrentCoord = FGridCoord(0, 0);

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	FGridCoord TargetCoord = FGridCoord(0, 0);

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	FVector LocalPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	FVector TargetPosition = FVector::ZeroVector;
};

/**
 * Fragment storing path navigation state along local grid waypoints.
 * Uses a fixed buffer to ensure trivially copyable ECS chunk performance.
 */
USTRUCT()
struct MASSDEMO_API FMassPathFollowingFragment : public FMassFragment
{
	GENERATED_BODY()

	static constexpr int32 MaxWaypoints = 64;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	FVector LocalWaypoints[MaxWaypoints];

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	int32 NumWaypoints = 0;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	int32 CurrentWaypointIndex = 0;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	float MovementSpeed = 200.0f;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	float AcceptanceRadius = 15.0f;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	bool bHasActivePath = false;

	void SetPath(TConstArrayView<FVector> InWaypoints)
	{
		NumWaypoints = FMath::Min(InWaypoints.Num(), MaxWaypoints);
		for (int32 i = 0; i < NumWaypoints; ++i)
		{
			LocalWaypoints[i] = InWaypoints[i];
		}
		CurrentWaypointIndex = NumWaypoints > 1 ? 1 : 0;
		bHasActivePath = NumWaypoints > 0;
	}

	void ClearPath()
	{
		NumWaypoints = 0;
		CurrentWaypointIndex = 0;
		bHasActivePath = false;
	}

	bool HasReachedDestination() const
	{
		return !bHasActivePath || CurrentWaypointIndex >= NumWaypoints;
	}
};

/**
 * Fragment linking a Mass entity to its owning WanderingWorldActor / GridID.
 */
USTRUCT()
struct MASSDEMO_API FMassWorldReferenceFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	int32 GridId = INDEX_NONE;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	TWeakObjectPtr<AWanderingWorldActor> OwningWorldActor = nullptr;
};

/**
 * Fragment tracking stun state (e.g. from world-to-world collision impacts).
 */
USTRUCT()
struct MASSDEMO_API FMassStunFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	bool bIsStunned = false;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	float StunTimeRemaining = 0.0f;

	void ApplyStun(float Duration = 2.0f)
	{
		bIsStunned = true;
		StunTimeRemaining = FMath::Max(StunTimeRemaining, Duration);
	}

	void ClearStun()
	{
		bIsStunned = false;
		StunTimeRemaining = 0.0f;
	}
};

/**
 * Fragment controlling idle & wander decision timers.
 */
USTRUCT()
struct MASSDEMO_API FMassUnitBehaviorFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	float IdleTimeRemaining = 1.0f;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	float MinIdleTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "MassGrid")
	float MaxIdleTime = 3.0f;

	void ResetIdleTimer()
	{
		IdleTimeRemaining = FMath::RandRange(MinIdleTime, MaxIdleTime);
	}
};
