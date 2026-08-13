// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShogiTypes.generated.h"

/**
 * Native mirror of the Blueprint UserDefinedEnum /Game/DataTypes/E_PieceType.
 * Ordinal values (0-8) are intentionally kept identical to the Blueprint enum so
 * that data authored against the old enum (e.g. row-name construction) stays valid:
 * 0=Fu(Pawn) 1=Kyou(Lance) 2=Kei(Knight) 3=Gin(Silver) 4=Kin(Gold) 5=Kaku(Bishop)
 * 6=Hisha(Rook) 7=Ou(King) 8=None(empty square sentinel).
 */
UENUM(BlueprintType)
enum class EPieceType : uint8
{
	Fu		UMETA(DisplayName = "Fu (Pawn)"),
	Kyou	UMETA(DisplayName = "Kyou (Lance)"),
	Kei		UMETA(DisplayName = "Kei (Knight)"),
	Gin		UMETA(DisplayName = "Gin (Silver General)"),
	Kin		UMETA(DisplayName = "Kin (Gold General)"),
	Kaku	UMETA(DisplayName = "Kaku (Bishop)"),
	Hisha	UMETA(DisplayName = "Hisha (Rook)"),
	Ou		UMETA(DisplayName = "Ou (King)"),
	None	UMETA(DisplayName = "None (Empty Square)")
};

/**
 * Native mirror of the Blueprint UserDefinedEnum /Game/DataTypes/E_PlayerSide.
 * Ordinal values kept identical to the Blueprint enum: 0=Sente 1=Gote 2=None.
 */
UENUM(BlueprintType)
enum class EPlayerSide : uint8
{
	Sente	UMETA(DisplayName = "Sente"),
	Gote	UMETA(DisplayName = "Gote"),
	None	UMETA(DisplayName = "None (Empty Square)")
};

/**
 * Native mirror of the Blueprint UserDefinedStruct /Game/DataTypes/F_PieceData.
 */
USTRUCT(BlueprintType)
struct SHOGI_API FShogiPieceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	EPieceType PieceType = EPieceType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	EPlayerSide PlayerSide = EPlayerSide::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	bool bIsPromoted = false;
};
