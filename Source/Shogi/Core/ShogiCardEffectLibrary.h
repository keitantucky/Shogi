// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShogiTypes.h"
#include "ShogiCardTypes.h"
#include "ShogiCardEffectLibrary.generated.h"

/**
 * Stateless target-validation and board-mutation helpers for the Phase A card set
 * (see docs/2026-08-14-card-system-phase-a.md). Mirrors the UShogiRulesLibrary /
 * UShogiPromotionLibrary pattern: pure functions over a BoardArray, called both by
 * the client (to grey out unplayable cards) and by the server (to re-validate before
 * applying), so the two never disagree about what's legal.
 *
 * Hyena is deliberately NOT implemented here - it moves an AShogiPiece actor between
 * hand-piece arrays, which only AShogiBoardManager has access to, so its target check
 * and application both live directly on AShogiBoardManager::ApplyCardEffect.
 */
UCLASS()
class SHOGI_API UShogiCardEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * True if CardType currently has at least one legal target/use for Side given the
	 * board and (for Hyena) the opponent's captured-piece pool. Used to grey out cards
	 * with no valid target in the hand UI, and re-checked server-side before applying.
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static bool HasValidTarget(
		ECardType CardType,
		const TArray<FShogiPieceData>& BoardArray,
		const TArray<FShogiPieceData>& OpponentCapturedPieces,
		EPlayerSide Side);

	/** True if Index holds Side's own unpromoted Fu - the only legal Fu Rocket target. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static bool IsValidFuRocketTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index);

	/** True if Index holds Side's own unpromoted piece of a promotable type (Fu/Kyou/Kei/Gin/Kaku/Hisha). */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static bool IsValidInstantAwakeningTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index);

	/** Grants the permanent forward+2 move to the Fu at Index. Caller must have validated the target first. */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	static void ApplyFuRocket(UPARAM(ref) TArray<FShogiPieceData>& BoardArray, int32 Index);

	/** Promotes the piece at Index in place, ignoring zone/rank rules. Caller must have validated the target first. */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	static void ApplyInstantAwakening(UPARAM(ref) TArray<FShogiPieceData>& BoardArray, int32 Index);

	/** Flips bIsPromoted for every promotable-type piece currently on the board, both sides. */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	static void ApplyTenpenChii(UPARAM(ref) TArray<FShogiPieceData>& BoardArray);

	/** UI display name, transcribed from docs/GameConcept.md section 3. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static FText GetCardDisplayName(ECardType CardType);

	/** UI effect-description text, transcribed from docs/GameConcept.md section 3. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static FText GetCardDescription(ECardType CardType);
};
