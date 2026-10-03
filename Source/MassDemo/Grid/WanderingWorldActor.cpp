// Copyright Epic Games, Inc. All Rights Reserved.

#include "Grid/WanderingWorldActor.h"
#include "Grid/WorldGridComponent.h"
#include "Grid/MassGridFragments.h"
#include "Grid/MassGridSubsystem.h"
#include "Grid/MassGridBuilding.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassCommonFragments.h"
#include "MassSignalSubsystem.h"
#include "Engine/World.h"

namespace MassDemoSignals
{
	const FName WorldCollision = FName(TEXT("WorldCollision"));
}

AWanderingWorldActor::AWanderingWorldActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BaseMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMeshComponent"));
	BaseMeshComponent->SetupAttachment(RootComponent);
	BaseMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BuildingVisualComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BuildingVisualComponent"));
	BuildingVisualComponent->SetupAttachment(RootComponent);
	BuildingVisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EntryVisualComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("EntryVisualComponent"));
	EntryVisualComponent->SetupAttachment(RootComponent);
	EntryVisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UnitVisualComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("UnitVisualComponent"));
	UnitVisualComponent->SetupAttachment(RootComponent);
	UnitVisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		BuildingMesh = CubeMeshFinder.Object;
		EntryMesh = CubeMeshFinder.Object;
		BuildingVisualComponent->SetStaticMesh(BuildingMesh);
		EntryVisualComponent->SetStaticMesh(EntryMesh);
		BaseMeshComponent->SetStaticMesh(CubeMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		UnitMesh = SphereMeshFinder.Object;
		UnitVisualComponent->SetStaticMesh(UnitMesh);
	}

	CollisionBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBoxComponent"));
	CollisionBoxComponent->SetupAttachment(RootComponent);
	CollisionBoxComponent->SetBoxExtent(FVector(800.0f, 1000.0f, 200.0f));
	CollisionBoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBoxComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBoxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBoxComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionBoxComponent->SetGenerateOverlapEvents(true);

	WorldGridComponent = CreateDefaultSubobject<UWorldGridComponent>(TEXT("WorldGridComponent"));
}

void AWanderingWorldActor::BeginPlay()
{
	Super::BeginPlay();

	CollisionBoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AWanderingWorldActor::OnCollisionBoxBeginOverlap);

	if (WorldGridComponent)
	{
		WorldGridComponent->OnGridInitialized.AddDynamic(this, &AWanderingWorldActor::HandleGridInitialized);
		WorldGridComponent->OnBuildingAdded.AddDynamic(this, &AWanderingWorldActor::HandleBuildingAdded);

		WorldGridComponent->InitializeGrid(GridWidth, GridHeight, GridCellSize, bAutoPopulateBuildings ? InitialBuildingCount : 0);
	}

	if (bAutoSpawnEntitiesOnBeginPlay)
	{
		SpawnGridEntities(InitialEntityCount);
	}
}

void AWanderingWorldActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CollisionCooldownTimer > 0.0f)
	{
		CollisionCooldownTimer -= DeltaSeconds;
	}

	if (bEnableMovement)
	{
		FVector CurrentLocation = GetActorLocation();
		FVector NewLocation = CurrentLocation + MovementVelocity * DeltaSeconds;

		// Bounding box checks & bounce reflections
		if (NewLocation.X <= MovementBoundsMin.X || NewLocation.X >= MovementBoundsMax.X)
		{
			MovementVelocity.X *= -1.0f;
			NewLocation.X = FMath::Clamp(NewLocation.X, MovementBoundsMin.X, MovementBoundsMax.X);
		}
		if (NewLocation.Y <= MovementBoundsMin.Y || NewLocation.Y >= MovementBoundsMax.Y)
		{
			MovementVelocity.Y *= -1.0f;
			NewLocation.Y = FMath::Clamp(NewLocation.Y, MovementBoundsMin.Y, MovementBoundsMax.Y);
		}
		if (NewLocation.Z <= MovementBoundsMin.Z || NewLocation.Z >= MovementBoundsMax.Z)
		{
			MovementVelocity.Z *= -1.0f;
			NewLocation.Z = FMath::Clamp(NewLocation.Z, MovementBoundsMin.Z, MovementBoundsMax.Z);
		}

		SetActorLocation(NewLocation);
	}

	if (bVisualizeUnits && UnitVisualComponent && SpawnedEntities.Num() > 0)
	{
		UpdateUnitVisuals();
	}
}

