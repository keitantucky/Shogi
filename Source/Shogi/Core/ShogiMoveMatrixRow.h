// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ShogiMoveMatrixRow.generated.h"

/**
 * Native mirror of the Blueprint UserDefinedStruct /Game/DataTypes/F_MoveMatrixRow.
 * Column names (X0..X18) are kept identical to the Blueprint struct so existing
 * DataTable assets can have their RowStruct swapped to this type without needing
 * to re-import the CSV data (column-name matching is what UE uses to preserve values).
 *
 * Encoding: row name is "Y{CSV_Row}", column X{CSV_Col} holds the literal string
 * "〇" for a legal relative destination. CSV_Row = dY+9, CSV_Col = dX+9, centering
 * the offset range -9..+9 onto index range 0..18. Illegal cells are NOT reliably
 * empty - after a DataTable's CSV export/reimport round-trip (see docs/GameSpec.md
 * migration checklist) they were found to contain the literal string "×" instead
 * (the original Blueprint struct's per-field default value, only omitted from
 * serialization while unmigrated). Legality checks must therefore test for
 * equality with "〇" specifically, never merely "is this cell non-empty".
 */
USTRUCT(BlueprintType)
struct SHOGI_API FShogiMoveMatrixRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X9;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X11;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X12;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X13;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X14;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X15;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X16;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X17;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X18;

	/** Returns the column value for a 0-18 index (CSV_Col), or nullptr if out of range. */
	const FString* GetColumn(int32 ColumnIndex) const
	{
		switch (ColumnIndex)
		{
			case 0: return &X0;
			case 1: return &X1;
			case 2: return &X2;
			case 3: return &X3;
			case 4: return &X4;
			case 5: return &X5;
			case 6: return &X6;
			case 7: return &X7;
			case 8: return &X8;
			case 9: return &X9;
			case 10: return &X10;
			case 11: return &X11;
			case 12: return &X12;
			case 13: return &X13;
			case 14: return &X14;
			case 15: return &X15;
			case 16: return &X16;
			case 17: return &X17;
			case 18: return &X18;
			default: return nullptr;
		}
	}
};
