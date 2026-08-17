// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShogiTypes.h"
#include "ShogiCardTypes.h"
#include "ShogiBoardManager.generated.h"

class AShogiPiece;
class UDataTable;
class USceneComponent;

/**
 * One candidate move or drop, as enumerated by AShogiBoardManager::PickRandomLegalAction.
 * Used by AShogiSinglePlayerVsAIGameMode::MakeAIMove to pick the CPU opponent's move from
 * the exact same "every legal move/drop for Side" pool a human player's move markers show.
 */
USTRUCT()
struct FShogiLegalAction
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsDrop = false;

	UPROPERTY()
	int32 From = INDEX_NONE;

	UPROPERTY()
	int32 To = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<AShogiPiece> DropActor = nullptr;

	UPROPERTY()
	EPieceType DropPieceType = EPieceType::None;
};

/**
 * Native replacement for the Blueprint Actor BP_BoardManager. Owns the authoritative
 * board state and spawned piece actors, and is placed once in Content/Map/Main.umap.
 *
 * Networking note: unlike the original Blueprint (which defined Server_RequestMovePiece/
 * Server_RequestDropPiece as Server RPCs directly on BP_BoardManager), this actor is not
 * owned by either connecting client, and Unreal only routes a Server RPC correctly when
 * the calling client owns the target actor. The RPC entry points therefore live on
 * AShogiPlayerController (always owned by its own client) and call the plain
 * ApplyMove/ApplyDrop functions here once already running on the server - see
 * docs/GameSpec.md for details on this deviation from the original architecture.
 *
 * Board convention: flat 9x9 array, BoardIndex = Y*9+X (see UShogiRulesLibrary).
 * World placement: GetWorldLocationForIndex offsets from this actor's own location by
 * BoardCellSizeX/BoardCellSizeY per cell, centered on the middle square (4,4) - tune
 * these and/or this actor's placement to match the existing board mesh.
 */
UCLASS()
class SHOGI_API AShogiBoardManager : public AActor
{
	GENERATED_BODY()

public:
	AShogiBoardManager();

	UPROPERTY(ReplicatedUsing = OnRep_BoardArray, BlueprintReadOnly, Category = "Shogi")
	TArray<FShogiPieceData> BoardArray;

	UPROPERTY(ReplicatedUsing = OnRep_PieceActorArray, BlueprintReadOnly, Category = "Shogi")
	TArray<TObjectPtr<AShogiPiece>> PieceActorArray;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shogi")
	TArray<FShogiPieceData> CapturedPieces_Sente;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shogi")
	TArray<FShogiPieceData> CapturedPieces_Gote;

	UPROPERTY(ReplicatedUsing = OnRep_HandPieceActors_Sente, BlueprintReadOnly, Category = "Shogi")
	TArray<TObjectPtr<AShogiPiece>> HandPieceActors_Sente;

	UPROPERTY(ReplicatedUsing = OnRep_HandPieceActors_Gote, BlueprintReadOnly, Category = "Shogi")
	TArray<TObjectPtr<AShogiPiece>> HandPieceActors_Gote;

	UPROPERTY(VisibleAnywhere, Category = "Shogi")
	TObjectPtr<USceneComponent> SenteKomadai;

	UPROPERTY(VisibleAnywhere, Category = "Shogi")
	TObjectPtr<USceneComponent> GoteKomadai;

	/** Actor class spawned for each board/hand piece. Defaults to AShogiPiece. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	TSubclassOf<AShogiPiece> PieceActorClass;

	/** Assign to the existing BP_MoveTileMarker (or any highlight actor) in the editor. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	TSubclassOf<AActor> MoveMarkerClass;

	/** RowStruct must be FShogiBoardSetupRow. Assign the migrated initial-board DataTable in the editor. */
	UPROPERTY(EditAnywhere, Category = "Shogi|Data")
	TObjectPtr<UDataTable> InitialBoardSetupTable;

	/** One movement DataTable (RowStruct FShogiMoveMatrixRow) per unpromoted piece type. */
	UPROPERTY(EditAnywhere, Category = "Shogi|Data")
	TMap<EPieceType, TObjectPtr<UDataTable>> UnpromotedMoveTables;

	/** 角行成り (promoted Bishop / Uma) movement table. */
	UPROPERTY(EditAnywhere, Category = "Shogi|Data")
	TObjectPtr<UDataTable> PromotedBishopMoveTable;

	/** 飛車成り (promoted Rook / Ryu) movement table. */
	UPROPERTY(EditAnywhere, Category = "Shogi|Data")
	TObjectPtr<UDataTable> PromotedRookMoveTable;

