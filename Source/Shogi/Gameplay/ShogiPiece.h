// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShogiTypes.h"
#include "ShogiPiece.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UWidgetComponent;

/**
 * Native replacement for the Blueprint Actor BP_Piece.
 *
 * Visual notes ported from the existing Blueprint: piece meshes are the purchased
 * Shogi_JapaneseChess asset pack (8 meshes, one per unpromoted piece type - there is
 * currently no promoted-piece mesh in the project, see docs/GameSpec.md known gaps).
 * Gote's pieces are rotated 180 degrees on Yaw so they visually face the opponent,
 * matching real Shogi piece orientation conventions.
 */
UCLASS()
class SHOGI_API AShogiPiece : public AActor
{
	GENERATED_BODY()

public:
	AShogiPiece();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_PieceData, Category = "Shogi")
	FShogiPieceData PieceData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Shogi")
	bool bIsCaptured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shogi")
	TObjectPtr<UStaticMeshComponent> PieceMesh;

	/**
	 * Screen-space status-icon widget (石化/無敵/歩兵ロケット強化/貸出中 etc. - see
	 * UShogiPieceStatusWidgetBase), hovering above the piece. Purely local/visual, so it needs
	 * no replication - each client's own AShogiPiece instance reads PieceData directly.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shogi")
	TObjectPtr<UWidgetComponent> StatusWidgetComponent;

	/** Sets PieceMesh/rotation to match PieceData. Called after PieceData changes locally or via replication. */
	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void UpdateAppearance();

	/** Sets PieceData.bIsPromoted and refreshes appearance. */
	UFUNCTION(BlueprintCallable, Category = "Shogi")
	void SetPromoted(bool bNewPromoted);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_PieceData();

private:
	UPROPERTY()
	TObjectPtr<UStaticMesh> FuMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> KyouMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> KeiMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> GinMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> KinMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> KakuMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> HishaMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> OuMesh;

	UStaticMesh* GetMeshForPieceType(EPieceType PieceType) const;
};
