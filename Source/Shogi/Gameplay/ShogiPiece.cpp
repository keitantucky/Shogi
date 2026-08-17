// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "ShogiPieceStatusWidgetBase.h"
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

	StatusWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusWidgetComponent"));
	StatusWidgetComponent->SetupAttachment(PieceMesh);
	StatusWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	StatusWidgetComponent->SetDrawSize(FVector2D(32.f, 32.f));
	// Pivot (0,0) anchors the widget's top-left corner (rather than its center) at the
	// projected screen point set in UpdateAppearance, so the icon renders toward the
	// bottom-right of that point instead of straddling it.
	StatusWidgetComponent->SetPivot(FVector2D(0.f, 0.f));

	// WBP_PieceStatusIcon (parented to UShogiPieceStatusWidgetBase) is created later in the
	// Unreal Editor - not by this C++ change. Loaded via a soft path (rather than
	// ConstructorHelpers::FClassFinder, which would fatally assert on a missing asset) so
	// construction stays safe both before and after that widget Blueprint exists; the status
	// icon simply starts working once it's authored at this path with no further code changes.
	static const TSoftClassPtr<UUserWidget> StatusWidgetSoftClass(FSoftObjectPath(TEXT("/Game/BP/UMG/WBP_PieceStatusIcon.WBP_PieceStatusIcon_C")));
	if (UClass* LoadedStatusWidgetClass = StatusWidgetSoftClass.LoadSynchronous())
	{
		StatusWidgetComponent->SetWidgetClass(LoadedStatusWidgetClass);
	}
}

void AShogiPiece::BeginPlay()
{
	Super::BeginPlay();

	if (StatusWidgetComponent)
	{
		StatusWidgetComponent->InitWidget();
		if (UShogiPieceStatusWidgetBase* StatusWidget = Cast<UShogiPieceStatusWidgetBase>(StatusWidgetComponent->GetUserWidgetObject()))
		{
			StatusWidget->SetOwningPiece(this);
		}
	}
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

	if (StatusWidgetComponent)
	{
		// Anchor at the piece mesh's own vertical center (not floating high above it - a large
		// 3D Z offset made the icon's projected screen position swing noticeably relative to the
		// piece as the camera angle changed). SetPivot (set in the constructor) then nudges the
		// icon toward the bottom-right of this point in pure 2D screen space, which stays visually
		// attached to the piece across camera angles since the anchor barely moves relative to it.
		// Positioned via SetWorldLocation (not SetRelativeLocation) specifically so the Pitch/Yaw
		// flip above - a purely cosmetic mesh flip, not a real reorientation of the piece - doesn't
		// drag the icon along with it; StatusWidgetComponent is attached to PieceMesh, so a
		// relative offset would otherwise be rotated by this same transform.
		float CenterZ = 0.f;
		if (const UStaticMesh* Mesh = PieceMesh->GetStaticMesh())
		{
			CenterZ = Mesh->GetBounds().Origin.Z;
		}
		StatusWidgetComponent->SetWorldLocation(GetActorLocation() + FVector(0.f, 0.f, CenterZ));
	}
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
