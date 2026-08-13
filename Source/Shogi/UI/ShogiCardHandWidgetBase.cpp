// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiCardHandWidgetBase.h"
#include "ShogiPlayerController.h"
#include "ShogiCardEffectLibrary.h"

void UShogiCardHandWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	OwningShogiController = Cast<AShogiPlayerController>(GetOwningPlayer());
}

AShogiPlayerController* UShogiCardHandWidgetBase::GetShogiController() const
{
	return OwningShogiController;
}

TArray<FText> UShogiCardHandWidgetBase::GetMyHandDisplayNames() const
{
	TArray<FText> Names;
	if (const AShogiPlayerController* PC = GetShogiController())
	{
		for (ECardType CardType : PC->GetMyHand())
		{
			Names.Add(UShogiCardEffectLibrary::GetCardDisplayName(CardType));
		}
	}
	return Names;
}

FText UShogiCardHandWidgetBase::GetHandSlotDisplayName(int32 HandSlot) const
{
	if (const AShogiPlayerController* PC = GetShogiController())
	{
		const TArray<ECardType>& Hand = PC->GetMyHand();
		if (Hand.IsValidIndex(HandSlot))
		{
			return UShogiCardEffectLibrary::GetCardDisplayName(Hand[HandSlot]);
		}
	}
	return FText::GetEmpty();
}

bool UShogiCardHandWidgetBase::IsCardPlayableAt(int32 HandSlot) const
{
	const AShogiPlayerController* PC = GetShogiController();
	if (!PC)
	{
		return false;
	}
	const TArray<ECardType>& Hand = PC->GetMyHand();
	return Hand.IsValidIndex(HandSlot) && PC->IsCardPlayable(Hand[HandSlot]);
}

void UShogiCardHandWidgetBase::OnCardButtonClicked(int32 HandSlot)
{
	AShogiPlayerController* PC = GetShogiController();
	if (!PC)
	{
		return;
	}
	const TArray<ECardType>& Hand = PC->GetMyHand();
	if (Hand.IsValidIndex(HandSlot))
	{
		PC->SelectCardToPlay(Hand[HandSlot]);
	}
}

void UShogiCardHandWidgetBase::OnPassButtonClicked()
{
	if (AShogiPlayerController* PC = GetShogiController())
	{
		PC->Server_PassCardPhase();
	}
}

FText UShogiCardHandWidgetBase::GetCardPhaseWarningText() const
{
	if (const AShogiPlayerController* PC = GetShogiController())
	{
		return PC->GetCardPhaseWarningText();
	}
	return FText::GetEmpty();
}

ESlateVisibility UShogiCardHandWidgetBase::GetCardPhaseWarningVisibility() const
{
	return GetCardPhaseWarningText().IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::Visible;
}
