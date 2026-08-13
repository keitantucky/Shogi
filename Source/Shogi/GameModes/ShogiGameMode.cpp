// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiGameMode.h"
#include "ShogiGameState.h"
#include "ShogiPlayerController.h"

AShogiGameMode::AShogiGameMode()
{
	GameStateClass = AShogiGameState::StaticClass();
	PlayerControllerClass = AShogiPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void AShogiGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AShogiPlayerController* ShogiPC = Cast<AShogiPlayerController>(NewPlayer);
	if (!ShogiPC)
	{
		return;
	}

	if (JoinedPlayerCount == 0)
	{
		SentePlayer = ShogiPC;
		ShogiPC->PlayerSide = EPlayerSide::Sente;
		++JoinedPlayerCount;
	}
	else if (JoinedPlayerCount == 1)
	{
		GotePlayer = ShogiPC;
		ShogiPC->PlayerSide = EPlayerSide::Gote;
		++JoinedPlayerCount;
	}
	// 3rd+ connecting players are left at the default PlayerSide::None (spectator).
}
