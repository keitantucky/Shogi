// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShogiTypes.h"
#include "ShogiPromotionLibrary.generated.h"

/**
 * Outcome of evaluating a single from->to board move against Shogi's promotion rules.
 * Brand-new, C++-only enum, unrelated to the Blueprint UUserDefinedEnum assets
 * E_PieceType / E_PlayerSide - no existing enum asset is touched or migrated.
 */
UENUM(BlueprintType)
enum class EShogiPromotionOutcome : uint8
{
	NoPromotion			UMETA(DisplayName = "No Promotion"),
	OptionalPromotion	UMETA(DisplayName = "Optional Promotion"),
	ForcedPromotion		UMETA(DisplayName = "Forced Promotion")
};

/**
 * Stateless promotion rule-decision helpers for Shogi.
 *
 * Board orientation assumption: all functions take GridY (0-8) as raw row indices
 * exactly as stored in BP_BoardManager's BoardArray, plus an explicit bIsSente flag
 * for the piece's owner. Internally this library assumes a single fixed mapping of
 * "which raw row is whose home rank" (see ShogiBoardConvention in the .cpp). This
 * assumption is UNVERIFIED against BP_BoardManager - confirm in-editor (see
 * docs/GameSpec.md) before trusting these in the real move flow.
 */
UCLASS()
class SHOGI_API UShogiPromotionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Diagnostic/verification helper: how many rows "forward" GridY is from the far
	 * edge of the board for a piece owned by bIsSente (0 = the very last rank).
	 * Wire to a Print String during in-editor testing to confirm the row-orientation
	 * convention before trusting the other functions.
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Promotion|Debug")
	static int32 GetForwardDistanceFromFarEdge(int32 GridY, bool bIsSente);

	/** True if GridY lies within the 3-row promotion zone for a piece owned by bIsSente. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Promotion")
	static bool IsInPromotionZone(int32 GridY, bool bIsSente);

	/**
	 * Optional-promotion eligibility: true if the piece is allowed to promote on this
	 * move (but is not required to). Does NOT include the forced-promotion check.
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Promotion")
	static bool CanPromote(
		int32 FromGridY,
		int32 ToGridY,
		bool bIsSente,
		bool bIsPromotablePieceType,
		bool bIsAlreadyPromoted,
		bool bIsDrop);

	/**
	 * Forced-promotion check: true if leaving the piece unpromoted at ToGridY would be
	 * illegal (pawn/lance on the last rank, knight on the last two ranks).
	 * ForcedPromotionRowDepth: 0 = never forced, 1 = forced on last 1 rank (pawn/lance),
	 * 2 = forced on last 2 ranks (knight).
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Promotion")
	static bool MustPromote(
		int32 ToGridY,
		bool bIsSente,
		int32 ForcedPromotionRowDepth,
		bool bIsDrop);

	/**
	 * Convenience aggregate: the single call site recommended for BP_MyPlayerController's
	 * move-execution flow. Combines MustPromote + CanPromote into one Switch-able result.
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Promotion")
	static EShogiPromotionOutcome EvaluatePromotion(
		int32 FromGridY,
		int32 ToGridY,
		bool bIsSente,
		bool bIsPromotablePieceType,
		bool bIsAlreadyPromoted,
		bool bIsDrop,
		int32 ForcedPromotionRowDepth);

	/**
	 * Looks up whether PieceType can ever promote, and its forced-promotion row depth
	 * (0 = never forced, 1 = pawn/lance on the last rank, 2 = knight on the last two
	 * ranks). King and Gold are never promotable. Replaces the "Switch on E_PieceType"
	 * table that would otherwise need to be built in Blueprint.
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Promotion")
	static void GetPromotionRuleForPieceType(
		EPieceType PieceType,
		bool& bOutIsPromotable,
		int32& OutForcedPromotionRowDepth);
};
