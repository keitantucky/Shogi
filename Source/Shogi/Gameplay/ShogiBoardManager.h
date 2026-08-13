// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShogiTypes.h"
#include "ShogiBoardManager.generated.h"

class AShogiPiece;
class UDataTable;
class USceneComponent;

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
	class AShogiGameState* GetShogiGameState() const;
	static void DebugLogTableContents(UDataTable* Table, const FString& Label);
};
