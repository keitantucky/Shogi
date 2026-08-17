// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiPieceStatusWidgetBase.h"
#include "ShogiPiece.h"

void UShogiPieceStatusWidgetBase::SetOwningPiece(AShogiPiece* InOwningPiece)
{
	OwningPiece = InOwningPiece;
}

bool UShogiPieceStatusWidgetBase::GetIsPromoted() const
{
	return OwningPiece && OwningPiece->PieceData.bIsPromoted;
}

bool UShogiPieceStatusWidgetBase::GetHasFuRocketBoost() const
{
	return OwningPiece && OwningPiece->PieceData.bHasFuRocketBoost;
}

bool UShogiPieceStatusWidgetBase::GetIsPetrified() const
{
	return OwningPiece && OwningPiece->PieceData.bIsPetrified;
}

bool UShogiPieceStatusWidgetBase::GetIsInvincible() const
{
	return OwningPiece && OwningPiece->PieceData.bIsInvincible;
}

bool UShogiPieceStatusWidgetBase::GetIsLoanActive() const
{
	return OwningPiece && OwningPiece->PieceData.bLoanActive;
}

ESlateVisibility UShogiPieceStatusWidgetBase::GetPromotedVisibility() const
{
	return GetIsPromoted() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

ESlateVisibility UShogiPieceStatusWidgetBase::GetFuRocketBoostVisibility() const
{
	return GetHasFuRocketBoost() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

ESlateVisibility UShogiPieceStatusWidgetBase::GetPetrifiedVisibility() const
{
	return GetIsPetrified() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

ESlateVisibility UShogiPieceStatusWidgetBase::GetInvincibleVisibility() const
{
	return GetIsInvincible() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

ESlateVisibility UShogiPieceStatusWidgetBase::GetLoanActiveVisibility() const
{
	return GetIsLoanActive() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}
