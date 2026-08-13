// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ShogiTypes.h"
#include "ShogiGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShogiTurnChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShogiGameOver);

/**
 * Native replacement for the Blueprint GameStateBase BP_ShogiGameState.
 * Adds an OnTurnChanged delegate so UI can subscribe instead of polling every
 * frame (the original WBP_TurnUI used a per-tick UMG text binding).
 */
UCLASS()
class SHOGI_API AShogiGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentTurn, BlueprintReadOnly, Category = "Shogi")
	EPlayerSide CurrentTurn = EPlayerSide::Sente;

	UPROPERTY(BlueprintAssignable, Category = "Shogi")
	FOnShogiTurnChanged OnTurnChanged;

	/** Simplified win condition: true once a King has been captured. See docs/GameSpec.md - full check/checkmate detection is out of scope. */
	UPROPERTY(ReplicatedUsing = OnRep_GameOver, BlueprintReadOnly, Category = "Shogi")
	bool bGameOver = false;

	/** Valid only once bGameOver is true. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shogi")
	EPlayerSide Winner = EPlayerSide::None;

	UPROPERTY(BlueprintAssignable, Category = "Shogi")
	FOnShogiGameOver OnGameOver;

	UFUNCTION()
	void OnRep_CurrentTurn();

	UFUNCTION()
	void OnRep_GameOver();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
