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

		// Standalone means Main was opened directly (no EOSCore session) - there is no
		// second client that could ever log in as Gote, so default to local hot-seat.
		if (GetNetMode() == NM_Standalone)
		{
			ShogiPC->bControlBothSides = true;
		}
	}
	else if (JoinedPlayerCount == 1)
	{
		GotePlayer = ShogiPC;
		ShogiPC->PlayerSide = EPlayerSide::Gote;
		++JoinedPlayerCount;
	}
	// 3rd+ connecting players are left at the default PlayerSide::None (spectator).
}

AShogiPlayerController* AShogiGameMode::GetControllerForSide(EPlayerSide Side) const
{
	if (SentePlayer && (SentePlayer->bControlBothSides || SentePlayer->PlayerSide == Side))
	{
		return SentePlayer;
	}
	if (GotePlayer && GotePlayer->PlayerSide == Side)
	{
		return GotePlayer;
	}
	return nullptr;
}
