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

void AShogiGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShogiGameState, CurrentTurn);
	DOREPLIFETIME(AShogiGameState, bGameOver);
	DOREPLIFETIME(AShogiGameState, Winner);
}
