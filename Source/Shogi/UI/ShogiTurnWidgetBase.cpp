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

FText UShogiTurnWidgetBase::GetPhaseTimerText() const
{
	const AShogiGameState* GS = GetWorld() ? GetWorld()->GetGameState<AShogiGameState>() : nullptr;
	if (!GS || GS->bGameOver || GS->CurrentPhaseTimerEndTime < 0.f)
	{
		return FText::GetEmpty();
	}

	const int32 SecondsRemaining = FMath::CeilToInt(GS->GetPhaseTimerSecondsRemaining());
	return FText::AsNumber(SecondsRemaining);
}

ESlateVisibility UShogiTurnWidgetBase::GetPhaseTimerVisibility() const
{
	return GetPhaseTimerText().IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::Visible;
}

FText UShogiTurnWidgetBase::GetTimeBankDisplayText(EPlayerSide Side) const
{
	const AShogiGameState* GS = GetWorld() ? GetWorld()->GetGameState<AShogiGameState>() : nullptr;
	if (!GS)
	{
		return FText::GetEmpty();
	}

	// Matches AShogiBoardManager::CardPhaseTimeoutSeconds/MovePhaseTimeoutSeconds - purely the
	// display default shown for a side that isn't currently ticking (see AShogiBoardManager,
	// both are hardcoded 15.f there too rather than shared via one constant).
	constexpr int32 DefaultFreeSeconds = 15;

	const bool bIsActingSide = !GS->bGameOver && GS->CurrentTurn == Side;
	const bool bDrawingBank = bIsActingSide && GS->bIsDrawingFromTimeBank;

	const int32 BankSeconds = FMath::Max(0, FMath::CeilToInt(bDrawingBank ? GS->GetPhaseTimerSecondsRemaining() : GS->GetTimeBankSeconds(Side)));
	const int32 FreeSeconds = bDrawingBank ? 0 : (bIsActingSide ? FMath::Max(0, FMath::CeilToInt(GS->GetPhaseTimerSecondsRemaining())) : DefaultFreeSeconds);

	return FText::FromString(FString::Printf(TEXT("%d+%d"), BankSeconds, FreeSeconds));
}

ESlateVisibility UShogiTurnWidgetBase::GetTimeBankVisibility(EPlayerSide Side) const
{
	const AShogiGameState* GS = GetWorld() ? GetWorld()->GetGameState<AShogiGameState>() : nullptr;
	if (!GS || GS->bGameOver)
	{
		return ESlateVisibility::Hidden;
	}

	return (GS->CurrentTurn == Side) ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
}
