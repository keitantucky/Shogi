// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShogiCardTypes.h"
#include "ShogiTypes.h"
#include "ShogiCardEffectBannerWidgetBase.generated.h"

/**
 * Native logic base class for a new WBP_CardEffectBanner widget (does not exist yet - must
 * be created in the editor with a parent class of this type, and CardEffectBannerWidgetClass
 * assigned on BP_MyPlayerController). Created once and kept alive for the whole match (like
 * WBP_TurnUI) rather than per-activation, so it can own a slide-in/slide-out UMG Widget
 * Animation triggered from OnCardEffectActivatedEvent.
 */
UCLASS()
class SHOGI_API UShogiCardEffectBannerWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Records CardType/Side, fires OnCardEffectActivatedEvent (bind a UMG Widget Animation's
	 * "Play Animation" to this in the Blueprint event graph to drive the slide-in), and
	 * (re)starts the auto-dismiss timer. Call from AShogiPlayerController::HandleCardEffectActivated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	void ShowCardEffect(ECardType CardType, EPlayerSide Side);

	/** UShogiCardEffectLibrary::GetCardDisplayName for the card currently being shown. UI bind target. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	FText GetCardNameText() const;

	/** UShogiCardEffectLibrary::GetCardDescription for the card currently being shown. UI bind target. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	FText GetCardDescriptionText() const;

	/** Which side played the card currently being shown. UI bind target. */
	UFUNCTION(BlueprintPure, Category = "Shogi|Cards")
	EPlayerSide GetActivatingSide() const;

	/**
	 * Fired once per ShowCardEffect call. Bind a UMG Widget Animation ("Play Animation") to
	 * this in the Blueprint's event graph to drive the slide-in-from-right presentation.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Shogi|Cards")
	void OnCardEffectActivatedEvent();

	/**
	 * Fired by the AutoDismissSeconds timer when it's time to start dismissing the banner.
	 * The default (native) implementation just calls HideCardEffect() immediately - override
	 * this event in Blueprint to play a "SlideOut" Widget Animation instead, and call
	 * HideCardEffect() from that animation's "Finished" event once it completes.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Shogi|Cards")
	void OnCardEffectDismissEvent();
	virtual void OnCardEffectDismissEvent_Implementation();

	/**
	 * Actually hides the banner (collapses it). Call from the SlideOut Widget Animation's
	 * "Finished" event once you've overridden OnCardEffectDismissEvent to play it. Safe to
	 * call more than once.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shogi|Cards")
	void HideCardEffect();

	/** Seconds after ShowCardEffect before OnCardEffectDismissEvent fires automatically. */
	UPROPERTY(EditDefaultsOnly, Category = "Shogi|Cards")
	float AutoDismissSeconds = 3.0f;

private:
	ECardType CurrentCardType = ECardType::None;
	EPlayerSide CurrentSide = EPlayerSide::None;
	FTimerHandle AutoDismissTimerHandle;
};
