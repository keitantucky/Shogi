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

namespace
{
	struct FAIMoveOption
	{
		bool bIsDrop = false;
		int32 From = INDEX_NONE;
		int32 To = INDEX_NONE;
		AShogiPiece* DropActor = nullptr;
		EPieceType DropPieceType = EPieceType::None;
	};
}

EPlayerSide AShogiSinglePlayerVsAIGameMode::GetAISide() const
{
	return (LocalHumanSide == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;
}

void AShogiSinglePlayerVsAIGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

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

	TArray<FAIMoveOption> Options;

	for (int32 Index = 0; Index < BoardManager->BoardArray.Num(); ++Index)
	{
		const FShogiPieceData& Piece = BoardManager->BoardArray[Index];
		if (Piece.PlayerSide != AISide)
		{
			continue;
		}

		UDataTable* MoveTable = BoardManager->GetMoveDataTableFor(Piece);
		const TArray<int32> Movable = UShogiRulesLibrary::GetMovableIndices(BoardManager->BoardArray, Piece, Index, MoveTable);
		for (int32 To : Movable)
		{
			FAIMoveOption Option;
			Option.From = Index;
			Option.To = To;
			Options.Add(Option);
		}
	}

	const TArray<TObjectPtr<AShogiPiece>>& HandActors =
		(AISide == EPlayerSide::Sente) ? BoardManager->HandPieceActors_Sente : BoardManager->HandPieceActors_Gote;

	for (AShogiPiece* HandActor : HandActors)
	{
		if (!HandActor)
		{
			continue;
		}
		for (int32 Index = 0; Index < BoardManager->BoardArray.Num(); ++Index)
		{
			if (BoardManager->BoardArray[Index].PlayerSide == EPlayerSide::None)
			{
				FAIMoveOption Option;
				Option.bIsDrop = true;
				Option.To = Index;
				Option.DropActor = HandActor;
				Option.DropPieceType = HandActor->PieceData.PieceType;
				Options.Add(Option);
			}
		}
	}

	if (Options.Num() == 0)
	{
		// No legal move or drop available - a stalemate-equivalent state. Full
		// checkmate/stalemate handling is out of scope (see docs/GameSpec.md), so the
		// AI simply passes rather than ending the game.
		return;
	}

	const FAIMoveOption& Chosen = Options[FMath::RandRange(0, Options.Num() - 1)];
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
