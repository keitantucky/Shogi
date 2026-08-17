// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShogiPieceStatusWidgetBase.generated.h"

class AShogiPiece;

/**
 * Native logic base class for a new WBP_PieceStatusIcon widget (does not exist yet - must be
 * created in the editor with a parent class of this type; see AShogiPiece::StatusWidgetComponent
 * for how it's attached/loaded). Exposes one BlueprintPure bool getter per status flag on
 * FShogiPieceData, plus a matching ESlateVisibility wrapper for each (UMG's "Bind" only accepts
 * functions whose return type exactly matches the target property, so Visibility needs its own
 * ESlateVisibility-returning getter rather than the bool one directly) - bind each icon Image's
 * Visibility to the matching Get*Visibility() function via the editor's "Bind" button. This
 * matches the existing polling-based convention (UShogiTurnWidgetBase, UShogiCardHandWidgetBase)
 * rather than pushing refresh events - PieceData already replicates and AShogiPiece::UpdateAppearance
 * runs on every mutation, so per-frame polling here stays cheap and always current.
 *
 * RentalReservation's "pending" phase is intentionally NOT exposed here (no GetHasPendingLoan) -
 * see AShogiBoardManager::ApplyCardEffect's RentalReservation case, which deliberately keeps the
 * reservation invisible until it activates. Only the post-activation loan (GetIsLoanActive) is shown.
 */
UCLASS()
class SHOGI_API UShogiPieceStatusWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called once by AShogiPiece right after spawning this widget's UserWidget instance. */
	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void SetOwningPiece(AShogiPiece* InOwningPiece);

	UFUNCTION(BlueprintPure, Category = "Shogi")
	bool GetIsPromoted() const;

	UFUNCTION(BlueprintPure, Category = "Shogi")
	bool GetHasFuRocketBoost() const;

	UFUNCTION(BlueprintPure, Category = "Shogi")
	bool GetIsPetrified() const;

	UFUNCTION(BlueprintPure, Category = "Shogi")
	bool GetIsInvincible() const;

	UFUNCTION(BlueprintPure, Category = "Shogi")
	bool GetIsLoanActive() const;

	/** Visible if GetIsPromoted(), Collapsed otherwise. Bind an icon Image's Visibility to this. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	ESlateVisibility GetPromotedVisibility() const;

	/** Visible if GetHasFuRocketBoost(), Collapsed otherwise. Bind an icon Image's Visibility to this. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	ESlateVisibility GetFuRocketBoostVisibility() const;

	/** Visible if GetIsPetrified(), Collapsed otherwise. Bind an icon Image's Visibility to this. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	ESlateVisibility GetPetrifiedVisibility() const;

	/** Visible if GetIsInvincible(), Collapsed otherwise. Bind an icon Image's Visibility to this. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	ESlateVisibility GetInvincibleVisibility() const;

	/** Visible if GetIsLoanActive(), Collapsed otherwise. Bind an icon Image's Visibility to this. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	ESlateVisibility GetLoanActiveVisibility() const;

private:
	UPROPERTY()
	TObjectPtr<AShogiPiece> OwningPiece;
};
