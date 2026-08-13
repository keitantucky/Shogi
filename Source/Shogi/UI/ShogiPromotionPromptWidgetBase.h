// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShogiPromotionPromptWidgetBase.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPromotionChoiceMade, bool, bPromote);

/**
 * Native logic base class for a new WBP_PromotionPrompt widget (does not exist yet -
 * must be created in the editor with a parent class of this type, containing two
 * buttons named YesButton/NoButton). Broadcasts OnPromotionChoiceMade and removes
 * itself once the player picks an option. See docs/GameSpec.md known gaps.
 */
UCLASS()
class SHOGI_API UShogiPromotionPromptWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Shogi")
	FOnPromotionChoiceMade OnPromotionChoiceMade;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> YesButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NoButton;

private:
	UFUNCTION()
	void HandleYesClicked();

	UFUNCTION()
	void HandleNoClicked();

	void MakeChoice(bool bPromote);
};
