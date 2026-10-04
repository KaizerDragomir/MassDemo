# MASS Demo — Part 1 Architecture Summary & Part 2 Roadmap

## 1. Project Overview & Architecture
This project implements a multi-world navigation and simulation demo using Unreal Engine's **Mass Framework** (`MassEntity`, `MassGameplay`, `MassSignals`, `MassActors`).
The system consists of independent "moving worlds" (floating masses/cubes) in 3D space, each hosting an isolated 2D grid with dynamic Tetris-like obstacle buildings and Mass entities (units) wandering in local actor space.

---

## 2. Completed Architecture (Part 1)

### 2.1. Configured Plugins & Build Modules
- **Plugins (`MassDemo.uproject`)**:
  - `MassGameplay`, `MassCrowd`, `MassAI`, `StateTree`, `GameplayStateTree`.
- **Modules (`MassDemo.Build.cs`)**:
  - `MassEntity`, `MassCommon`, `MassSignals`, `MassActors`, `MassSpawner`, `MassRepresentation`, `MassMovement`, `MassNavigation`, `StructUtils`.

### 2.2. Grid Data Structures (`Source/MassDemo/Grid/MassGridTypes.h`)
- **`FGridCoord`**: Integer 2D coordinate `(X, Y)` with Manhattan & Euclidean distance helpers and hash functions.
- **`EGridCellType`**: `Free`, `Blocked` (Building body), `BuildingEntry` (Walkable entrance/service point), `Reserved`.
- **`FGridCell`**: Cell metadata storing coordinate, cell type, owning `BuildingId`, and `OccupancyCount`.
- **`FWorldGridData`**: Singleton-free, center-aligned 2D grid structure:
  - Dynamic configurable dimensions ($W \times H$, default $10 \times 15$, scalable to $25 \times 25$ / $50 \times 50$).
  - Coordinate conversions: Local Cell Index $(X, Y) \longleftrightarrow$ Actor Local 3D Position $\longleftrightarrow$ World Space.
  - Walkability verification and random walkable cell query (`GetRandomWalkableCell`).

### 2.3. Tetris Blocker Generator (`Source/MassDemo/Grid/MassGridBuilding.h/.cpp`)
- **`FTetrisBuildingShape` / `ETetrisShapeType`**:
  - Configurable 2D blocker matrix presets from $1 \times 1$ to $3 \times 4$:
    - `Single1x1`, `Square2x2`, `Line3x1`, `Line1x3`, `LShape2x3`, `TShape3x2`, `ZShape3x2`, `BigBlock3x3`, `Custom3x4`.
  - Designated walkable entrance cell (`EntryOffset` $\rightarrow$ `EGridCellType::BuildingEntry`).
- **`UMassGridBuildingLibrary`**:
  - `TryPlaceBuilding()`: Validates grid boundaries and collision overlap before stamping blocked cells and entry points.
  - `PopulateGridWithRandomBuildings()`: Procedurally generates non-overlapping buildings across the grid.

### 2.4. Local 2D Pathfinding Engine (`Source/MassDemo/Grid/MassGridPathfinder.h/.cpp`)
- **`UMassGridPathfinderLibrary`**:
  - Min-Heap priority queue A* pathfinding algorithm with Manhattan distance heuristic.
  - Runs in local grid space, treating `Blocked` cells as impassable and `Free` / `BuildingEntry` cells as walkable.
  - `ConvertPathToLocalPositions()`: Converts a path of `FGridCoord` directly into local 3D waypoints with configurable height offset.

### 2.5. Multi-Grid Subsystem & Actor Component (`Source/MassDemo/Grid/MassGridSubsystem.h/.cpp`)
- **`UMassGridSubsystem`**: `UWorldSubsystem` managing multiple grid instances mapped by unique `GridID` / `WorldID`.
- **`UWorldGridComponent`**: `UActorComponent` attached to any world actor to own its grid instance without singletons.

---

## 3. Implementation Roadmap (Part 2: Moving Worlds, Mass Entities & Collision Stun)

### 3.1. Wandering World Actor (`AWanderingWorldActor`)
- Independent floating `AActor` with kinematic/spline movement or drifting velocity in 3D space.
- Components:
  - `UWorldGridComponent` (owns grid data and building layout).
  - `UStaticMeshComponent` / Visualizer (base cube or floating island mesh).
  - Collision component (Box/Sphere) for detecting world-to-world impacts.
  - Mass Spawner integration to spawn initial units (10–20 default, exposed for testing).

### 3.2. Mass Entity Definition & Fragments
- **`FMassGridLocationFragment`**:
  - Current grid coordinate $(X, Y)$ and target grid coordinate $(X, Y)$.
- **`FMassPathFollowingFragment`**:
  - Local path waypoint array, current path index, traversal progress, movement speed.
