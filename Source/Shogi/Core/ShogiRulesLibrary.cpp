// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiRulesLibrary.h"
#include "ShogiMoveMatrixRow.h"
#include "Engine/DataTable.h"

bool UShogiRulesLibrary::IsValidBoardIndex(int32 BoardIndex)
{
	return BoardIndex >= 0 && BoardIndex < BoardCellCount;
}

void UShogiRulesLibrary::IndexToXY(int32 BoardIndex, int32& OutX, int32& OutY)
{
	OutX = BoardIndex % BoardDimension;
	OutY = BoardIndex / BoardDimension;
}

int32 UShogiRulesLibrary::XYToIndex(int32 X, int32 Y)
{
	return Y * BoardDimension + X;
}

bool UShogiRulesLibrary::CheckCanMove(
	const TArray<FShogiPieceData>& BoardArray,
	const FShogiPieceData& SelectedPiece,
	int32 FromIndex,
	int32 ToIndex,
	UDataTable* MoveDataTable)
{
	if (!MoveDataTable || !IsValidBoardIndex(FromIndex) || !IsValidBoardIndex(ToIndex) || FromIndex == ToIndex)
	{
		return false;
	}

	int32 FromX, FromY, ToX, ToY;
	IndexToXY(FromIndex, FromX, FromY);
	IndexToXY(ToIndex, ToX, ToY);

	int32 DeltaX = ToX - FromX;
	int32 DeltaY = ToY - FromY;

	// The movement matrix is authored once, from Sente's forward-facing perspective
	// (confirmed empirically via AShogiBoardManager::DebugLogAllMoveTables: Fu's single
	// legal cell is dY=-1, dX=0, matching Sente's forward direction of -Y). Gote's
	// pieces face the opposite way (also visually rotated 180 degrees in
	// AShogiPiece::UpdateAppearance), so mirror both axes when looking up the table.
	if (SelectedPiece.PlayerSide == EPlayerSide::Gote)
	{
		DeltaX = -DeltaX;
		DeltaY = -DeltaY;
	}

	const int32 CsvCol = DeltaX + (BoardDimension);
	const int32 CsvRow = DeltaY + (BoardDimension);

	if (CsvCol < 0 || CsvCol > 18 || CsvRow < 0 || CsvRow > 18)
	{
		return false;
	}

	const FName RowName(*FString::Printf(TEXT("Y%d"), CsvRow));
	const FShogiMoveMatrixRow* Row = MoveDataTable->FindRow<FShogiMoveMatrixRow>(RowName, TEXT("UShogiRulesLibrary::CheckCanMove"));
	if (!Row)
	{
		return false;
	}

	// Legal cells are marked with the literal "〇" character; illegal cells were found
	// (empirically, via AShogiBoardManager::DebugLogAllMoveTables) to contain the
	// literal "×" character after the DataTable's CSV export/reimport migration, not
	// an empty string as originally assumed - so explicitly require "〇" rather than
	// merely checking for non-empty.
	const FString* Cell = Row->GetColumn(CsvCol);
	return Cell && *Cell == TEXT("〇");
}

bool UShogiRulesLibrary::IsPathClear(
	const TArray<FShogiPieceData>& BoardArray,
	int32 FromIndex,
	int32 ToIndex)
{
	if (!IsValidBoardIndex(FromIndex) || !IsValidBoardIndex(ToIndex))
	{
		return false;
	}

	int32 FromX, FromY, ToX, ToY;
	IndexToXY(FromIndex, FromX, FromY);
	IndexToXY(ToIndex, ToX, ToY);

	const int32 DeltaX = ToX - FromX;
	const int32 DeltaY = ToY - FromY;
	const int32 StepX = (DeltaX > 0) - (DeltaX < 0);
	const int32 StepY = (DeltaY > 0) - (DeltaY < 0);
	const int32 CheckCount = FMath::Max(FMath::Abs(DeltaX), FMath::Abs(DeltaY));

	for (int32 CheckIndex = 1; CheckIndex < CheckCount; ++CheckIndex)
	{
		const int32 CheckX = FromX + StepX * CheckIndex;
		const int32 CheckY = FromY + StepY * CheckIndex;
		const int32 CheckBoardIndex = XYToIndex(CheckX, CheckY);

		if (!BoardArray.IsValidIndex(CheckBoardIndex))
		{
			return false;
		}

		if (BoardArray[CheckBoardIndex].PlayerSide != EPlayerSide::None)
		{
			return false;
		}
	}

	return true;
}

TArray<int32> UShogiRulesLibrary::GetMovableIndices(
	const TArray<FShogiPieceData>& BoardArray,
	const FShogiPieceData& SelectedPiece,
	int32 FromIndex,
	UDataTable* MoveDataTable)
{
	TArray<int32> MovableList;

	if (!MoveDataTable || !IsValidBoardIndex(FromIndex))
	{
		return MovableList;
	}

	for (int32 ToIndex = 0; ToIndex < BoardCellCount; ++ToIndex)
	{
		if (ToIndex == FromIndex)
		{
			continue;
		}

		if (!CheckCanMove(BoardArray, SelectedPiece, FromIndex, ToIndex, MoveDataTable))
		{
			continue;
		}

		if (!IsPathClear(BoardArray, FromIndex, ToIndex))
		{
			continue;
		}

		if (BoardArray.IsValidIndex(ToIndex) && BoardArray[ToIndex].PlayerSide == SelectedPiece.PlayerSide)
		{
			// Occupied by a friendly piece - not a legal destination.
			continue;
		}

		MovableList.Add(ToIndex);
	}

	return MovableList;
}