int32 AWanderingWorldActor::GetGridId() const
{
	return WorldGridComponent ? WorldGridComponent->GetGridId() : INDEX_NONE;
}

void AWanderingWorldActor::OnCollisionBoxBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		if (AWanderingWorldActor* OtherWorld = Cast<AWanderingWorldActor>(OtherActor))
		{
			HandleWorldCollision(OtherWorld);
		}
	}
}

void AWanderingWorldActor::HandleWorldCollision(AWanderingWorldActor* OtherWorld)
{
	if (!OtherWorld || CollisionCooldownTimer > 0.0f)
	{
		return;
	}

	CollisionCooldownTimer = 1.0f;

	// Bounce velocity away from the other world
	FVector BounceDir = (GetActorLocation() - OtherWorld->GetActorLocation()).GetSafeNormal();
	if (BounceDir.IsNearlyZero())
	{
		MovementVelocity *= -1.0f;
	}
	else
	{
		const float Speed = MovementVelocity.Size();
		MovementVelocity = BounceDir * (Speed > 10.0f ? Speed : 100.0f);
	}

	// Trigger collision delegate
	OnWorldCollision.Broadcast(this, OtherWorld);

	// Stun entities on both colliding worlds for 2.0s
	StunAllEntities(2.0f);
	OtherWorld->StunAllEntities(2.0f);
}

void AWanderingWorldActor::StunAllEntities(float StunDuration)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	// Direct fragment stun application
	for (const FMassEntityHandle& EntityHandle : SpawnedEntities)
	{
		if (EntityManager.IsEntityValid(EntityHandle))
		{
			if (FMassStunFragment* StunFrag = EntityManager.GetFragmentDataPtr<FMassStunFragment>(EntityHandle))
			{
				StunFrag->ApplyStun(StunDuration);
			}
		}
	}

	// Dispatch Mass Signal for any reactive processors listening to WorldCollision
	if (UMassSignalSubsystem* SignalSubsystem = World->GetSubsystem<UMassSignalSubsystem>())
	{
		SignalSubsystem->SignalEntities(MassDemoSignals::WorldCollision, SpawnedEntities);
	}
}

void AWanderingWorldActor::SpawnGridEntities(int32 Count)
{
	if (Count <= 0 || !WorldGridComponent)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
	const FWorldGridData& GridData = WorldGridComponent->GetGridData();
	const int32 MyGridId = WorldGridComponent->GetGridId();

	const TArray<const UScriptStruct*> FragmentTypes = {
		FMassGridLocationFragment::StaticStruct(),
		FMassPathFollowingFragment::StaticStruct(),
		FMassWorldReferenceFragment::StaticStruct(),
		FMassStunFragment::StaticStruct(),
		FMassUnitBehaviorFragment::StaticStruct(),
		FTransformFragment::StaticStruct()
	};

	FMassArchetypeHandle Archetype = EntityManager.CreateArchetype(FragmentTypes);

	for (int32 i = 0; i < Count; ++i)
	{
		FGridCoord SpawnCoord;
		if (!GridData.GetRandomWalkableCell(SpawnCoord))
		{
			SpawnCoord = FGridCoord(0, 0);
		}

		FVector LocalPos = GridData.CellToLocalPosition(SpawnCoord, 50.0f);
		FVector WorldPos = GetTransform().TransformPosition(LocalPos);

		FMassEntityHandle NewEntity = EntityManager.CreateEntity(Archetype);

		// Initialize Fragment values
		FMassGridLocationFragment& LocFrag = EntityManager.GetFragmentDataChecked<FMassGridLocationFragment>(NewEntity);
		LocFrag.CurrentCoord = SpawnCoord;
		LocFrag.TargetCoord = SpawnCoord;
		LocFrag.LocalPosition = LocalPos;
		LocFrag.TargetPosition = LocalPos;

		FMassPathFollowingFragment& PathFrag = EntityManager.GetFragmentDataChecked<FMassPathFollowingFragment>(NewEntity);
		PathFrag.ClearPath();
		PathFrag.MovementSpeed = 200.0f;
		PathFrag.AcceptanceRadius = 15.0f;

		FMassWorldReferenceFragment& WorldRefFrag = EntityManager.GetFragmentDataChecked<FMassWorldReferenceFragment>(NewEntity);
		WorldRefFrag.GridId = MyGridId;
		WorldRefFrag.OwningWorldActor = this;

		FMassStunFragment& StunFrag = EntityManager.GetFragmentDataChecked<FMassStunFragment>(NewEntity);
		StunFrag.ClearStun();

		FMassUnitBehaviorFragment& BehaviorFrag = EntityManager.GetFragmentDataChecked<FMassUnitBehaviorFragment>(NewEntity);
		BehaviorFrag.ResetIdleTimer();

		FTransformFragment& TransformFrag = EntityManager.GetFragmentDataChecked<FTransformFragment>(NewEntity);
		TransformFrag.SetTransform(FTransform(FRotator::ZeroRotator, WorldPos));

		SpawnedEntities.Add(NewEntity);
	}

	if (bVisualizeUnits)
	{
		RebuildUnitVisuals();
	}
}

