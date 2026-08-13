// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiSinglePlayerHotSeatGameMode.h"
#include "ShogiPlayerController.h"

void AShogiSinglePlayerHotSeatGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AShogiPlayerController* ShogiPC = Cast<AShogiPlayerController>(NewPlayer))
	{
		ShogiPC->PlayerSide = EPlayerSide::Sente;
		ShogiPC->bControlBothSides = true;
	}
}
