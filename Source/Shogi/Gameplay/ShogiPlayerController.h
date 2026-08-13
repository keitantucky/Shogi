// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShogiTypes.h"
#include "ShogiPlayerController.generated.h"

class AShogiPiece;
class AShogiBoardManager;
class AShogiGameState;
class UUserWidget;
class UShogiPromotionPromptWidgetBase;

/**
 * Native replacement for the Blueprint PlayerController BP_MyPlayerController.
 *
 * Scope note: the original Blueprint also created WBP_Login/WBP_Main (EOSCore
 * session/menu widgets) in BeginPlay. Per the agreed scope (core gameplay only goes
 * to C++, online/menu systems stay Blueprint), this class does NOT recreate that -
 * see docs/GameSpec.md known gaps. Only the turn-indicator widget is handled here.
 */
UCLASS()
class SHOGI_API AShogiPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AShogiPlayerController();

	UPROPERTY(ReplicatedUsing = OnRep_PlayerSide, BlueprintReadOnly, Category = "Shogi")
	EPlayerSide PlayerSide = EPlayerSide::None;

	/**
	 * Hot-seat / local pass-and-play mode: when true, this controller can select and
	 * move pieces for whichever side's turn it currently is (see GetControllableSide),
	 * ignoring PlayerSide. Set by AShogiSinglePlayerHotSeatGameMode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
	bool bControlBothSides = false;

	/** Assign to the existing BP_SenteCamera in the editor. Used when bControlBothSides switches back to Sente's turn. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	TSubclassOf<AActor> SenteCameraClass;

	/** Assign to the existing BP_GoteCamera (or any camera actor) in the editor. Sente keeps the default view target unless bControlBothSides. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	TSubclassOf<AActor> GoteCameraClass;

	/** bControlBothSides ? GameState->CurrentTurn : PlayerSide - the side this controller may currently act for. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	EPlayerSide GetControllableSide() const;

	/** True if GetControllableSide() matches GameState->CurrentTurn, i.e. it's actually this controller's turn to move. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	bool IsMyTurn() const;

	/** Assign to WBP_TurnUI (reparented to UShogiTurnWidgetBase) in the editor. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	TSubclassOf<UUserWidget> TurnWidgetClass;

	/** Assign to a new WBP_PromotionPrompt (parented to UShogiPromotionPromptWidgetBase) in the editor. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	TSubclassOf<UShogiPromotionPromptWidgetBase> PromotionPromptWidgetClass;

	UFUNCTION(Server, Reliable)
	void Server_RequestMovePiece(int32 From, int32 To, bool bPromote);

	UFUNCTION(Server, Reliable)
	void Server_RequestDropPiece(int32 DropIndex, AShogiPiece* DropPieceActor, EPieceType DropPieceType);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION()
	void OnRep_PlayerSide();

	UFUNCTION()
	void HandleTurnChanged();

	void HandleLeftClick();

private:
	UPROPERTY()
	TObjectPtr<AShogiBoardManager> BoardManager;

	UPROPERTY()
	TObjectPtr<AShogiGameState> GameState;

	int32 SelectedPieceIndex = INDEX_NONE;
	EPieceType SelectedPieceType = EPieceType::None;
	int32 ClickedIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<AShogiPiece> SelectedHandPieceActor;
	EPieceType SelectedHandPieceType = EPieceType::None;

	TArray<int32> CurrentMovableList;

	// Pending move awaiting an optional-promotion decision from the player.
	int32 PendingMoveFrom = INDEX_NONE;
	int32 PendingMoveTo = INDEX_NONE;

	AShogiBoardManager* GetOrFindBoardManager();
	AShogiGameState* GetOrFindGameState();
	bool TryComputeClickedBoardIndex(int32& OutIndex) const;
	void ClearSelection();
	void TrySelectAt(int32 Index);
	void TryMoveOrDropTo(int32 Index);
	void ConfirmMove(int32 From, int32 To, bool bPromote);
	void SwitchCameraForSide(EPlayerSide Side);

	UFUNCTION()
	void OnPromotionChoiceMade(bool bPromote);
};
