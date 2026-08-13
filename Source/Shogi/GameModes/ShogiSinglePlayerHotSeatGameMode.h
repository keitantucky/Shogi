// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShogiGameMode.h"
#include "ShogiSinglePlayerHotSeatGameMode.generated.h"

/**
 * Local pass-and-play GameMode: the single local player controls both Sente and Gote,
 * alternating by turn (see AShogiPlayerController::bControlBothSides).
 *
 * Not wired into any menu - open Content/Map/Main.umap with this class as the GameMode
 * override (e.g. UGameplayStatics::OpenLevel with Options="game=ShogiSinglePlayerHotSeatGameMode").
 */
UCLASS()
class SHOGI_API AShogiSinglePlayerHotSeatGameMode : public AShogiGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
