// Copyright Epic Games, Inc. All Rights Reserved.

#include "Grid/MassGridPathfinder.h"

namespace
{
	struct FAStarNode
	{
		int32 Index = INDEX_NONE;
		FGridCoord Coord;
		float GScore = TNumericLimits<float>::Max();
		float FScore = TNumericLimits<float>::Max();

		bool operator<(const FAStarNode& Other) const
		{
			// Greater for Min-Heap in Unreal
			return FScore > Other.FScore;
		}
	};
}

bool UMassGridPathfinderLibrary::FindPath(
	const FWorldGridData& Grid, 
	const FGridCoord& Start, 
	const FGridCoord& Goal, 
	TArray<FGridCoord>& OutPath)
{
	OutPath.Reset();

	if (!Grid.IsValidCoord(Start) || !Grid.IsValidCoord(Goal))
	{
		return false;
	}

	if (!Grid.IsWalkable(Start) || !Grid.IsWalkable(Goal))
	{
		return false;
	}

	if (Start == Goal)
	{
		OutPath.Add(Start);
		return true;
	}

	const int32 TotalCells = Grid.Width * Grid.Height;
	TArray<float> GScore;
	GScore.Init(TNumericLimits<float>::Max(), TotalCells);

	TArray<int32> CameFrom;
	CameFrom.Init(INDEX_NONE, TotalCells);

	TArray<bool> ClosedSet;
	ClosedSet.Init(false, TotalCells);

	TArray<FAStarNode> OpenSetHeap;

	const int32 StartIndex = Grid.CoordToIndex(Start);
	const int32 GoalIndex = Grid.CoordToIndex(Goal);

	GScore[StartIndex] = 0.0f;
	const float InitialH = static_cast<float>(FGridCoord::ManhattanDistance(Start, Goal));

	FAStarNode StartNode;
	StartNode.Index = StartIndex;
	StartNode.Coord = Start;
	StartNode.GScore = 0.0f;
	StartNode.FScore = InitialH;

	auto MinHeapPred = [](const FAStarNode& A, const FAStarNode& B)
	{
		if (FMath::IsNearlyEqual(A.FScore, B.FScore))
		{
			return A.GScore > B.GScore;
		}
		return A.FScore < B.FScore;
	};

	OpenSetHeap.HeapPush(StartNode, MinHeapPred);

	const FGridCoord Directions[4] = {
		FGridCoord(1, 0),
		FGridCoord(-1, 0),
		FGridCoord(0, 1),
		FGridCoord(0, -1)
	};

	bool bFound = false;

	while (OpenSetHeap.Num() > 0)
	{
		FAStarNode CurrentNode;
		OpenSetHeap.HeapPop(CurrentNode, MinHeapPred);

		if (CurrentNode.Index == GoalIndex)
		{
			bFound = true;
			break;
		}

		if (ClosedSet[CurrentNode.Index])
		{
			continue;
		}

		ClosedSet[CurrentNode.Index] = true;

		for (const FGridCoord& Dir : Directions)
		{
			const FGridCoord NeighborCoord(CurrentNode.Coord.X + Dir.X, CurrentNode.Coord.Y + Dir.Y);
			if (!Grid.IsValidCoord(NeighborCoord))
			{
				continue;
			}

			if (!Grid.IsWalkable(NeighborCoord))
			{
				continue;
			}

			const int32 NeighborIndex = Grid.CoordToIndex(NeighborCoord);
			if (ClosedSet[NeighborIndex])
			{
				continue;
			}

			const float TentativeG = CurrentNode.GScore + 1.0f;
			if (TentativeG < GScore[NeighborIndex])
			{
				CameFrom[NeighborIndex] = CurrentNode.Index;
				GScore[NeighborIndex] = TentativeG;

				const float HScore = static_cast<float>(FGridCoord::ManhattanDistance(NeighborCoord, Goal));
				FAStarNode NeighborNode;
				NeighborNode.Index = NeighborIndex;
				NeighborNode.Coord = NeighborCoord;
				NeighborNode.GScore = TentativeG;
				NeighborNode.FScore = TentativeG + HScore;

				OpenSetHeap.HeapPush(NeighborNode, MinHeapPred);
			}
		}
	}

	if (!bFound)
	{
		return false;
	}

	// Reconstruct path
	int32 Curr = GoalIndex;
	while (Curr != INDEX_NONE)
	{
		OutPath.Add(Grid.IndexToCoord(Curr));
		if (Curr == StartIndex)
		{
			break;
		}
		Curr = CameFrom[Curr];
	}

	Algo::Reverse(OutPath);
	return true;
}

