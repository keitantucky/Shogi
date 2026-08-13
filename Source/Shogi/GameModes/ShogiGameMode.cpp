// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiGameMode.h"
#include "ShogiGameState.h"
#include "ShogiPlayerController.h"
#include "ShogiBoardManager.h"
#include "Kismet/GameplayStatics.h"

AShogiGameMode::AShogiGameMode()
{
	GameStateClass = AShogiGameState::StaticClass();
	PlayerControllerClass = AShogiPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void AShogiGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	AssignPlayerSideOnLogin(NewPlayer);
	TryStartInitialCardPhase();
}

void AShogiGameMode::AssignPlayerSideOnLogin(APlayerController* NewPlayer)
{
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

void AShogiGameMode::TryStartInitialCardPhase()
{
	if (bInitialCardPhaseStarted)
	{
		return;
	}

	// Online multiplayer (NM_ListenServer/NM_Client) needs both Sente and Gote connected before
	// the match can begin - otherwise Sente's card-phase countdown starts (and can time out,
	// forcing a random card) while still waiting for Gote to even log in. Standalone
	// (AShogiSinglePlayerVsAIGameMode/AShogiSinglePlayerHotSeatGameMode) only ever has the one
	// local player, so it should start as soon as that single PostLogin call finishes.
	const bool bNeedsBothPlayers = (GetNetMode() != NM_Standalone);
	if (bNeedsBothPlayers && JoinedPlayerCount < 2)
	{
		return;
	}

	AShogiBoardManager* BM = Cast<AShogiBoardManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AShogiBoardManager::StaticClass()));
	if (!BM)
	{
		return;
	}

	bInitialCardPhaseStarted = true;
	BM->StartCardPhaseForCurrentTurn();
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
