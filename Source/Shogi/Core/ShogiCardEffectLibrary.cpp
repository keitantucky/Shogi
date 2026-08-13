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

bool UShogiCardEffectLibrary::IsValidPetrifyCurseTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index)
{
	if (!UShogiRulesLibrary::IsValidBoardIndex(Index) || !BoardArray.IsValidIndex(Index) || Side == EPlayerSide::None)
	{
		return false;
	}

	const FShogiPieceData& Piece = BoardArray[Index];
	const EPlayerSide OpponentSide = (Side == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;
	return Piece.PlayerSide == OpponentSide && !Piece.bIsInvincible;
}

bool UShogiCardEffectLibrary::IsValidPositionSwapTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index)
{
	if (!UShogiRulesLibrary::IsValidBoardIndex(Index) || !BoardArray.IsValidIndex(Index))
	{
		return false;
	}

	const FShogiPieceData& Piece = BoardArray[Index];
	return Piece.PlayerSide == Side && Piece.PieceType == EPieceType::Fu && !Piece.bIsInvincible;
}

bool UShogiCardEffectLibrary::IsValidMegatonImpactTarget(const TArray<FShogiPieceData>& BoardArray, int32 Index)
{
	if (!UShogiRulesLibrary::IsValidBoardIndex(Index) || !BoardArray.IsValidIndex(Index))
	{
		return false;
	}

	const FShogiPieceData& Piece = BoardArray[Index];
	return Piece.PlayerSide != EPlayerSide::None && !Piece.bIsInvincible;
}

bool UShogiCardEffectLibrary::IsValidSelfDestructBombTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index)
{
	if (!UShogiRulesLibrary::IsValidBoardIndex(Index) || !BoardArray.IsValidIndex(Index))
	{
		return false;
	}

	const FShogiPieceData& Piece = BoardArray[Index];
	return Piece.PlayerSide == Side && !Piece.bIsInvincible;
}

bool UShogiCardEffectLibrary::IsValidTemporaryInvincibilityTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index)
{
	if (!UShogiRulesLibrary::IsValidBoardIndex(Index) || !BoardArray.IsValidIndex(Index))
	{
		return false;
	}

	const FShogiPieceData& Piece = BoardArray[Index];
	return Piece.PlayerSide == Side && Piece.PieceType != EPieceType::Ou;
}