	/** 成金 (promoted Pawn/Lance/Knight/Silver - all move like Gold) movement table. */
	UPROPERTY(EditAnywhere, Category = "Shogi|Data")
	TObjectPtr<UDataTable> PromotedGenericMoveTable;

	/** World-space distance between adjacent board cells along the file (X) axis; tune to match the board mesh. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	float BoardCellSizeX = 100.f;

	/** World-space distance between adjacent board cells along the rank (Y) axis; tune to match the board mesh. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	float BoardCellSizeY = 100.f;

	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void SpawnPieces();

	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void RefreshBoardVisual();

	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void ShowMoveMarkers(const TArray<int32>& Indices);

	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void ClearMoveMarkers();

	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void UpdateStandLayout();

	/** Returns the correct movement DataTable for Piece (promoted or not). May return nullptr if unassigned. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	UDataTable* GetMoveDataTableFor(const FShogiPieceData& Piece) const;

	/**
	 * Editor debug helper: click in the Details panel (no PIE needed) to dump every
	 * non-empty cell of every assigned movement table to the Output Log, showing the
	 * raw (dY, dX) offset each cell represents. Use this to verify the DataTable's
	 * row/column axis convention matches UShogiRulesLibrary::CheckCanMove's assumption
	 * (CsvRow = dY+9, CsvCol = dX+9) - see docs/GameSpec.md.
	 * CallInEditor buttons only render for parameterless functions, hence no arguments -
	 * this dumps every piece type / promoted-state table in one go.
	 */
	UFUNCTION(CallInEditor, Category = "Shogi|Debug")
	void DebugLogAllMoveTables();

	UFUNCTION(BlueprintPure, Category = "Shogi")
	FVector GetWorldLocationForIndex(int32 BoardIndex) const;

	/**
	 * Server-authoritative move application. Must only be called on the server (from
	 * AShogiPlayerController::Server_RequestMovePiece_Implementation). Re-validates
	 * legality and promotion server-side rather than trusting the caller.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void ApplyMove(int32 From, int32 To, EPlayerSide RequestingSide, bool bClientRequestedPromote);

	/** Server-authoritative drop application. Must only be called on the server. */
	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void ApplyDrop(int32 DropIndex, AShogiPiece* DropPieceActor, EPieceType DropPieceType, EPlayerSide RequestingSide);

	/**
	 * Server-authoritative card-effect application (see docs/2026-08-14-card-system-phase-a.md).
	 * Must only be called on the server, from AShogiPlayerController::Server_RequestPlayCard_
	 * Implementation. Re-validates turn/card-phase/target legality itself rather than trusting
	 * the caller. TargetBoardIndex is used by FuRocket/InstantAwakening, TargetHandPieceActor
	 * by Hyena (must be one of the opponent's captured-piece actors); both are ignored by
	 * cards that need neither (TenpenChii). Returns true only if the effect was actually
	 * applied - the caller (PlayerController) removes the card from hand and resolves the
	 * card phase only on a true return.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shogi")
	bool ApplyCardEffect(ECardType CardType, EPlayerSide RequestingSide, int32 TargetBoardIndex, AShogiPiece* TargetHandPieceActor);

	/**
	 * Uniformly-random pick among every currently legal move/drop for Side (board moves via
	 * GetMovableIndices, hand-piece drops onto any empty, non-nifu-violating square) - the pool
	 * AShogiSinglePlayerVsAIGameMode::MakeAIMove chooses the CPU opponent's move from. Returns
	 * false (OutAction left default) if Side has no legal move or drop at all.
	 */
	bool PickRandomLegalAction(EPlayerSide Side, FShogiLegalAction& OutAction) const;

	/**
	 * Called by AShogiPlayerController whenever Side's card phase becomes resolved (a card was
	 * played or a pass was requested) - see docs/2026-08-14-card-system-phase-b.md 3.7. Deducts
	 * any time drawn from Side's persistent bank (EndBankDepletion), clears the card-phase free-
	 * allowance timer, and, only if a controller actually plays Side
	 * (AShogiGameMode::GetControllerForSide), starts the move phase's 15-second free allowance.
	 * Safe to call even when nothing was running.
	 */
	void OnCardPhaseResolved(EPlayerSide Side);

	/**
	 * Resets and (re)starts CurrentTurn's card phase: redraws that side's hand and starts the
	 * 15-second card-phase timeout, or clears any countdown if nobody controls that side (the
	 * AI's side in AShogiSinglePlayerVsAIGameMode). Called by AdvanceTurn after every
	 * subsequent turn change, and once by AShogiGameMode::TryStartInitialCardPhase to cover the
	 * very first turn (Sente's, at game start), which AdvanceTurn itself never runs for since
	 * it only fires after a move completes. See docs/2026-08-14-card-system-phase-b.md 3.7.
	 */
	void StartCardPhaseForCurrentTurn();

