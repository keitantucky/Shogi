// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "ShogiTurnWidgetBase.generated.h"

/**
 * Native logic base class for WBP_TurnUI. Reparent the existing widget's Blueprint
 * parent class to this one, then rebind the Txt_TurnInfo TextBlock's "Text" property
 * to call GetCurrentTurnText (Bind in the Details panel) instead of the old Blueprint
 * function graph. See docs/GameSpec.md for the exact migration steps.
 */
UCLASS()
class SHOGI_API UShogiTurnWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Shogi")
	FText GetCurrentTurnText() const;

	/**
	 * Whole-seconds countdown ("15" down to "0") for whichever 15-second phase timer is
	 * currently running (card phase or move phase - see docs/2026-08-14-card-system-phase-b.md
	 * 3.7), or empty text if no timer is active (game over, or the CPU AI's turn). Bind a
	 * TextBlock's Text property to this.
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	FText GetPhaseTimerText() const;

	/** Visible while GetPhaseTimerText() is non-empty, Hidden otherwise - bind a widget's Visibility to this. */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	ESlateVisibility GetPhaseTimerVisibility() const;
};