bool UShogiCardEffectLibrary::IsValidRentalReservationTarget(const TArray<FShogiPieceData>& BoardArray, EPlayerSide Side, int32 Index)
{
	if (!UShogiRulesLibrary::IsValidBoardIndex(Index) || !BoardArray.IsValidIndex(Index) || Side == EPlayerSide::None)
	{
		return false;
	}

	const FShogiPieceData& Piece = BoardArray[Index];
	const EPlayerSide OpponentSide = (Side == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;
	return Piece.PlayerSide == OpponentSide && !Piece.bIsInvincible && Piece.PendingLoanCasterSide == EPlayerSide::None;
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

		case ECardType::PetrifyCurse:
			for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
			{
				if (IsValidPetrifyCurseTarget(BoardArray, Side, Index))
				{
					return true;
				}
			}
			return false;

		case ECardType::PositionSwap:
			for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
			{
				if (IsValidPositionSwapTarget(BoardArray, Side, Index))
				{
					return true;
				}
			}
			return false;

		case ECardType::MegatonImpact:
			for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
			{
				if (IsValidMegatonImpactTarget(BoardArray, Index))
				{
					return true;
				}
			}
			return false;

		case ECardType::SelfDestructBomb:
			for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
			{
				if (IsValidSelfDestructBombTarget(BoardArray, Side, Index))
				{
					return true;
				}
			}
			return false;

		case ECardType::TemporaryInvincibility:
			for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
			{
				if (IsValidTemporaryInvincibilityTarget(BoardArray, Side, Index))
				{
					return true;
				}
			}
			return false;

		case ECardType::RentalReservation:
			for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
			{
				if (IsValidRentalReservationTarget(BoardArray, Side, Index))
				{
					return true;
				}
			}
			return false;

		default:
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

void UShogiCardEffectLibrary::ApplyPetrifyCurse(TArray<FShogiPieceData>& BoardArray, int32 Index)
{
	if (BoardArray.IsValidIndex(Index))
	{
		BoardArray[Index].bIsPetrified = true;
	}
}

void UShogiCardEffectLibrary::ApplyPositionSwap(TArray<FShogiPieceData>& BoardArray, int32 KingIndex, int32 FuIndex)
{
	if (BoardArray.IsValidIndex(KingIndex) && BoardArray.IsValidIndex(FuIndex) && KingIndex != FuIndex)
	{
		::Swap(BoardArray[KingIndex], BoardArray[FuIndex]);
	}
}

int32 UShogiCardEffectLibrary::ApplyMegatonImpact(TArray<FShogiPieceData>& BoardArray, int32 Index)
{
	if (!BoardArray.IsValidIndex(Index))
	{
		return Index;
	}

	const FShogiPieceData Piece = BoardArray[Index];
	if (Piece.PlayerSide == EPlayerSide::None)
	{
		return Index;
	}

	int32 X, Y;
	UShogiRulesLibrary::IndexToXY(Index, X, Y);
	// Knockback direction is the piece owner's own home edge (retreat), the opposite of that
	// side's forward direction (Sente forward=-Y per UShogiRulesLibrary::CheckCanMove's
	// authoring-perspective comment, so Sente's home/back direction is +Y; Gote mirrors to -Y).
	const int32 StepY = (Piece.PlayerSide == EPlayerSide::Sente) ? 1 : -1;

	int32 FinalY = Y;
	for (int32 Step = 1; Step <= 3; ++Step)
	{
		const int32 CandidateY = Y + StepY * Step;
		if (CandidateY < 0 || CandidateY >= UShogiRulesLibrary::BoardDimension)
		{
			break;
		}

		const int32 CandidateIndex = UShogiRulesLibrary::XYToIndex(X, CandidateY);
		if (BoardArray[CandidateIndex].PlayerSide != EPlayerSide::None)
		{
			break;
		}

		FinalY = CandidateY;
	}

	if (FinalY == Y)
	{
		return Index;
	}

	const int32 FinalIndex = UShogiRulesLibrary::XYToIndex(X, FinalY);
	BoardArray[FinalIndex] = Piece;
	BoardArray[Index] = FShogiPieceData();
	return FinalIndex;
}

void UShogiCardEffectLibrary::ApplyTemporaryInvincibility(TArray<FShogiPieceData>& BoardArray, int32 Index)
{
	if (BoardArray.IsValidIndex(Index))
	{
		BoardArray[Index].bIsInvincible = true;
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
		case ECardType::RentalReservation:		return NSLOCTEXT("ShogiCards", "RentalReservationName", "貸し出し予約");
		case ECardType::PetrifyCurse:			return NSLOCTEXT("ShogiCards", "PetrifyCurseName", "石化の呪い");
		case ECardType::PositionSwap:			return NSLOCTEXT("ShogiCards", "PositionSwapName", "位置シャッフル");
		case ECardType::MegatonImpact:			return NSLOCTEXT("ShogiCards", "MegatonImpactName", "メガトンインパクト");
		case ECardType::SelfDestructBomb:		return NSLOCTEXT("ShogiCards", "SelfDestructBombName", "道連れボム");
		case ECardType::TemporaryInvincibility:return NSLOCTEXT("ShogiCards", "TemporaryInvincibilityName", "一時無敵");
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
		case ECardType::RentalReservation:
			return NSLOCTEXT("ShogiCards", "RentalReservationDesc", "相手の駒1枚を選択し、2ターン後に自分の駒になる。");
		case ECardType::PetrifyCurse:
			return NSLOCTEXT("ShogiCards", "PetrifyCurseDesc", "選択した駒1枚の動きをすべて消去する。");
		case ECardType::PositionSwap:
			return NSLOCTEXT("ShogiCards", "PositionSwapDesc", "自分の「王」と、指定した自分の「歩」の位置を即座に入れ替える。");
		case ECardType::MegatonImpact:
			return NSLOCTEXT("ShogiCards", "MegatonImpactDesc", "指定した駒を後方へ3マスノックバック（吹き飛ばす）。");
		case ECardType::SelfDestructBomb:
			return NSLOCTEXT("ShogiCards", "SelfDestructBombDesc", "自分の駒1枚を爆破消滅させ、周囲1マスにある駒を敵味方問わず全滅させる。");
		case ECardType::TemporaryInvincibility:
			return NSLOCTEXT("ShogiCards", "TemporaryInvincibilityDesc", "このターン、指定した駒（王以外）が相手に取られなくなり、カードの効果も受け付けなくなる。");
		default:
			return FText::GetEmpty();
	}
}