void AWanderingWorldActor::HandleGridInitialized()
{
	UpdateBaseMeshVisual();
	RebuildBuildingVisuals();
	if (bVisualizeUnits)
	{
		RebuildUnitVisuals();
	}
}

void AWanderingWorldActor::HandleBuildingAdded(const FPlacedBuilding& NewBuilding)
{
	if (bVisualizeBuildings)
	{
		AddBuildingVisual(NewBuilding);
	}
}

void AWanderingWorldActor::RebuildBuildingVisuals()
{
	ClearBuildingVisuals();

	if (!bVisualizeBuildings || !WorldGridComponent)
	{
		return;
	}

	if (BuildingMesh && BuildingVisualComponent && BuildingVisualComponent->GetStaticMesh() != BuildingMesh)
	{
		BuildingVisualComponent->SetStaticMesh(BuildingMesh);
	}
	if (BuildingMaterial && BuildingVisualComponent)
	{
		BuildingVisualComponent->SetMaterial(0, BuildingMaterial);
	}

	if (EntryMesh && EntryVisualComponent && EntryVisualComponent->GetStaticMesh() != EntryMesh)
	{
		EntryVisualComponent->SetStaticMesh(EntryMesh);
	}
	if (EntryMaterial && EntryVisualComponent)
	{
		EntryVisualComponent->SetMaterial(0, EntryMaterial);
	}

	const TArray<FPlacedBuilding>& Buildings = WorldGridComponent->GetPlacedBuildings();
	for (const FPlacedBuilding& Building : Buildings)
	{
		AddBuildingVisual(Building);
	}
}

void AWanderingWorldActor::AddBuildingVisual(const FPlacedBuilding& Building)
{
	if (!WorldGridComponent)
	{
		return;
	}

	const FWorldGridData& GridData = WorldGridComponent->GetGridData();
	const float GridZ = WorldGridComponent->GridZOffset;

	if (BuildingVisualComponent)
	{
		const TArray<FTransform> BlockTransforms = UMassGridBuildingLibrary::CalculateBuildingBlockTransforms(
			GridData, Building, GridZ, BuildingHeight, BuildingBlockScale);

		for (const FTransform& Trans : BlockTransforms)
		{
			BuildingVisualComponent->AddInstance(Trans);
		}
	}

	if (bVisualizeEntries && EntryVisualComponent)
	{
		const FTransform EntryTrans = UMassGridBuildingLibrary::CalculateBuildingEntryTransform(
			GridData, Building, GridZ, EntryHeight, EntryBlockScale);

		EntryVisualComponent->AddInstance(EntryTrans);
	}
}

void AWanderingWorldActor::ClearBuildingVisuals()
{
	if (BuildingVisualComponent)
	{
		BuildingVisualComponent->ClearInstances();
	}
	if (EntryVisualComponent)
	{
		EntryVisualComponent->ClearInstances();
	}
}

