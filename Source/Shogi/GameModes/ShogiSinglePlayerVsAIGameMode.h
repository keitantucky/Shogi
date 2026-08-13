// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShogiGameMode.h"
#include "ShogiTypes.h"
#include "ShogiSinglePlayerVsAIGameMode.generated.h"

class AShogiBoardManager;
class AShogiGameState;

/**
 * Single-player-vs-CPU GameMode. Assigns the one local player to LocalHumanSide and
 * has the opposite side played by a simple AI that picks uniformly at random among
 * all currently legal moves (board moves and hand-piece drops), reusing
 * UShogiRulesLibrary::GetMovableIndices - no search/evaluation, see docs/GameSpec.md.
 *
 * Not wired into any menu - open Content/Map/Main.umap with this class as the GameMode
 * override (e.g. UGameplayStatics::OpenLevel with Options="game=ShogiSinglePlayerVsAIGameMode").
 */
UCLASS()
class SHOGI_API AShogiSinglePlayerVsAIGameMode : public AShogiGameMode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Shogi")
	EPlayerSide LocalHumanSide = EPlayerSide::Sente;

	/** Delay before the AI applies its chosen move, so it doesn't feel instantaneous. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	float AIThinkDelaySeconds = 0.5f;

	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	UFUNCTION()
	void HandleTurnChanged();

	void MakeAIMove();

private:
	UPROPERTY()
	TObjectPtr<AShogiBoardManager> BoardManager;

	UPROPERTY()
	TObjectPtr<AShogiGameState> CachedShogiGameState;

	FTimerHandle AIMoveTimerHandle;

	EPlayerSide GetAISide() const;
};
