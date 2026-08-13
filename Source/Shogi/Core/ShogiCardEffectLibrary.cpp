// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiCardEffectLibrary.h"
#include "ShogiRulesLibrary.h"
#include "ShogiPromotionLibrary.h"

bool UShogiCardEffectLibrary::IsValidFuRocketTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index)
{
	if (!UShogiRulesLibrary::IsValidBoardIndex(Index) || !BoardArray.IsValidIndex(Index))
	{
		return false;
	}

	const FShogiPieceData& Piece = BoardArray[Index];
	return Piece.PlayerSide == Side && Piece.PieceType == EPieceType::Fu && !Piece.bIsPromoted;
}

bool UShogiCardEffectLibrary::IsValidInstantAwakeningTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index)
{
	if (!UShogiRulesLibrary::IsValidBoardIndex(Index) || !BoardArray.IsValidIndex(Index))
	{
		return false;
	}

	const FShogiPieceData& Piece = BoardArray[Index];
	if (Piece.PlayerSide != Side || Piece.bIsPromoted)
	{
		return false;
	}

	bool bIsPromotable = false;
	int32 ForcedDepth = 0;
	UShogiPromotionLibrary::GetPromotionRuleForPieceType(Piece.PieceType, bIsPromotable, ForcedDepth);
	return bIsPromotable;
}

bool UShogiCardEffectLibrary::HasValidTarget(
	ECardType CardType,
	const TArray<FShogiPieceData>& BoardArray,
	const TArray<FShogiPieceData>& OpponentCapturedPieces,
	EPlayerSide Side)
{
	switch (CardType)
	{
		case ECardType::FuRocket:
			for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
			{
				if (IsValidFuRocketTarget(BoardArray, Side, Index))
				{
					return true;
				}
			}
			return false;

		case ECardType::InstantAwakening:
			for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
			{
				if (IsValidInstantAwakeningTarget(BoardArray, Side, Index))
				{
					return true;
				}
			}
			return false;

		case ECardType::TenpenChii:
			// Board-wide effect, no target selection needed - always playable.
			return true;

		case ECardType::Hyena:
			return OpponentCapturedPieces.Num() > 0;

		default:
			// Phase B cards have no effect logic yet.
			return false;
	}
}

void UShogiCardEffectLibrary::ApplyFuRocket(TArray<FShogiPieceData>& BoardArray, int32 Index)
{
	if (BoardArray.IsValidIndex(Index))
	{
		BoardArray[Index].bHasFuRocketBoost = true;
	}
}

void UShogiCardEffectLibrary::ApplyInstantAwakening(TArray<FShogiPieceData>& BoardArray, int32 Index)
{
	if (BoardArray.IsValidIndex(Index))
	{
		BoardArray[Index].bIsPromoted = true;
	}
}

void UShogiCardEffectLibrary::ApplyTenpenChii(TArray<FShogiPieceData>& BoardArray)
{
	for (FShogiPieceData& Piece : BoardArray)
	{
		if (Piece.PlayerSide == EPlayerSide::None)
		{
			continue;
		}

		bool bIsPromotable = false;
		int32 ForcedDepth = 0;
		UShogiPromotionLibrary::GetPromotionRuleForPieceType(Piece.PieceType, bIsPromotable, ForcedDepth);
		if (bIsPromotable)
		{
			Piece.bIsPromoted = !Piece.bIsPromoted;
		}
	}
}

FText UShogiCardEffectLibrary::GetCardDisplayName(ECardType CardType)
{
	switch (CardType)
	{
		case ECardType::FuRocket:				return NSLOCTEXT("ShogiCards", "FuRocketName", "歩兵ロケット");
		case ECardType::InstantAwakening:		return NSLOCTEXT("ShogiCards", "InstantAwakeningName", "即時覚醒");
		case ECardType::TenpenChii:			return NSLOCTEXT("ShogiCards", "TenpenChiiName", "天変地異");
		case ECardType::Hyena:					return NSLOCTEXT("ShogiCards", "HyenaName", "ハイエナ");
		default:								return FText::GetEmpty();
	}
}

FText UShogiCardEffectLibrary::GetCardDescription(ECardType CardType)
{
	switch (CardType)
	{
		case ECardType::FuRocket:
			return NSLOCTEXT("ShogiCards", "FuRocketDesc", "選択した「歩」の移動可能マスに「前+2マス」を追加する。");
		case ECardType::InstantAwakening:
			return NSLOCTEXT("ShogiCards", "InstantAwakeningDesc", "選択した自分の駒1枚を、条件を無視してその場で即座に「成る」。");
		case ECardType::TenpenChii:
			return NSLOCTEXT("ShogiCards", "TenpenChiiDesc", "盤面上にある「成れる状態の駒」全員の成り状態を反転（通常↔成り）。");
		case ECardType::Hyena:
			return NSLOCTEXT("ShogiCards", "HyenaDesc", "相手の駒台から一枚選択し、自分の駒台に加える。");
		default:
			return FText::GetEmpty();
	}
}
