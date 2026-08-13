// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShogiTypes.h"
#include "ShogiCardTypes.h"
#include "ShogiCardEffectLibrary.generated.h"

/**
 * Stateless target-validation and board-mutation helpers for the full 10-card set
 * (see docs/2026-08-14-card-system-phase-a.md, docs/2026-08-14-card-system-phase-b.md).
 * Mirrors the UShogiRulesLibrary / UShogiPromotionLibrary pattern: pure functions over a
 * BoardArray, called both by the client (to grey out unplayable cards) and by the server
 * (to re-validate before applying), so the two never disagree about what's legal.
 *
 * Hyena and SelfDestructBomb are deliberately NOT implemented here - Hyena moves an
 * AShogiPiece actor between hand-piece arrays, and SelfDestructBomb destroys actors
 * outright; both need AShogiBoardManager's actor access, so their target checks and
 * application live directly on AShogiBoardManager::ApplyCardEffect. RentalReservation's
 * target check lives here (it's a pure BoardArray predicate), but its application is a
 * single-field write simple enough to inline directly in ApplyCardEffect rather than add
 * a one-line wrapper function here.
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

	/** True if Index holds an opponent-owned, non-invincible piece (any type, King included). */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static bool IsValidPetrifyCurseTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index);

	/** True if Index holds Side's own, non-invincible Fu (promoted or not - see docs/2026-08-14-card-system-phase-b.md 2.3). */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static bool IsValidPositionSwapTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index);

	/** True if Index holds any non-invincible piece (either side, King included) - Megaton Impact has no ownership restriction. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static bool IsValidMegatonImpactTarget(const TArray<FShogiPieceData>& BoardArray, int32 Index);

	/** True if Index holds Side's own, non-invincible piece (any type, King included). */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static bool IsValidSelfDestructBombTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index);

	/** True if Index holds Side's own piece, excluding the King. Re-targeting an already-invincible piece is allowed (no-op). */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static bool IsValidTemporaryInvincibilityTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index);

	/** True if Index holds an opponent-owned, non-invincible piece with no reservation already pending/active. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static bool IsValidRentalReservationTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index);

	/** Permanently erases all movement for the piece at Index. Caller must have validated the target first. */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	static void ApplyPetrifyCurse(UPARAM(ref) TArray<FShogiPieceData>& BoardArray, int32 Index);

	/** Swaps the two pieces at KingIndex/FuIndex in place. Caller must have validated the target first. */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	static void ApplyPositionSwap(UPARAM(ref) TArray<FShogiPieceData>& BoardArray, int32 KingIndex, int32 FuIndex);

	/**
	 * Knocks the piece at Index back up to 3 squares toward its own owner's home edge (see
	 * docs/2026-08-14-card-system-phase-b.md 2.4), stopping short of any occupied square or
	 * the board edge. Caller must have validated the target first. Returns the piece's new
	 * board index (equal to Index if it couldn't move at all) so the caller can sync the
	 * corresponding AShogiPiece actor.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	static int32 ApplyMegatonImpact(UPARAM(ref) TArray<FShogiPieceData>& BoardArray, int32 Index);

	/** Grants temporary capture/card-effect immunity to the piece at Index. Caller must have validated the target first. */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	static void ApplyTemporaryInvincibility(UPARAM(ref) TArray<FShogiPieceData>& BoardArray, int32 Index);

	/** UI display name, transcribed from docs/GameConcept.md section 3. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static FText GetCardDisplayName(ECardType CardType);

	/** UI effect-description text, transcribed from docs/GameConcept.md section 3. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	static FText GetCardDescription(ECardType CardType);
};