	/**
	 * True if any of Side's opponent's pieces could currently move onto Side's King
	 * square (i.e. Side is in check). Display-only (see docs/GameSpec.md - this does not
	 * restrict move legality, so moves that ignore or walk into check are still allowed).
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	bool IsKingInCheck(EPlayerSide Side) const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_BoardArray();

	UFUNCTION()
	void OnRep_PieceActorArray();

	UFUNCTION()
	void OnRep_HandPieceActors_Sente();

	UFUNCTION()
	void OnRep_HandPieceActors_Gote();

private:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActiveMoveMarkers;

	AShogiPiece* SpawnPieceActor(const FShogiPieceData& PieceData, int32 BoardIndex);
	void AdvanceTurn();

	/**
	 * Called once per AdvanceTurn, right after CurrentTurn flips to NewCurrentTurn: clears
	 * expired Temporary Invincibility and ticks/activates/reverts Rental Reservation for every
	 * board piece. See docs/2026-08-14-card-system-phase-b.md 3.6.
	 */
	void TickTimedCardEffects(EPlayerSide NewCurrentTurn);

	/**
	 * True if Side's card phase is (now) resolved and a move/drop may proceed. If
	 * GS->bCardPhaseResolved is already true, returns true immediately. Otherwise, looks up
	 * whether any controller currently plays Side (AShogiGameMode::GetControllerForSide) - if
	 * not (the AI's side in AShogiSinglePlayerVsAIGameMode, or the very first turn evaluated
	 * before any controller has logged in), latches bCardPhaseResolved=true and returns true;
	 * if a controller does exist, returns false (that controller must call
	 * Server_RequestPlayCard/Server_PassCardPhase first). See docs/2026-08-14-card-system-phase-a.md.
	 */
	bool ResolveCardPhaseIfUncontrolled(class AShogiGameState* GS, EPlayerSide Side);
	class AShogiGameState* GetShogiGameState() const;
	static void DebugLogTableContents(UDataTable* Table, const FString& Label);

	/**
	 * Free per-phase allowance before a side's persistent time bank
	 * (AShogiGameState::TimeBankSeconds_Sente/_Gote) starts being drawn from - see
	 * BeginBankDepletion. Only ever started when a controller plays the relevant side.
	 */
	static constexpr float CardPhaseTimeoutSeconds = 15.f;
	static constexpr float MovePhaseTimeoutSeconds = 15.f;

	FTimerHandle CardPhaseTimerHandle;
	FTimerHandle MovePhaseTimerHandle;

	/**
	 * GetServerWorldTimeSeconds() at which CurrentTurn's side started being drawn from its
	 * persistent time bank (BeginBankDepletion); -1 while still within a phase's free
	 * CardPhaseTimeoutSeconds/MovePhaseTimeoutSeconds allowance, or when no phase is active.
	 */
	float BankDepletionStartTime = -1.f;

	/** Fires when CardPhaseTimerHandle's free allowance expires without the card phase resolving. */
	void HandleCardPhaseFreePeriodExpired();

	/** Fires when MovePhaseTimerHandle's free allowance expires without a move/drop being made. */
	void HandleMovePhaseFreePeriodExpired();

	/**
	 * Starts drawing CurrentTurn's persistent time bank: re-arms PhaseTimerHandle for however
	 * much bank remains, pointed at HandleTimeUp, and updates CurrentPhaseTimerEndTime so the
	 * existing phase-timer UI switches from the free-allowance countdown to the bank countdown.
	 * If the bank is already empty, ends the game immediately instead (see HandleTimeUp).
	 */
	void BeginBankDepletion(FTimerHandle& PhaseTimerHandle);

	/**
	 * Called whenever a phase resolves normally (a card is played/passed, or a move/drop is
	 * applied) for Side. If BeginBankDepletion had started drawing the bank, deducts the elapsed
	 * time from Side's persistent bank and stops drawing. Safe to call even when the bank wasn't
	 * being drawn (no-op).
	 */
	void EndBankDepletion(EPlayerSide Side);

	/** Fires when CurrentTurn's persistent time bank reaches zero mid-phase: a time-up loss for that side. */
	void HandleTimeUp();

	/** Stamps AShogiGameState::CurrentPhaseTimerEndTime so clients can render a synced countdown (see docs/2026-08-14-card-system-phase-b.md 3.7, revised). */
	void SetPhaseTimerEndTime(float DurationSeconds);

	/** Sets AShogiGameState::CurrentPhaseTimerEndTime back to -1 (no countdown to display). */
	void ClearPhaseTimerEndTime();
};
