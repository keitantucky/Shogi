// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiGameState.h"
#include "Net/UnrealNetwork.h"

void AShogiGameState::OnRep_CurrentTurn()
{
	OnTurnChanged.Broadcast();
}

void AShogiGameState::OnRep_GameOver()
{
	OnGameOver.Broadcast();
}

void AShogiGameState::Multicast_NotifyCardEffectActivated_Implementation(ECardType CardType, EPlayerSide Side, int32 TargetBoardIndex)
{
	OnCardEffectActivated.Broadcast(CardType, Side, TargetBoardIndex);
}

float AShogiGameState::GetPhaseTimerSecondsRemaining() const
{
	if (CurrentPhaseTimerEndTime < 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, CurrentPhaseTimerEndTime - GetServerWorldTimeSeconds());
}

float AShogiGameState::GetTimeBankSeconds(EPlayerSide Side) const
{
	switch (Side)
	{
		case EPlayerSide::Sente:	return TimeBankSeconds_Sente;
		case EPlayerSide::Gote:	return TimeBankSeconds_Gote;
		default:					return 0.f;
	}
}

void AShogiGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShogiGameState, CurrentTurn);
	DOREPLIFETIME(AShogiGameState, bInCheck);
	DOREPLIFETIME(AShogiGameState, bGameOver);
	DOREPLIFETIME(AShogiGameState, Winner);
	DOREPLIFETIME(AShogiGameState, bCardPhaseResolved);
	DOREPLIFETIME(AShogiGameState, CurrentPhaseTimerEndTime);
	DOREPLIFETIME(AShogiGameState, TimeBankSeconds_Sente);
	DOREPLIFETIME(AShogiGameState, TimeBankSeconds_Gote);
	DOREPLIFETIME(AShogiGameState, bIsDrawingFromTimeBank);
}
