// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiTurnWidgetBase.h"
#include "ShogiGameState.h"

FText UShogiTurnWidgetBase::GetCurrentTurnText() const
{
	const AShogiGameState* GS = GetWorld() ? GetWorld()->GetGameState<AShogiGameState>() : nullptr;
	if (!GS)
	{
		return FText::GetEmpty();
	}

	if (GS->bGameOver)
	{
		switch (GS->Winner)
		{
			case EPlayerSide::Sente:
				return FText::FromString(TEXT("Sente wins!"));
			case EPlayerSide::Gote:
				return FText::FromString(TEXT("Gote wins!"));
			default:
				return FText::GetEmpty();
		}
	}

	switch (GS->CurrentTurn)
	{
		case EPlayerSide::Sente:
			return FText::FromString(GS->bInCheck ? TEXT("Sente - Check!") : TEXT("Sente"));
		case EPlayerSide::Gote:
			return FText::FromString(GS->bInCheck ? TEXT("Gote - Check!") : TEXT("Gote"));
		default:
			return FText::GetEmpty();
	}
}
