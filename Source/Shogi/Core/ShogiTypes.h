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

	/**
	 * Set by the "Fu Rocket" card (see docs/2026-08-14-card-system-phase-a.md). Permanent
	 * once granted - persists through capture/drop like every other field here, since
	 * FShogiPieceData is copied wholesale by ApplyMove's capture logic and ApplyDrop.
	 * Only meaningful for EPieceType::Fu; UShogiRulesLibrary::GetMovableIndices adds an
	 * extra forward+2 destination when this is set.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	bool bHasFuRocketBoost = false;

	/**
	 * Set by the "Petrify Curse" card (see docs/2026-08-14-card-system-phase-b.md). Permanent
	 * once granted, persists through capture/drop like bHasFuRocketBoost.
	 * UShogiRulesLibrary::GetMovableIndices returns an empty list whenever this is set,
	 * checked before (and overriding) any move-granting buff such as Fu Rocket.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	bool bIsPetrified = false;

	/**
	 * Set by the "Temporary Invincibility" card. While true, this piece cannot be captured
	 * (AShogiBoardManager::ApplyMove rejects the capture) and cannot be selected as the
	 * target of any card effect (UShogiCardEffectLibrary target checks). Cleared automatically
	 * by AShogiBoardManager::AdvanceTurn at the start of this piece's owning side's own next
	 * turn (protects through exactly one opposing turn). See docs/2026-08-14-card-system-phase-b.md.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	bool bIsInvincible = false;

	/**
	 * "Rental Reservation" card state (see docs/2026-08-14-card-system-phase-b.md section 2.1).
	 * None = no reservation in effect. Ticks up by 1 each AShogiBoardManager::AdvanceTurn call
	 * while pending; once it reaches 2 on the caster's own turn, ownership flips to the caster
	 * for that turn (bLoanActive=true) and reverts to LoanOriginalOwnerSide at the start of the
	 * following turn, at which point all four fields reset to their defaults.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	EPlayerSide PendingLoanCasterSide = EPlayerSide::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	int32 PendingLoanTurnsElapsed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	bool bLoanActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	EPlayerSide LoanOriginalOwnerSide = EPlayerSide::None;
};
