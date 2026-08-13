// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiPromotionPromptWidgetBase.h"
#include "Components/Button.h"

void UShogiPromotionPromptWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (YesButton)
	{
		YesButton->OnClicked.AddDynamic(this, &UShogiPromotionPromptWidgetBase::HandleYesClicked);
	}
	if (NoButton)
	{
		NoButton->OnClicked.AddDynamic(this, &UShogiPromotionPromptWidgetBase::HandleNoClicked);
	}
}

void UShogiPromotionPromptWidgetBase::HandleYesClicked()
{
	MakeChoice(true);
}

void UShogiPromotionPromptWidgetBase::HandleNoClicked()
{
	MakeChoice(false);
}

void UShogiPromotionPromptWidgetBase::MakeChoice(bool bPromote)
{
	OnPromotionChoiceMade.Broadcast(bPromote);
	RemoveFromParent();
}
