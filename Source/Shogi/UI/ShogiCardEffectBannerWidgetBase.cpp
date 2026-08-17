// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiCardEffectBannerWidgetBase.h"
#include "ShogiCardEffectLibrary.h"

void UShogiCardEffectBannerWidgetBase::ShowCardEffect(ECardType CardType, EPlayerSide Side)
{
	CurrentCardType = CardType;
	CurrentSide = Side;

	SetVisibility(ESlateVisibility::HitTestInvisible);
	OnCardEffectActivatedEvent();

	GetWorld()->GetTimerManager().ClearTimer(AutoDismissTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(AutoDismissTimerHandle, this, &UShogiCardEffectBannerWidgetBase::OnCardEffectDismissEvent, AutoDismissSeconds, false);
}

FText UShogiCardEffectBannerWidgetBase::GetCardNameText() const
{
	return UShogiCardEffectLibrary::GetCardDisplayName(CurrentCardType);
}

FText UShogiCardEffectBannerWidgetBase::GetCardDescriptionText() const
{
	return UShogiCardEffectLibrary::GetCardDescription(CurrentCardType);
}

EPlayerSide UShogiCardEffectBannerWidgetBase::GetActivatingSide() const
{
	return CurrentSide;
}

void UShogiCardEffectBannerWidgetBase::OnCardEffectDismissEvent_Implementation()
{
	HideCardEffect();
}

void UShogiCardEffectBannerWidgetBase::HideCardEffect()
{
	GetWorld()->GetTimerManager().ClearTimer(AutoDismissTimerHandle);
	SetVisibility(ESlateVisibility::Collapsed);
}
