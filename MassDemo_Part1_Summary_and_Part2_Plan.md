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

## 4. Part 3 Scope & Improvements
- **Performance Benchmarks & Tuning**:
  - Compare density: 10–20 units on $10 \times 15$ grid vs. 50–100+ units on $25 \times 25$ grid.
- **Dynamic Building Spawning**:
  - Spawn additional Tetris blockers over time at runtime, triggering dynamic path rerouting.
- **Unit Avoidance & Interaction**:
  - Unit-to-unit local cell reservation / basic steering.
  - Building entry interactions (cargo drop, service cooldown, etc.).

---

## 5. Quick Reference for Next Session Prompt
> *"We have completed Part 1 of the MASS Demo. Using the architecture in `Grid/MassGrid*.h`, let's begin Part 2: Wandering World Actors, Mass Entity Fragments, Mass Processors (Wander, Movement, Stun), and Collision Signal Handling."*
