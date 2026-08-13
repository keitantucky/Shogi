// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
};
