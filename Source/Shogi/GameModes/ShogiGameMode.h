// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShogiTypes.h"
#include "ShogiGameMode.generated.h"

class AShogiPlayerController;

/**
 * Native replacement for the Blueprint GameModeBase BP_MyGameMode.
 * Assigns the first connecting player to Sente and the second to Gote. Behavior for
 * a 3rd+ connecting player was not confirmed in the original Blueprint (see
 * docs/GameSpec.md) - they are simply left at PlayerSide::None (spectator) here.
 *
 * Mode auto-detection: this is the single GameMode assigned to Content/Map/Main.umap's
 * World Settings, used for both single-player and online play - which one happens is
 * decided by NetMode, not by a separate GameMode class/Blueprint option string:
 *  - NM_Standalone (Main opened directly, no EOSCore session): the sole local player is
 *    switched to local hot-seat (bControlBothSides = true, see AShogiPlayerController)
 *    since there is no second client that could ever log in as Gote.
 *  - NM_ListenServer / NM_Client (reached via an EOSCore session hosted/joined from the
 *    Title map): normal networked first-connects-Sente/second-connects-Gote assignment.
 */
UCLASS()
class SHOGI_API AShogiGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShogiGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	UPROPERTY(BlueprintReadOnly, Category = "Shogi")
	int32 JoinedPlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Shogi")
	TObjectPtr<AShogiPlayerController> SentePlayer;

	UPROPERTY(BlueprintReadOnly, Category = "Shogi")
	TObjectPtr<AShogiPlayerController> GotePlayer;

	/**
	 * Finds whichever connected controller currently acts for Side, used by
	 * AShogiBoardManager::AdvanceTurn to start that side's card phase (see
	 * docs/2026-08-14-card-system-phase-a.md). Checks each controller's actual PlayerSide/
	 * bControlBothSides rather than trusting the SentePlayer/GotePlayer slot names, since
	 * AShogiSinglePlayerVsAIGameMode overwrites PlayerSide to LocalHumanSide after the base
	 * class's PostLogin already stored the controller in the SentePlayer slot - the human
	 * could end up actually playing Gote while still sitting in the "SentePlayer" field.
	 * Returns nullptr if nobody is currently controlling Side (e.g. the AI's side in
	 * AShogiSinglePlayerVsAIGameMode, which moves via BoardManager directly, not a controller).
	 */
	UFUNCTION(BlueprintPure, Category = "Shogi")
	AShogiPlayerController* GetControllerForSide(EPlayerSide Side) const;
};
