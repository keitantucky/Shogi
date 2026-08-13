// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShogiCardTypes.generated.h"

/**
 * Card catalogue for the card-effect system layered on top of core Shogi rules
 * (see docs/GameConcept.md for the full 10-card design). All 10 values now have effect
 * logic in UShogiCardEffectLibrary/AShogiBoardManager::ApplyCardEffect - Phase A
 * (docs/2026-08-14-card-system-phase-a.md) implemented FuRocket/InstantAwakening/
 * TenpenChii/Hyena, Phase B (docs/2026-08-14-card-system-phase-b.md) added the rest.
 */
UENUM(BlueprintType)
enum class ECardType : uint8
{
	None					UMETA(DisplayName = "None"),
	FuRocket				UMETA(DisplayName = "Fu Rocket"),
	InstantAwakening		UMETA(DisplayName = "Instant Awakening"),
	TenpenChii				UMETA(DisplayName = "Tenpen Chii"),
	Hyena					UMETA(DisplayName = "Hyena"),
	RentalReservation		UMETA(DisplayName = "Rental Reservation"),
	PetrifyCurse			UMETA(DisplayName = "Petrify Curse"),
	PositionSwap			UMETA(DisplayName = "Position Swap"),
	MegatonImpact			UMETA(DisplayName = "Megaton Impact"),
	SelfDestructBomb		UMETA(DisplayName = "Self-Destruct Bomb"),
	TemporaryInvincibility	UMETA(DisplayName = "Temporary Invincibility")
};

/**
 * A single side's hand of drawn cards. Only the Hand needs to replicate (COND_OwnerOnly,
 * see AShogiPlayerController) - the deck order backing it is server-only and lives
 * separately (AShogiPlayerController::Deck_Sente/Deck_Gote) since it never needs to reach
 * any client.
 */
USTRUCT()
struct FShogiCardHandState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<ECardType> Hand;
};
