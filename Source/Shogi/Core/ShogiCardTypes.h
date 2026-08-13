// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShogiCardTypes.generated.h"

/**
 * Card catalogue for the card-effect system layered on top of core Shogi rules
 * (see docs/GameConcept.md for the full 10-card design, docs/2026-08-14-card-system-phase-a.md
 * for what is actually wired up). Only FuRocket/InstantAwakening/TenpenChii/Hyena have
 * effect logic in UShogiCardEffectLibrary/AShogiBoardManager::ApplyCardEffect - the
 * remaining values exist so the enum/deck-composition code is future-proofed, but they
 * are never added to a deck and hit the `default` (no-op) case anywhere they're switched on.
 */
UENUM(BlueprintType)
enum class ECardType : uint8
{
	None					UMETA(DisplayName = "None"),
	FuRocket				UMETA(DisplayName = "Fu Rocket (Phase A)"),
	InstantAwakening		UMETA(DisplayName = "Instant Awakening (Phase A)"),
	TenpenChii				UMETA(DisplayName = "Tenpen Chii (Phase A)"),
	Hyena					UMETA(DisplayName = "Hyena (Phase A)"),
	RentalReservation		UMETA(DisplayName = "Rental Reservation (Phase B, not implemented)"),
	PetrifyCurse			UMETA(DisplayName = "Petrify Curse (Phase B, not implemented)"),
	PositionSwap			UMETA(DisplayName = "Position Swap (Phase B, not implemented)"),
	MegatonImpact			UMETA(DisplayName = "Megaton Impact (Phase B, not implemented)"),
	SelfDestructBomb		UMETA(DisplayName = "Self-Destruct Bomb (Phase B, not implemented)"),
	TemporaryInvincibility	UMETA(DisplayName = "Temporary Invincibility (Phase B, not implemented)")
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
