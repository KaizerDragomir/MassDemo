// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassGridTypes.generated.h"

UENUM(BlueprintType)
enum class EGridCellType : uint8
{
	Free = 0 UMETA(DisplayName = "Free"),
	Blocked = 1 UMETA(DisplayName = "Blocked (Building)"),
	BuildingEntry = 2 UMETA(DisplayName = "Building Entry"),
	Reserved = 3 UMETA(DisplayName = "Reserved/Occupied")
};

USTRUCT(BlueprintType)
struct MASSDEMO_API FGridCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Y = 0;

	FGridCoord() : X(0), Y(0) {}
	FGridCoord(int32 InX, int32 InY) : X(InX), Y(InY) {}

	bool operator==(const FGridCoord& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}

	bool operator!=(const FGridCoord& Other) const
	{
		return !(*this == Other);
	}

	friend uint32 GetTypeHash(const FGridCoord& Coord)
	{
		return HashCombine(::GetTypeHash(Coord.X), ::GetTypeHash(Coord.Y));
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("(%d, %d)"), X, Y);
	}

	static int32 ManhattanDistance(const FGridCoord& A, const FGridCoord& B)
	{
		return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
	}

	static float EuclideanDistance(const FGridCoord& A, const FGridCoord& B)
	{
		return FMath::Sqrt(static_cast<float>(FMath::Square(A.X - B.X) + FMath::Square(A.Y - B.Y)));
	}
};

USTRUCT(BlueprintType)
struct MASSDEMO_API FGridCell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FGridCoord Coord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	EGridCellType CellType = EGridCellType::Free;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 BuildingId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 OccupancyCount = 0;

	bool IsWalkable() const
	{
		return CellType == EGridCellType::Free || CellType == EGridCellType::BuildingEntry;
	}
};

USTRUCT(BlueprintType)
struct MASSDEMO_API FWorldGridData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Width = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Height = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	float CellSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	TArray<FGridCell> Cells;

	void Initialize(int32 InWidth, int32 InHeight, float InCellSize = 100.0f)
	{
		Width = FMath::Max(1, InWidth);
		Height = FMath::Max(1, InHeight);
		CellSize = FMath::Max(10.0f, InCellSize);

		const int32 TotalCells = Width * Height;
		Cells.SetNum(TotalCells);

		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const int32 Index = CoordToIndex(X, Y);
				Cells[Index].Coord = FGridCoord(X, Y);
				Cells[Index].CellType = EGridCellType::Free;
				Cells[Index].BuildingId = INDEX_NONE;
				Cells[Index].OccupancyCount = 0;
			}
		}
	}

	FORCEINLINE bool IsValidCoord(int32 X, int32 Y) const
	{
		return X >= 0 && X < Width && Y >= 0 && Y < Height;
	}

	FORCEINLINE bool IsValidCoord(const FGridCoord& Coord) const
	{
		return IsValidCoord(Coord.X, Coord.Y);
	}

	FORCEINLINE int32 CoordToIndex(int32 X, int32 Y) const
	{
		return Y * Width + X;
	}

	FORCEINLINE int32 CoordToIndex(const FGridCoord& Coord) const
	{
		return CoordToIndex(Coord.X, Coord.Y);
	}

	FORCEINLINE FGridCoord IndexToCoord(int32 Index) const
	{
		return FGridCoord(Index % Width, Index / Width);
	}

	const FGridCell* GetCell(int32 X, int32 Y) const
	{
		if (!IsValidCoord(X, Y))
		{
			return nullptr;
		}
		return &Cells[CoordToIndex(X, Y)];
	}

	FGridCell* GetCell(int32 X, int32 Y)
	{
		if (!IsValidCoord(X, Y))
		{
			return nullptr;
		}
		return &Cells[CoordToIndex(X, Y)];
	}

	const FGridCell* GetCell(const FGridCoord& Coord) const
	{
		return GetCell(Coord.X, Coord.Y);
	}

	FGridCell* GetCell(const FGridCoord& Coord)
	{
		return GetCell(Coord.X, Coord.Y);
	}

	bool IsWalkable(const FGridCoord& Coord) const
	{
		const FGridCell* Cell = GetCell(Coord);
		return Cell != nullptr && Cell->IsWalkable();
	}

	FVector CellToLocalPosition(const FGridCoord& Coord, float ZOffset = 0.0f) const
	{
		// Center the grid around origin (0, 0) in actor local space
		const float OriginX = -(Width * CellSize) * 0.5f + (CellSize * 0.5f);
		const float OriginY = -(Height * CellSize) * 0.5f + (CellSize * 0.5f);

		const float LocalX = OriginX + Coord.X * CellSize;
		const float LocalY = OriginY + Coord.Y * CellSize;

		return FVector(LocalX, LocalY, ZOffset);
	}

	FGridCoord LocalPositionToCell(const FVector& LocalPos) const
	{
		const float OriginX = -(Width * CellSize) * 0.5f;
		const float OriginY = -(Height * CellSize) * 0.5f;

		const int32 X = FMath::FloorToInt((LocalPos.X - OriginX) / CellSize);
		const int32 Y = FMath::FloorToInt((LocalPos.Y - OriginY) / CellSize);

		return FGridCoord(FMath::Clamp(X, 0, Width - 1), FMath::Clamp(Y, 0, Height - 1));
	}

	bool GetRandomWalkableCell(FGridCoord& OutCoord) const
	{
		TArray<FGridCoord> WalkableCoords;
		WalkableCoords.Reserve(Cells.Num());

		for (const FGridCell& Cell : Cells)
		{
			if (Cell.IsWalkable())
			{
				WalkableCoords.Add(Cell.Coord);
			}
		}

		if (WalkableCoords.Num() == 0)
		{
			return false;
		}

		const int32 RandIndex = FMath::RandRange(0, WalkableCoords.Num() - 1);
		OutCoord = WalkableCoords[RandIndex];
		return true;
	}
};
