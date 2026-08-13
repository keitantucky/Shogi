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

	/** True if CurrentTurn's King is currently attacked (王手). Display-only, see AShogiBoardManager::IsKingInCheck. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shogi")
	bool bInCheck = false;

	/**
	 * True once CurrentTurn's side has resolved its card phase this turn (played a card or
	 * passed) - see docs/2026-08-14-card-system-phase-a.md. AShogiBoardManager::ApplyMove/
	 * ApplyDrop reject requests while this is false, enforcing "play a card (or pass) before
	 * moving". Reset to false by AShogiBoardManager::AdvanceTurn whenever CurrentTurn changes.
	 * Shared/public information (not hand contents), so no COND_OwnerOnly is needed.
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shogi")
	bool bCardPhaseResolved = false;

	/**
	 * Server-clock (GetServerWorldTimeSeconds) timestamp at which the current card-phase or
	 * move-phase 15-second timeout will fire (see docs/2026-08-14-card-system-phase-b.md 3.7).
	 * -1 means no timer is currently running (e.g. it's the CPU-controlled AI's turn). Set by
	 * AShogiBoardManager whenever it starts/stops CardPhaseTimerHandle/MovePhaseTimerHandle.
	 * GetServerWorldTimeSeconds (not GetWorld()->GetTimeSeconds) is used on both ends because
	 * it's kept in sync between server and clients specifically for this kind of countdown.
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shogi")
	float CurrentPhaseTimerEndTime = -1.f;

	/** Seconds left before the current phase's timeout fires, or 0 if no timer is running. UI bind target. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	float GetPhaseTimerSecondsRemaining() const;

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
