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

float AShogiGameState::GetPhaseTimerSecondsRemaining() const
{
	if (CurrentPhaseTimerEndTime < 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, CurrentPhaseTimerEndTime - GetServerWorldTimeSeconds());
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
}
