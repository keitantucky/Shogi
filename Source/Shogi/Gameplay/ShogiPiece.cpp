// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AShogiPiece::AShogiPiece()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	PieceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PieceMesh"));
	RootComponent = PieceMesh;

	// Exact asset paths confirmed from the existing BP_Piece::UpdateAppearance mesh
	// switch (including the original asset pack's "Shilver" typo).
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FuFinder(TEXT("/Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPiecePawn"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> KyouFinder(TEXT("/Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceLance"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> KeiFinder(TEXT("/Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceKnight"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GinFinder(TEXT("/Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceShilverGeneral"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> KinFinder(TEXT("/Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceGoldGeneral"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> KakuFinder(TEXT("/Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceBishop"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HishaFinder(TEXT("/Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceRook"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> OuFinder(TEXT("/Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceKing"));

	FuMesh = FuFinder.Object;
	KyouMesh = KyouFinder.Object;
	KeiMesh = KeiFinder.Object;
	GinMesh = GinFinder.Object;
	KinMesh = KinFinder.Object;
	KakuMesh = KakuFinder.Object;
	HishaMesh = HishaFinder.Object;
	OuMesh = OuFinder.Object;
}

UStaticMesh* AShogiPiece::GetMeshForPieceType(EPieceType PieceType) const
{
	switch (PieceType)
	{
		case EPieceType::Fu:	return FuMesh;
		case EPieceType::Kyou:	return KyouMesh;
		case EPieceType::Kei:	return KeiMesh;
		case EPieceType::Gin:	return GinMesh;
		case EPieceType::Kin:	return KinMesh;
		case EPieceType::Kaku:	return KakuMesh;
		case EPieceType::Hisha:return HishaMesh;
		case EPieceType::Ou:	return OuMesh;
		default:				return nullptr;
	}
}

void AShogiPiece::UpdateAppearance()
{
	// No promoted-piece meshes exist in the current asset pack (see docs/GameSpec.md
	// known gaps) - promoted pieces currently reuse their unpromoted mesh.
	if (UStaticMesh* Mesh = GetMeshForPieceType(PieceData.PieceType))
	{
		PieceMesh->SetStaticMesh(Mesh);
	}

	const float Yaw = (PieceData.PlayerSide == EPlayerSide::Gote) ? 180.f : 0.f;

	// Promoted pieces are flipped over (like a real double-sided Shogi piece) by pitching
	// 180 degrees around the piece's own left-right axis. Applying Pitch before Yaw
	// (FRotator composes Roll, then Pitch, then Yaw) keeps this flip correct regardless of
	// which way the piece is currently facing.
	const float Pitch = PieceData.bIsPromoted ? 180.f : 0.f;
	SetActorRelativeRotation(FRotator(Pitch, Yaw, 0.f));
}

void AShogiPiece::SetPromoted(bool bNewPromoted)
{
	PieceData.bIsPromoted = bNewPromoted;
	UpdateAppearance();
}

void AShogiPiece::OnRep_PieceData()
{
	UpdateAppearance();
}

void AShogiPiece::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShogiPiece, PieceData);
	DOREPLIFETIME(AShogiPiece, bIsCaptured);
}