void AWanderingWorldActor::UpdateBaseMeshVisual()
{
	if (!BaseMeshComponent || !WorldGridComponent)
	{
		return;
	}

	if (BaseMeshMaterial)
	{
		BaseMeshComponent->SetMaterial(0, BaseMeshMaterial);
	}

	const float FloorWidth = GridWidth * GridCellSize;
	const float FloorHeight = GridHeight * GridCellSize;
	const float FloorThickness = 20.0f;
	const float GridZ = WorldGridComponent->GridZOffset;

	BaseMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, GridZ - FloorThickness * 0.5f));
	BaseMeshComponent->SetRelativeScale3D(FVector(FloorWidth / 100.0f, FloorHeight / 100.0f, FloorThickness / 100.0f));

	if (CollisionBoxComponent)
	{
		CollisionBoxComponent->SetBoxExtent(FVector(FloorWidth * 0.5f + 100.0f, FloorHeight * 0.5f + 100.0f, 200.0f));
	}
}

void AWanderingWorldActor::RebuildUnitVisuals()
{
	ClearUnitVisuals();

	if (!bVisualizeUnits || !UnitVisualComponent)
	{
		return;
	}

	if (UnitMesh && UnitVisualComponent->GetStaticMesh() != UnitMesh)
	{
		UnitVisualComponent->SetStaticMesh(UnitMesh);
	}
	if (UnitMaterial && UnitVisualComponent)
	{
		UnitVisualComponent->SetMaterial(0, UnitMaterial);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	for (const FMassEntityHandle& EntityHandle : SpawnedEntities)
	{
		if (!EntityManager.IsEntityValid(EntityHandle))
		{
			continue;
		}

		FVector LocalPos = FVector::ZeroVector;
		if (const FMassGridLocationFragment* LocFrag = EntityManager.GetFragmentDataPtr<FMassGridLocationFragment>(EntityHandle))
		{
			LocalPos = LocFrag->LocalPosition;
		}

		bool bIsStunned = false;
		if (const FMassStunFragment* StunFrag = EntityManager.GetFragmentDataPtr<FMassStunFragment>(EntityHandle))
		{
			bIsStunned = StunFrag->bIsStunned;
		}

		const FTransform UnitTransform = UMassGridBuildingLibrary::CalculateUnitTransform(LocalPos, UnitZOffset, UnitScale, bIsStunned);
		UnitVisualComponent->AddInstance(UnitTransform);
	}
}

void AWanderingWorldActor::UpdateUnitVisuals()
{
	if (!bVisualizeUnits || !UnitVisualComponent)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	if (UnitVisualComponent->GetInstanceCount() != SpawnedEntities.Num())
	{
		RebuildUnitVisuals();
		return;
	}

	for (int32 i = 0; i < SpawnedEntities.Num(); ++i)
	{
		const FMassEntityHandle& EntityHandle = SpawnedEntities[i];
		if (!EntityManager.IsEntityValid(EntityHandle))
		{
			continue;
		}

		FVector LocalPos = FVector::ZeroVector;
		if (const FMassGridLocationFragment* LocFrag = EntityManager.GetFragmentDataPtr<FMassGridLocationFragment>(EntityHandle))
		{
			LocalPos = LocFrag->LocalPosition;
		}

		bool bIsStunned = false;
		if (const FMassStunFragment* StunFrag = EntityManager.GetFragmentDataPtr<FMassStunFragment>(EntityHandle))
		{
			bIsStunned = StunFrag->bIsStunned;
		}

		const FTransform UnitTransform = UMassGridBuildingLibrary::CalculateUnitTransform(LocalPos, UnitZOffset, UnitScale, bIsStunned);
		UnitVisualComponent->UpdateInstanceTransform(i, UnitTransform, false, false, true);
	}

	UnitVisualComponent->MarkRenderStateDirty();
}

void AWanderingWorldActor::ClearUnitVisuals()
{
	if (UnitVisualComponent)
	{
		UnitVisualComponent->ClearInstances();
	}
}