- **`FMassWorldReferenceFragment`**:
  - Reference/ID (`GridID` or `TWeakObjectPtr<AWanderingWorldActor>`) to the owning world actor for local-to-world transform evaluation.
- **`FMassStunFragment`**:
  - `bIsStunned` flag and `StunTimeRemaining` (default 2.0s countdown).
- **`FMassUnitBehaviorFragment`**:
  - Idle/wander decision delay timer (1–3s configurable delay between moves).

### 3.3. Mass Processors (`UMassProcessor`)
- **`UMassGridWanderBehaviorProcessor`**:
  - Counts down the idle timer on free units.
  - Selects a random reachable target cell and calls `UMassGridPathfinderLibrary::FindPath()`.
  - Populates `FMassPathFollowingFragment` with local waypoints.
- **`UMassGridMovementProcessor`**:
  - Steps along local path coordinates per frame.
  - Applies local transforms relative to the moving world actor and updates `FTransformFragment`.
- **`UMassStunProcessor`**:
  - Decrements stun timers.
  - Pauses wander decisions and halts movement execution while an entity is stunned.

### 3.4. World Collision & Signal Subsystem
- When two `AWanderingWorldActor` instances collide/overlap:
  - Detect collision event via Actor OnHit/OnOverlap.
  - Send a signal using `UMassSignalSubsystem` targeting all entities belonging to both colliding worlds (`GridID`).
  - Signal triggers immediate 2-second stun on all affected entities.

### 3.5. Visual Representation (`MassActorSubsystem`)
- Connect entities with `MassActorSubsystem` / `FMassActorFragment` or lightweight visual meshes attached to the moving world actor.

---

## 4. Completed Architecture (Part 3: Dynamic Spawning, Rerouting, Interactions & Benchmarks)

### 4.1. Dynamic Building Spawning & Rerouting
- **`AWanderingWorldActor` / `UWorldGridComponent`**:
  - `bEnableDynamicBuildingSpawning`, `DynamicBuildingSpawnInterval`, `MaxDynamicBuildings`, and `SpawnRuntimeBuilding()`.
  - Automatically spawns additional Tetris shapes at runtime and updates visual cube instances.
- **Dynamic Path Rerouting**:
  - `UMassGridPathfinderLibrary::IsPathValid()` checks path validity against current grid state.
  - `UMassGridMovementProcessor` detects blocked waypoints and sets `PathFrag.InvalidateForReroute()`.
  - `UMassGridWanderBehaviorProcessor` immediately re-plans paths around new obstacles using avoidance pathfinding.

### 4.2. Unit Cell Reservation & Avoidance
- **`FWorldGridData`**:
  - `TryIncrementOccupancy()`, `DecrementOccupancy()`, `GetOccupancy()`.
  - Tracks real-time unit density per cell.
- **`UMassGridPathfinderLibrary::FindPathWithAvoidance()`**:
  - Evaluates cell occupancy penalty during A* search to naturally steer units away from crowded bottlenecks.

### 4.3. Building Entry Interactions
- **`FMassUnitInteractionFragment`**:
  - Tracks `InteractionTimeRemaining`, `CargoCount`, `TotalInteractions`, and `InteractingBuildingId`.
- **`UMassGridInteractionProcessor`**:
  - Steps interaction countdown at building entrances (`BuildingEntry`).
  - Increments cargo/service statistics and cycles units back to `Idle` upon completion.
- **`EUnitBehaviorState`**:
  - Clean state transitions: `Idle` $\rightarrow$ `Wandering` $\rightarrow$ `Interacting` $\rightarrow$ `Idle`.

### 4.4. Performance Benchmarks & Testing
- **Automation Test Suite (`Source/MassDemo/Tests/MassPart3Tests.cpp`)**:
  - `MassDemo.Part3.DynamicBuildingSpawning`: Tests runtime obstacle spawning and cell updates.
  - `MassDemo.Part3.DynamicRerouting`: Tests obstacle detection and dynamic rerouting.
  - `MassDemo.Part3.UnitOccupancyAndAvoidance`: Tests occupancy reservations and avoidance pathfinding.
  - `MassDemo.Part3.BuildingEntryInteraction`: Tests full interaction lifecycle and cargo counting.
  - `MassDemo.Part3.DensityAndScaleBenchmark`: Benchmarks 20 units on $10 \times 15$ grid vs. 100 units on $25 \times 25$ grid.

---

## 5. Summary of Completed Phases
- **Part 1**: Core 2D Grid, Tetris Generator, A* Pathfinder, Multi-Grid Subsystem.
- **Part 2**: WanderingWorldActor, Mass Entity Fragments, Movement/Wander/Stun Processors, Collision Signal Subsystem, Visualizers.
- **Part 3**: Dynamic Obstacle Spawning, Dynamic Path Rerouting, Unit Avoidance & Cell Reservation, Building Entry Interactions (Cargo/Service), Scale Benchmarks & Automation Tests.
