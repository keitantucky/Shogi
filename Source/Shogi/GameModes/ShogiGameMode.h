// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShogiGameMode.generated.h"

class AShogiPlayerController;

/**
 * Native replacement for the Blueprint GameModeBase BP_MyGameMode.
 * Assigns the first connecting player to Sente and the second to Gote. Behavior for
 * a 3rd+ connecting player was not confirmed in the original Blueprint (see
 * docs/GameSpec.md) - they are simply left at PlayerSide::None (spectator) here.
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
};
