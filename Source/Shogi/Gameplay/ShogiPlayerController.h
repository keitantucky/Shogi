// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShogiTypes.h"
#include "ShogiCardTypes.h"
#include "ShogiPlayerController.generated.h"

class AShogiPiece;
class AShogiBoardManager;
class AShogiGameState;
class UUserWidget;
class UShogiPromotionPromptWidgetBase;
class UShogiCardHandWidgetBase;
class UShogiCardEffectBannerWidgetBase;

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
	 * ignoring PlayerSide. Set by AShogiGameMode::PostLogin when running NM_Standalone
	 * (Main opened directly), and unconditionally by AShogiSinglePlayerHotSeatGameMode.
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

	/** Assign to a new WBP_CardHand (parented to UShogiCardHandWidgetBase) in the editor. See docs/2026-08-14-card-system-phase-a.md. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	TSubclassOf<UShogiCardHandWidgetBase> CardHandWidgetClass;

	/** Assign to a new WBP_CardEffectBanner (parented to UShogiCardEffectBannerWidgetBase) in the editor. */
	UPROPERTY(EditAnywhere, Category = "Shogi")
	TSubclassOf<UShogiCardEffectBannerWidgetBase> CardEffectBannerWidgetClass;

	UFUNCTION(Server, Reliable)
	void Server_RequestMovePiece(int32 From, int32 To, bool bPromote);

	UFUNCTION(Server, Reliable)
	void Server_RequestDropPiece(int32 DropIndex, AShogiPiece* DropPieceActor, EPieceType DropPieceType);

	/**
	 * Plays CardType from this controller's (GetControllableSide()'s) hand. TargetBoardIndex
	 * is used by FuRocket/InstantAwakening, TargetHandPieceActor by Hyena; both are ignored
	 * by cards that don't need them (e.g. TenpenChii). See docs/2026-08-14-card-system-phase-a.md.
	 */
	UFUNCTION(Server, Reliable)
	void Server_RequestPlayCard(ECardType CardType, int32 TargetBoardIndex, AShogiPiece* TargetHandPieceActor);

	/** Declines to play a card this turn, resolving the card phase so a move/drop becomes legal. */
	UFUNCTION(Server, Reliable)
	void Server_PassCardPhase();

	/** This controller's hand for whichever side it currently controls (GetControllableSide()). UI bind target. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	const TArray<ECardType>& GetMyHand() const;

	/** True if CardType is currently playable (in hand AND has a valid target) for GetControllableSide(). */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	bool IsCardPlayable(ECardType CardType) const;

	/**
	 * Begins the card-target-selection flow for CardType (must be in GetMyHand() and pass
	 * IsCardPlayable). Cards needing no target (TenpenChii) are sent to the server immediately;
	 * others wait for the next board/hand-piece click to supply a target (see HandleLeftClick).
	 */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	void SelectCardToPlay(ECardType CardType);

	/**
	 * Non-empty (「カードフェーズ未解決です」) whenever this controller just tried to move/drop a
	 * piece on its own unresolved-card-phase turn (see TryMoveOrDropTo) and hasn't resolved the
	 * card phase since. Automatically goes back to empty once the card phase resolves (playing a
	 * card or passing) or a new turn starts - no explicit "clear" call needed. UI bind target for
	 * a warning message (see docs/2026-08-14-card-system-phase-a.md).
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	FText GetCardPhaseWarningText() const;

	/**
	 * Discards Side's current hand and deals a fresh 3 cards, drawn uniformly at random from
	 * all 10 implemented card types (the deck is effectively infinite - see
	 * docs/2026-08-14-card-system-phase-b.md 2.2, revised). Called by
	 * AShogiBoardManager::AdvanceTurn at the start of Side's turn - a plain function (not an
	 * RPC) since it's only ever invoked server-side, matching AShogiBoardManager::ApplyMove/
	 * ApplyDrop's convention.
	 */
	void RedrawHandForSide(EPlayerSide Side);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION()
	void OnRep_PlayerSide();

	UFUNCTION()
	void HandleTurnChanged();

	UFUNCTION()
	void HandleCardEffectActivated(ECardType CardType, EPlayerSide Side, int32 TargetBoardIndex);

	void HandleLeftClick();

	UFUNCTION()
	void OnRep_CardState_Sente();

	UFUNCTION()
	void OnRep_CardState_Gote();

private:
	// mutable: GetOrFindBoardManager/GetOrFindGameState lazily populate these on first
	// successful lookup and are called from const contexts too (e.g. IsCardPlayable) -
	// without retrying via the lazy getter, a lookup that raced ahead of AShogiBoardManager's
	// replication to a remote client (BeginPlay's initial GetOrFindBoardManager() call) would
	// leave BoardManager permanently null for that client, silently breaking every subsequent
	// IsCardPlayable check (and therefore every card button) for the rest of the match.
	UPROPERTY()
	mutable TObjectPtr<AShogiBoardManager> BoardManager;

	UPROPERTY()
	TObjectPtr<UShogiCardEffectBannerWidgetBase> CardEffectBannerWidget;

	UPROPERTY()
	mutable TObjectPtr<AShogiGameState> GameState;

	int32 SelectedPieceIndex = INDEX_NONE;
	EPieceType SelectedPieceType = EPieceType::None;
	int32 ClickedIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<AShogiPiece> SelectedHandPieceActor;
	EPieceType SelectedHandPieceType = EPieceType::None;

	// Card currently "armed" by SelectCardToPlay, awaiting a target click (or already sent,
	// for cards that need none). Cleared once Server_RequestPlayCard is sent.
	ECardType SelectedCardType = ECardType::None;

	// Set by TryMoveOrDropTo when it refuses to send a move/drop RPC because the card phase
	// isn't resolved yet. See GetCardPhaseWarningText. Reset in HandleTurnChanged so a stale
	// warning from a previous turn never bleeds into the next one.
	bool bBlockedByCardPhase = false;

	TArray<int32> CurrentMovableList;

	// Hands (Hand only - see FShogiCardHandState), replicated to this controller's owning
	// client only. Both members exist on every controller instance so a single hot-seat
	// controller (bControlBothSides) can hold both sides' hands; a normal online controller
	// only ever populates the member matching its own PlayerSide.
	UPROPERTY(ReplicatedUsing = OnRep_CardState_Sente)
	FShogiCardHandState CardState_Sente;

	UPROPERTY(ReplicatedUsing = OnRep_CardState_Gote)
	FShogiCardHandState CardState_Gote;

	FShogiCardHandState& GetCardState(EPlayerSide Side);
	const FShogiCardHandState& GetCardState(EPlayerSide Side) const;
	void InitializeInitialHands();

	// Pending move awaiting an optional-promotion decision from the player.
	int32 PendingMoveFrom = INDEX_NONE;
	int32 PendingMoveTo = INDEX_NONE;

	AShogiBoardManager* GetOrFindBoardManager() const;
	AShogiGameState* GetOrFindGameState() const;
	bool TryComputeClickedBoardIndex(int32& OutIndex) const;
	void ClearSelection();
	void TrySelectAt(int32 Index);
	void TryMoveOrDropTo(int32 Index);
	void ConfirmMove(int32 From, int32 To, bool bPromote);
	void SwitchCameraForSide(EPlayerSide Side);

	UFUNCTION()
	void OnPromotionChoiceMade(bool bPromote);
};
