// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShogiTypes.h"
#include "ShogiRulesLibrary.generated.h"

class UDataTable;

/**
 * Native replacement for the Blueprint BlueprintFunctionLibrary BFL_ShogiRules.
 *
 * Board convention (reverse-engineered from the existing Blueprint/DataTable assets,
 * see docs/GameSpec.md): the board is a flat 9x9 array, BoardIndex = Y*9 + X, where
 * X is the file (0-8) and Y is the rank (0-8). Move legality is driven by a per-piece
 * DataTable (row struct FShogiMoveMatrixRow) encoding a full 19x19 reachability grid
 * centered on the moving piece (relative offset -9..+9 mapped to index 0..18): row
 * name "Y{dY+9}", column "X{dX+9}" holds "〇" for a legal relative destination and
 * an empty string otherwise. Sliding pieces (Lance/Bishop/Rook and their promoted
 * forms) have every square along their line pre-marked in the table; actual blocking
 * by other pieces is filtered separately via IsPathClear.
 */
UCLASS()
class SHOGI_API UShogiRulesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 BoardDimension = 9;
	static constexpr int32 BoardCellCount = BoardDimension * BoardDimension;

	UFUNCTION(BlueprintPure, Category = "Shogi|Rules")
	static bool IsValidBoardIndex(int32 BoardIndex);

	UFUNCTION(BlueprintPure, Category = "Shogi|Rules")
	static void IndexToXY(int32 BoardIndex, int32& OutX, int32& OutY);

	UFUNCTION(BlueprintPure, Category = "Shogi|Rules")
	static int32 XYToIndex(int32 X, int32 Y);

	/**
	 * True if the relative offset from FromIndex to ToIndex is a legal destination
	 * for SelectedPiece according to MoveDataTable, ignoring board occupancy/blocking
	 * (see IsPathClear for that). Does not check whether the destination holds a
	 * friendly piece - callers combine this with occupancy checks (see GetMovableIndices).
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Rules")
	static bool CheckCanMove(
		const TArray<FShogiPieceData>& BoardArray,
		const FShogiPieceData& SelectedPiece,
		int32 FromIndex,
		int32 ToIndex,
		UDataTable* MoveDataTable);

	/**
	 * True if every square strictly between FromIndex and ToIndex (exclusive of both
	 * ends) is unoccupied. For adjacent squares (distance 1 in every axis) this is
	 * trivially true. FromIndex/ToIndex must lie on a straight line (horizontal,
	 * vertical, or diagonal) - the moving-piece DataTable already guarantees this for
	 * any offset it marks legal.
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Rules")
	static bool IsPathClear(
		const TArray<FShogiPieceData>& BoardArray,
		int32 FromIndex,
		int32 ToIndex);

	/**
	 * Full list of legal destination board indices for SelectedPiece at FromIndex:
	 * combines CheckCanMove + IsPathClear, and excludes squares occupied by a friendly
	 * piece (squares occupied by an enemy piece are included, representing a capture).
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Rules")
	static TArray<int32> GetMovableIndices(
		const TArray<FShogiPieceData>& BoardArray,
		const FShogiPieceData& SelectedPiece,
		int32 FromIndex,
		UDataTable* MoveDataTable);
};
