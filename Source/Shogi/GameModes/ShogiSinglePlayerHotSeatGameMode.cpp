// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiSinglePlayerHotSeatGameMode.h"
#include "ShogiPlayerController.h"

void AShogiSinglePlayerHotSeatGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Deliberately skips AShogiGameMode::PostLogin (calls the grandparent directly) so its
	// embedded TryStartInitialCardPhase() call doesn't fire before this override's own
	// PlayerSide/bControlBothSides correction below - see AShogiGameMode::TryStartInitialCardPhase.
	AGameModeBase::PostLogin(NewPlayer);
	AssignPlayerSideOnLogin(NewPlayer);

	if (AShogiPlayerController* ShogiPC = Cast<AShogiPlayerController>(NewPlayer))
	{
		ShogiPC->PlayerSide = EPlayerSide::Sente;
		ShogiPC->bControlBothSides = true;
	}

	TryStartInitialCardPhase();
}