bool UMassGridPathfinderLibrary::FindPathWithAvoidance(
	const FWorldGridData& Grid, 
	const FGridCoord& Start, 
	const FGridCoord& Goal, 
	TArray<FGridCoord>& OutPath,
	float OccupancyWeight,
	int32 MaxCellOccupancy)
{
	OutPath.Reset();

	if (!Grid.IsValidCoord(Start) || !Grid.IsValidCoord(Goal))
	{
		return false;
	}

	if (!Grid.IsWalkable(Start) || !Grid.IsWalkable(Goal))
	{
		return false;
	}

	if (Start == Goal)
	{
		OutPath.Add(Start);
		return true;
	}

	const int32 TotalCells = Grid.Width * Grid.Height;
	TArray<float> GScore;
	GScore.Init(TNumericLimits<float>::Max(), TotalCells);

	TArray<int32> CameFrom;
	CameFrom.Init(INDEX_NONE, TotalCells);

	TArray<bool> ClosedSet;
	ClosedSet.Init(false, TotalCells);

	TArray<FAStarNode> OpenSetHeap;

	const int32 StartIndex = Grid.CoordToIndex(Start);
	const int32 GoalIndex = Grid.CoordToIndex(Goal);

	GScore[StartIndex] = 0.0f;
	const float InitialH = static_cast<float>(FGridCoord::ManhattanDistance(Start, Goal));

	FAStarNode StartNode;
	StartNode.Index = StartIndex;
	StartNode.Coord = Start;
	StartNode.GScore = 0.0f;
	StartNode.FScore = InitialH;

	auto MinHeapPred = [](const FAStarNode& A, const FAStarNode& B)
	{
		if (FMath::IsNearlyEqual(A.FScore, B.FScore))
		{
			return A.GScore > B.GScore;
		}
		return A.FScore < B.FScore;
	};

	OpenSetHeap.HeapPush(StartNode, MinHeapPred);

	const FGridCoord Directions[4] = {
		FGridCoord(1, 0),
		FGridCoord(-1, 0),
		FGridCoord(0, 1),
		FGridCoord(0, -1)
	};

	bool bFound = false;

	while (OpenSetHeap.Num() > 0)
	{
		FAStarNode CurrentNode;
		OpenSetHeap.HeapPop(CurrentNode, MinHeapPred);

		if (CurrentNode.Index == GoalIndex)
		{
			bFound = true;
			break;
		}

		if (ClosedSet[CurrentNode.Index])
		{
			continue;
		}

		ClosedSet[CurrentNode.Index] = true;

		for (const FGridCoord& Dir : Directions)
		{
			const FGridCoord NeighborCoord(CurrentNode.Coord.X + Dir.X, CurrentNode.Coord.Y + Dir.Y);
			if (!Grid.IsValidCoord(NeighborCoord))
			{
				continue;
			}

			if (!Grid.IsWalkable(NeighborCoord))
			{
				continue;
			}

			const int32 NeighborOccupancy = Grid.GetOccupancy(NeighborCoord);
			// Do not block goal even if occupied
			if (NeighborCoord != Goal && MaxCellOccupancy > 0 && NeighborOccupancy >= MaxCellOccupancy)
			{
				continue;
			}

			const int32 NeighborIndex = Grid.CoordToIndex(NeighborCoord);
			if (ClosedSet[NeighborIndex])
			{
				continue;
			}

			const float MoveCost = 1.0f + (NeighborOccupancy * OccupancyWeight);
			const float TentativeG = CurrentNode.GScore + MoveCost;
			if (TentativeG < GScore[NeighborIndex])
			{
				CameFrom[NeighborIndex] = CurrentNode.Index;
				GScore[NeighborIndex] = TentativeG;

				const float HScore = static_cast<float>(FGridCoord::ManhattanDistance(NeighborCoord, Goal));
				FAStarNode NeighborNode;
				NeighborNode.Index = NeighborIndex;
				NeighborNode.Coord = NeighborCoord;
				NeighborNode.GScore = TentativeG;
				NeighborNode.FScore = TentativeG + HScore;

				OpenSetHeap.HeapPush(NeighborNode, MinHeapPred);
			}
		}
	}

	if (!bFound)
	{
		// Fallback to standard path if strictly avoided path was not found
		return FindPath(Grid, Start, Goal, OutPath);
	}

	// Reconstruct path
	int32 Curr = GoalIndex;
	while (Curr != INDEX_NONE)
	{
		OutPath.Add(Grid.IndexToCoord(Curr));
		if (Curr == StartIndex)
		{
			break;
		}
		Curr = CameFrom[Curr];
	}

	Algo::Reverse(OutPath);
	return true;
}

bool UMassGridPathfinderLibrary::IsPathValid(const FWorldGridData& Grid, const TArray<FGridCoord>& Path)
{
	if (Path.Num() == 0)
	{
		return false;
	}

	for (const FGridCoord& Coord : Path)
	{
		if (!Grid.IsValidCoord(Coord) || !Grid.IsWalkable(Coord))
		{
			return false;
		}
	}

	return true;
}

void UMassGridPathfinderLibrary::ConvertPathToLocalPositions(
	const FWorldGridData& Grid, 
	const TArray<FGridCoord>& Path, 
	TArray<FVector>& OutLocalPositions, 
	float ZOffset)
{
	OutLocalPositions.Reset(Path.Num());
	for (const FGridCoord& Coord : Path)
	{
		OutLocalPositions.Add(Grid.CellToLocalPosition(Coord, ZOffset));
	}
}
