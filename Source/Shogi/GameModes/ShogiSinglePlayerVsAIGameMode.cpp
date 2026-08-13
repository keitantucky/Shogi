// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiSinglePlayerVsAIGameMode.h"
#include "ShogiBoardManager.h"
#include "ShogiGameState.h"
#include "ShogiPiece.h"
#include "ShogiPlayerController.h"
#include "ShogiRulesLibrary.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

EPlayerSide AShogiSinglePlayerVsAIGameMode::GetAISide() const
{
	return (LocalHumanSide == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;
}

void AShogiSinglePlayerVsAIGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Deliberately skips AShogiGameMode::PostLogin (calls the grandparent directly) so its
	// embedded TryStartInitialCardPhase() call doesn't fire before this override's own
	// PlayerSide/bControlBothSides correction below - see AShogiGameMode::TryStartInitialCardPhase.
	// Getting this wrong would let TryStartInitialCardPhase see the base class's transient
	// NM_Standalone auto-hotseat bControlBothSides=true and hand the human Sente's card phase
	// even when LocalHumanSide is Gote.
	AGameModeBase::PostLogin(NewPlayer);
	AssignPlayerSideOnLogin(NewPlayer);

	if (AShogiPlayerController* ShogiPC = Cast<AShogiPlayerController>(NewPlayer))
	{
		ShogiPC->PlayerSide = LocalHumanSide;
		ShogiPC->bControlBothSides = false;
	}

	BoardManager = Cast<AShogiBoardManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AShogiBoardManager::StaticClass()));
	CachedShogiGameState = GetWorld() ? GetWorld()->GetGameState<AShogiGameState>() : nullptr;

	if (CachedShogiGameState)
	{
		CachedShogiGameState->OnTurnChanged.AddDynamic(this, &AShogiSinglePlayerVsAIGameMode::HandleTurnChanged);
	}

	TryStartInitialCardPhase();

	// Covers the (currently unlikely, since Sente always moves first) case where it's
	// already the AI's turn by the time we start watching.
	HandleTurnChanged();
}

void AShogiSinglePlayerVsAIGameMode::HandleTurnChanged()
{
	if (!CachedShogiGameState || CachedShogiGameState->bGameOver)
	{
		return;
	}

	if (CachedShogiGameState->CurrentTurn == GetAISide())
	{
		GetWorldTimerManager().SetTimer(
			AIMoveTimerHandle, this, &AShogiSinglePlayerVsAIGameMode::MakeAIMove, AIThinkDelaySeconds, false);
	}
}

void AShogiSinglePlayerVsAIGameMode::MakeAIMove()
{
	if (!BoardManager || !CachedShogiGameState || CachedShogiGameState->bGameOver)
	{
		return;
	}

	const EPlayerSide AISide = GetAISide();
	if (CachedShogiGameState->CurrentTurn != AISide)
	{
		return;
	}

	// Shared with the 15-second move-phase timeout - see
	// docs/2026-08-14-card-system-phase-b.md 3.8 and AShogiBoardManager::PickRandomLegalAction.
	FShogiLegalAction Chosen;
	if (!BoardManager->PickRandomLegalAction(AISide, Chosen))
	{
		// No legal move or drop available - a stalemate-equivalent state. Full
		// checkmate/stalemate handling is out of scope (see docs/GameSpec.md), so the
		// AI simply passes rather than ending the game.
		return;
	}

	if (Chosen.bIsDrop)
	{
		BoardManager->ApplyDrop(Chosen.To, Chosen.DropActor, Chosen.DropPieceType, AISide);
	}
	else
	{
		// Always promotes when an optional promotion is available - see docs/GameSpec.md.
		BoardManager->ApplyMove(Chosen.From, Chosen.To, AISide, /*bClientRequestedPromote=*/true);
	}
}
