// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "ShogiCardTypes.h"
#include "ShogiCardHandWidgetBase.generated.h"

class AShogiPlayerController;

/**
 * Native logic base class for a new WBP_CardHand widget (does not exist yet - must be
 * created in the editor with a parent class of this type, see
 * docs/2026-08-14-card-system-phase-a.md). Backend-first/minimal-UI by design: no card
 * art exists yet, so this exposes plain text/bool functions meant to drive simple
 * text-button UMG bindings rather than a full drag-and-drop card layout.
 */
UCLASS()
class SHOGI_API UShogiCardHandWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Display names (docs/GameConcept.md card names) for each card currently in hand, up to 3. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	TArray<FText> GetMyHandDisplayNames() const;

	/** Display name for the card at HandSlot (0-based), or empty text if that slot isn't filled. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	FText GetHandSlotDisplayName(int32 HandSlot) const;

	/**
	 * Parameterless convenience wrappers around GetHandSlotDisplayName, one per hand slot
	 * (hand max is 3 - see docs/2026-08-14-card-system-phase-a.md) - bind a TextBlock's Text
	 * property directly to one of these via the editor's "Bind" button, since UMG property
	 * bindings can only call parameterless functions.
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	FText GetHandSlot0DisplayName() const { return GetHandSlotDisplayName(0); }

	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	FText GetHandSlot1DisplayName() const { return GetHandSlotDisplayName(1); }

	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	FText GetHandSlot2DisplayName() const { return GetHandSlotDisplayName(2); }

	/** True if the hand slot at HandSlot (0-based index into GetMyHandDisplayNames()) can be played right now. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	bool IsCardPlayableAt(int32 HandSlot) const;

	/** Bind to each hand-slot button's OnClicked. Arms card-target-selection (see AShogiPlayerController::SelectCardToPlay). */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	void OnCardButtonClicked(int32 HandSlot);

	/** Bind to the pass button's OnClicked. Resolves the card phase without playing a card. */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	void OnPassButtonClicked();

	/** Forwards AShogiPlayerController::GetCardPhaseWarningText - bind a TextBlock's Text to this. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	FText GetCardPhaseWarningText() const;

	/** Visible while GetCardPhaseWarningText() is non-empty, Hidden otherwise - bind a widget's Visibility to this. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	ESlateVisibility GetCardPhaseWarningVisibility() const;

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TObjectPtr<AShogiPlayerController> OwningShogiController;

	AShogiPlayerController* GetShogiController() const;
};
