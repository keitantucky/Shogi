// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ShogiBoardSetupRow.generated.h"

/**
 * Native mirror of the Blueprint UserDefinedStruct /Game/DataTypes/F_BoardRowInput.
 * Column names (X0..X8) match the Blueprint struct so the existing initial-board
 * DataTable can have its RowStruct swapped to this type without re-importing CSV.
 *
 * Row names are "Y0".."Y8" (rank), columns X0..X8 (file). Each cell holds either the
 * literal string "None" (empty square) or a compound identifier "{PlayerSide}_{PieceType}"
 * e.g. "Sente_Fu", "Gote_Kyou", using the same internal enumerator names as EPlayerSide/
 * EPieceType (Fu, Kyou, Kei, Gin, Kin, Kaku, Hisha, Ou).
 */
USTRUCT(BlueprintType)
struct SHOGI_API FShogiBoardSetupRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X0 = TEXT("None");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X1 = TEXT("None");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X2 = TEXT("None");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X3 = TEXT("None");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X4 = TEXT("None");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X5 = TEXT("None");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X6 = TEXT("None");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X7 = TEXT("None");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	FString X8 = TEXT("None");

	/** Returns the column value for a 0-8 file index, or nullptr if out of range. */
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
			default: return nullptr;
		}
	}
};
