// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiPromotionLibrary.h"

namespace ShogiBoardConvention
{
	constexpr int32 BoardSize = 9;			// BoardArray is 9x9
	constexpr int32 PromotionZoneDepth = 3;	// last 3 ranks

	// Confirmed from the initial-board DataTable's actual data (Shogi_BoardState_-_初期盤面):
	// Sente's pieces occupy Y6-Y8 (Ou at Y8), Gote's pieces occupy Y0-Y2 (Ou at Y0).
	// true  => GridY == 0 is Sente's home rank (Sente advances toward GridY == 8)
	// false => GridY == 0 is Gote's home rank  (Sente advances toward GridY == 0)
	constexpr bool bSenteHomeIsRowZero = false;
}

int32 UShogiPromotionLibrary::GetForwardDistanceFromFarEdge(int32 GridY, bool bIsSente)
{
	using namespace ShogiBoardConvention;
	const bool bMovesTowardHighRow = (bIsSente == bSenteHomeIsRowZero);
	return bMovesTowardHighRow ? (BoardSize - 1 - GridY) : GridY;
}

bool UShogiPromotionLibrary::IsInPromotionZone(int32 GridY, bool bIsSente)
{
	return GetForwardDistanceFromFarEdge(GridY, bIsSente) < ShogiBoardConvention::PromotionZoneDepth;
}

bool UShogiPromotionLibrary::CanPromote(int32 FromGridY, int32 ToGridY, bool bIsSente,
	bool bIsPromotablePieceType, bool bIsAlreadyPromoted, bool bIsDrop)
{
	if (bIsDrop || bIsAlreadyPromoted || !bIsPromotablePieceType)
	{
		return false;
	}
	return IsInPromotionZone(FromGridY, bIsSente) || IsInPromotionZone(ToGridY, bIsSente);
}

bool UShogiPromotionLibrary::MustPromote(int32 ToGridY, bool bIsSente,
	int32 ForcedPromotionRowDepth, bool bIsDrop)
{
	if (bIsDrop || ForcedPromotionRowDepth <= 0)
	{
		return false;
	}
	return GetForwardDistanceFromFarEdge(ToGridY, bIsSente) < ForcedPromotionRowDepth;
}

EShogiPromotionOutcome UShogiPromotionLibrary::EvaluatePromotion(int32 FromGridY, int32 ToGridY,
	bool bIsSente, bool bIsPromotablePieceType, bool bIsAlreadyPromoted, bool bIsDrop,
	int32 ForcedPromotionRowDepth)
{
	if (bIsDrop || bIsAlreadyPromoted || !bIsPromotablePieceType)
	{
		return EShogiPromotionOutcome::NoPromotion;
	}
	if (MustPromote(ToGridY, bIsSente, ForcedPromotionRowDepth, bIsDrop))
	{
		return EShogiPromotionOutcome::ForcedPromotion;
	}
	if (CanPromote(FromGridY, ToGridY, bIsSente, bIsPromotablePieceType, bIsAlreadyPromoted, bIsDrop))
	{
		return EShogiPromotionOutcome::OptionalPromotion;
	}
	return EShogiPromotionOutcome::NoPromotion;
}

void UShogiPromotionLibrary::GetPromotionRuleForPieceType(
	EPieceType PieceType, bool& bOutIsPromotable, int32& OutForcedPromotionRowDepth)
{
	switch (PieceType)
	{
		case EPieceType::Fu:
		case EPieceType::Kyou:
			bOutIsPromotable = true;
			OutForcedPromotionRowDepth = 1;
			break;
		case EPieceType::Kei:
			bOutIsPromotable = true;
			OutForcedPromotionRowDepth = 2;
			break;
		case EPieceType::Gin:
		case EPieceType::Kaku:
		case EPieceType::Hisha:
			bOutIsPromotable = true;
			OutForcedPromotionRowDepth = 0;
			break;
		case EPieceType::Kin:
		case EPieceType::Ou:
		case EPieceType::None:
		default:
			bOutIsPromotable = false;
			OutForcedPromotionRowDepth = 0;
			break;
	}
}
