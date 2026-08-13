// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiBoardManager.h"
#include "ShogiPiece.h"
#include "ShogiBoardSetupRow.h"
#include "ShogiMoveMatrixRow.h"
#include "ShogiRulesLibrary.h"
#include "ShogiPromotionLibrary.h"
#include "ShogiGameState.h"
#include "Components/SceneComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

namespace
{
	EPlayerSide ParsePlayerSideName(const FString& Name)
	{
		if (Name == TEXT("Sente")) return EPlayerSide::Sente;
		if (Name == TEXT("Gote")) return EPlayerSide::Gote;
		return EPlayerSide::None;
	}

	EPieceType ParsePieceTypeName(const FString& Name)
	{
		if (Name == TEXT("Fu")) return EPieceType::Fu;
		if (Name == TEXT("Kyou")) return EPieceType::Kyou;
		if (Name == TEXT("Kei")) return EPieceType::Kei;
		if (Name == TEXT("Gin")) return EPieceType::Gin;
		if (Name == TEXT("Kin")) return EPieceType::Kin;
		if (Name == TEXT("Kaku")) return EPieceType::Kaku;
		if (Name == TEXT("Hisha")) return EPieceType::Hisha;
		if (Name == TEXT("Ou")) return EPieceType::Ou;
		return EPieceType::None;
	}
}

AShogiBoardManager::AShogiBoardManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = Root;

	SenteKomadai = CreateDefaultSubobject<USceneComponent>(TEXT("SenteKomadai"));
	SenteKomadai->SetupAttachment(RootComponent);

	GoteKomadai = CreateDefaultSubobject<USceneComponent>(TEXT("GoteKomadai"));
	GoteKomadai->SetupAttachment(RootComponent);

	PieceActorClass = AShogiPiece::StaticClass();

	BoardArray.SetNum(UShogiRulesLibrary::BoardCellCount);
	PieceActorArray.SetNum(UShogiRulesLibrary::BoardCellCount);
}

void AShogiBoardManager::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnPieces();
	}
}

FVector AShogiBoardManager::GetWorldLocationForIndex(int32 BoardIndex) const
{
	int32 X, Y;
	UShogiRulesLibrary::IndexToXY(BoardIndex, X, Y);
	// Centered on the middle square (4,4); tune BoardCellSizeX/Y / this actor's placement
	// in the editor to match the existing board mesh.
	return GetActorLocation() + FVector((X - 4) * BoardCellSizeX, (Y - 4) * BoardCellSizeY, 0.f);
}

AShogiPiece* AShogiBoardManager::SpawnPieceActor(const FShogiPieceData& PieceData, int32 BoardIndex)
{
	if (!PieceActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AShogiPiece* NewPiece = GetWorld()->SpawnActor<AShogiPiece>(
		PieceActorClass, GetWorldLocationForIndex(BoardIndex), FRotator::ZeroRotator, SpawnParams);

	if (NewPiece)
	{
		NewPiece->PieceData = PieceData;
		NewPiece->UpdateAppearance();
	}

	return NewPiece;
}

void AShogiBoardManager::SpawnPieces()
{
	if (!HasAuthority() || !InitialBoardSetupTable)
	{
		return;
	}

	BoardArray.SetNum(UShogiRulesLibrary::BoardCellCount);
	PieceActorArray.SetNum(UShogiRulesLibrary::BoardCellCount);

	for (int32 Y = 0; Y < UShogiRulesLibrary::BoardDimension; ++Y)
	{
		const FName RowName(*FString::Printf(TEXT("Y%d"), Y));
		const FShogiBoardSetupRow* Row = InitialBoardSetupTable->FindRow<FShogiBoardSetupRow>(RowName, TEXT("AShogiBoardManager::SpawnPieces"));
		if (!Row)
		{
			continue;
		}

		for (int32 X = 0; X < UShogiRulesLibrary::BoardDimension; ++X)
		{
			const FString* Cell = Row->GetColumn(X);
			if (!Cell || *Cell == TEXT("None") || Cell->IsEmpty())
			{
				continue;
			}

			FString SideName, TypeName;
			if (!Cell->Split(TEXT("_"), &SideName, &TypeName))
			{
				continue;
			}

			FShogiPieceData PieceData;
			PieceData.PlayerSide = ParsePlayerSideName(SideName);
			PieceData.PieceType = ParsePieceTypeName(TypeName);
			PieceData.bIsPromoted = false;

			if (PieceData.PlayerSide == EPlayerSide::None || PieceData.PieceType == EPieceType::None)
			{
				continue;
			}

			const int32 BoardIndex = UShogiRulesLibrary::XYToIndex(X, Y);
			BoardArray[BoardIndex] = PieceData;
			PieceActorArray[BoardIndex] = SpawnPieceActor(PieceData, BoardIndex);
		}
	}
}

void AShogiBoardManager::RefreshBoardVisual()
{
	for (int32 Index = 0; Index < PieceActorArray.Num(); ++Index)
	{
		if (AShogiPiece* Piece = PieceActorArray[Index])
		{
			Piece->SetActorLocation(GetWorldLocationForIndex(Index));
			Piece->UpdateAppearance();
		}
	}
}

void AShogiBoardManager::ShowMoveMarkers(const TArray<int32>& Indices)
{
	ClearMoveMarkers();

	UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug] ShowMoveMarkers: requested=%d MoveMarkerClass=%s"),
		Indices.Num(), MoveMarkerClass ? *MoveMarkerClass->GetName() : TEXT("NULL"));

	if (!MoveMarkerClass)
	{
		return;
	}

	for (int32 Index : Indices)
	{
		if (!UShogiRulesLibrary::IsValidBoardIndex(Index))
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AActor* Marker = GetWorld()->SpawnActor<AActor>(MoveMarkerClass, GetWorldLocationForIndex(Index), FRotator::ZeroRotator, SpawnParams))
		{
			ActiveMoveMarkers.Add(Marker);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug] ShowMoveMarkers: spawned=%d (ActiveMoveMarkers.Num()=%d)"), Indices.Num(), ActiveMoveMarkers.Num());
}

void AShogiBoardManager::ClearMoveMarkers()
{
	UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug] ClearMoveMarkers: destroying %d marker(s)"), ActiveMoveMarkers.Num());

	for (AActor* Marker : ActiveMoveMarkers)
	{
		if (Marker)
		{
			Marker->Destroy();
		}
	}
	ActiveMoveMarkers.Reset();
}

void AShogiBoardManager::UpdateStandLayout()
{
	auto LayoutHand = [this](TArray<TObjectPtr<AShogiPiece>>& HandActors, USceneComponent* Komadai)
	{
		if (!Komadai)
		{
			return;
		}
		for (int32 Index = 0; Index < HandActors.Num(); ++Index)
		{
			if (AShogiPiece* Piece = HandActors[Index])
			{
				// Offset is expressed along the Komadai's own local axes (not raw world X/Y) so the
				// stacking direction follows the stand's placement/rotation - GoteKomadai is rotated
				// to face the opposite way from SenteKomadai, so a world-space offset would stack
				// pieces in the wrong direction and overflow off the board.
				const FVector LocalOffset((Index % 3) * BoardCellSizeX, (Index / 3) * BoardCellSizeY, 0.f);
				const FVector WorldOffset = Komadai->GetComponentRotation().RotateVector(LocalOffset);
				Piece->SetActorLocation(Komadai->GetComponentLocation() + WorldOffset);
			}
		}
	};

	LayoutHand(HandPieceActors_Sente, SenteKomadai);
	LayoutHand(HandPieceActors_Gote, GoteKomadai);
}

UDataTable* AShogiBoardManager::GetMoveDataTableFor(const FShogiPieceData& Piece) const
{
	if (Piece.bIsPromoted)
	{
		switch (Piece.PieceType)
		{
			case EPieceType::Kaku:	return PromotedBishopMoveTable;
			case EPieceType::Hisha:return PromotedRookMoveTable;
			case EPieceType::Fu:
			case EPieceType::Kyou:
			case EPieceType::Kei:
			case EPieceType::Gin:
				return PromotedGenericMoveTable;
			default:
				break;
		}
	}

	if (const TObjectPtr<UDataTable>* Found = UnpromotedMoveTables.Find(Piece.PieceType))
	{
		return *Found;
	}
	return nullptr;
}

void AShogiBoardManager::DebugLogTableContents(UDataTable* Table, const FString& Label)
{
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug] %s: no table assigned (check the Details panel)."), *Label);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug] %s -> table '%s' (CheckCanMove assumes CsvRow=dY+9, CsvCol=dX+9; only \"〇\" cells are legal):"), *Label, *Table->GetName());

	int32 FoundCount = 0;
	for (int32 CsvRow = 0; CsvRow <= 18; ++CsvRow)
	{
		const FName RowName(*FString::Printf(TEXT("Y%d"), CsvRow));
		const FShogiMoveMatrixRow* Row = Table->FindRow<FShogiMoveMatrixRow>(RowName, TEXT("DebugLogAllMoveTables"));
		if (!Row)
		{
			continue;
		}

		for (int32 CsvCol = 0; CsvCol <= 18; ++CsvCol)
		{
			const FString* Cell = Row->GetColumn(CsvCol);
			if (Cell && *Cell == TEXT("〇"))
			{
				++FoundCount;
				UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug]     Row=Y%d Col=X%d  (dY=%d, dX=%d)  Value=\"%s\""),
					CsvRow, CsvCol, CsvRow - 9, CsvCol - 9, **Cell);
			}
		}
	}

	if (FoundCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug]     (no \"〇\" cells found at all - this DataTable may not have migrated correctly)"));
	}
}

void AShogiBoardManager::DebugLogAllMoveTables()
{
	UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug] ==== Dumping all movement tables ===="));

	static const EPieceType UnpromotedTypes[] =
	{
		EPieceType::Fu, EPieceType::Kyou, EPieceType::Kei, EPieceType::Gin,
		EPieceType::Kin, EPieceType::Kaku, EPieceType::Hisha, EPieceType::Ou
	};

	for (EPieceType PieceType : UnpromotedTypes)
	{
		FShogiPieceData Probe;
		Probe.PieceType = PieceType;
		Probe.PlayerSide = EPlayerSide::Sente;
		Probe.bIsPromoted = false;

		DebugLogTableContents(GetMoveDataTableFor(Probe), FString::Printf(TEXT("Unpromoted %s"), *UEnum::GetValueAsString(PieceType)));
	}

	static const EPieceType PromotableTypes[] =
	{
		EPieceType::Fu, EPieceType::Kyou, EPieceType::Kei, EPieceType::Gin, EPieceType::Kaku, EPieceType::Hisha
	};

	for (EPieceType PieceType : PromotableTypes)
	{
		FShogiPieceData Probe;
		Probe.PieceType = PieceType;
		Probe.PlayerSide = EPlayerSide::Sente;
		Probe.bIsPromoted = true;

		DebugLogTableContents(GetMoveDataTableFor(Probe), FString::Printf(TEXT("Promoted %s"), *UEnum::GetValueAsString(PieceType)));
	}

	UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug] ==== Done ===="));
}

AShogiGameState* AShogiBoardManager::GetShogiGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<AShogiGameState>() : nullptr;
}

void AShogiBoardManager::AdvanceTurn()
{
	if (AShogiGameState* GS = GetShogiGameState())
	{
		GS->CurrentTurn = (GS->CurrentTurn == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;
		GS->OnRep_CurrentTurn();
	}
}

void AShogiBoardManager::ApplyMove(int32 From, int32 To, EPlayerSide RequestingSide, bool bClientRequestedPromote)
{
	if (!HasAuthority())
	{
		return;
	}

	if (const AShogiGameState* GS = GetShogiGameState())
	{
		if (GS->bGameOver || GS->CurrentTurn != RequestingSide)
		{
			return;
		}
	}

	if (!UShogiRulesLibrary::IsValidBoardIndex(From) || !UShogiRulesLibrary::IsValidBoardIndex(To))
	{
		return;
	}

	const FShogiPieceData MovingPiece = BoardArray[From];
	if (MovingPiece.PlayerSide != RequestingSide || MovingPiece.PlayerSide == EPlayerSide::None)
	{
		return;
	}

	UDataTable* MoveTable = GetMoveDataTableFor(MovingPiece);
	if (!UShogiRulesLibrary::CheckCanMove(BoardArray, MovingPiece, From, To, MoveTable))
	{
		return;
	}
	if (!UShogiRulesLibrary::IsPathClear(BoardArray, From, To))
	{
		return;
	}

	const FShogiPieceData TargetCell = BoardArray[To];
	if (TargetCell.PlayerSide == MovingPiece.PlayerSide)
	{
		// Occupied by a friendly piece.
		return;
	}

	const bool bCapturedKing = (TargetCell.PieceType == EPieceType::Ou);

	// Capture.
	if (TargetCell.PlayerSide != EPlayerSide::None)
	{
		FShogiPieceData Captured = TargetCell;
		Captured.PlayerSide = MovingPiece.PlayerSide; // ownership flips to the capturing side
		Captured.bIsPromoted = false; // captured pieces always return to hand unpromoted

		AShogiPiece* CapturedActor = PieceActorArray[To];
		if (CapturedActor)
		{
			CapturedActor->bIsCaptured = true;
			CapturedActor->PieceData = Captured;
			CapturedActor->UpdateAppearance();
		}

		if (MovingPiece.PlayerSide == EPlayerSide::Sente)
		{
			CapturedPieces_Sente.Add(Captured);
			HandPieceActors_Sente.Add(CapturedActor);
		}
		else
		{
			CapturedPieces_Gote.Add(Captured);
			HandPieceActors_Gote.Add(CapturedActor);
		}
	}

	int32 FromX, FromY, ToX, ToY;
	UShogiRulesLibrary::IndexToXY(From, FromX, FromY);
	UShogiRulesLibrary::IndexToXY(To, ToX, ToY);

	bool bIsPromotable = false;
	int32 ForcedDepth = 0;
	UShogiPromotionLibrary::GetPromotionRuleForPieceType(MovingPiece.PieceType, bIsPromotable, ForcedDepth);

	const bool bIsSente = (MovingPiece.PlayerSide == EPlayerSide::Sente);
	const EShogiPromotionOutcome Outcome = UShogiPromotionLibrary::EvaluatePromotion(
		FromY, ToY, bIsSente, bIsPromotable, MovingPiece.bIsPromoted, /*bIsDrop=*/false, ForcedDepth);

	bool bFinalPromoted = MovingPiece.bIsPromoted;
	switch (Outcome)
	{
		case EShogiPromotionOutcome::ForcedPromotion:
			bFinalPromoted = true;
			break;
		case EShogiPromotionOutcome::OptionalPromotion:
			bFinalPromoted = bClientRequestedPromote;
			break;
		case EShogiPromotionOutcome::NoPromotion:
		default:
			bFinalPromoted = MovingPiece.bIsPromoted;
			break;
	}

	FShogiPieceData UpdatedPiece = MovingPiece;
	UpdatedPiece.bIsPromoted = bFinalPromoted;

	BoardArray[From] = FShogiPieceData();
	BoardArray[To] = UpdatedPiece;

	AShogiPiece* MovedActor = PieceActorArray[From];
	PieceActorArray[From] = nullptr;
	PieceActorArray[To] = MovedActor;

	if (MovedActor)
	{
		MovedActor->PieceData = UpdatedPiece;
		MovedActor->SetActorLocation(GetWorldLocationForIndex(To));
		MovedActor->UpdateAppearance();
	}

	UpdateStandLayout();

	if (bCapturedKing)
	{
		if (AShogiGameState* GS = GetShogiGameState())
		{
			GS->bGameOver = true;
			GS->Winner = MovingPiece.PlayerSide;
			GS->OnRep_GameOver();
		}
	}
	else
	{
		AdvanceTurn();
	}
}

void AShogiBoardManager::ApplyDrop(int32 DropIndex, AShogiPiece* DropPieceActor, EPieceType DropPieceType, EPlayerSide RequestingSide)
{
	if (!HasAuthority() || !DropPieceActor)
	{
		return;
	}

	if (const AShogiGameState* GS = GetShogiGameState())
	{
		if (GS->bGameOver || GS->CurrentTurn != RequestingSide)
		{
			return;
		}
	}

	if (!UShogiRulesLibrary::IsValidBoardIndex(DropIndex))
	{
		return;
	}

	if (BoardArray[DropIndex].PlayerSide != EPlayerSide::None)
	{
		// Destination occupied.
		return;
	}

	TArray<FShogiPieceData>& CapturedPieces = (RequestingSide == EPlayerSide::Sente) ? CapturedPieces_Sente : CapturedPieces_Gote;
	TArray<TObjectPtr<AShogiPiece>>& HandActors = (RequestingSide == EPlayerSide::Sente) ? HandPieceActors_Sente : HandPieceActors_Gote;

	const int32 HandIndex = HandActors.IndexOfByKey(DropPieceActor);
	if (HandIndex == INDEX_NONE)
	{
		return;
	}

	FShogiPieceData DroppedPiece;
	DroppedPiece.PieceType = DropPieceType;
	DroppedPiece.PlayerSide = RequestingSide;
	DroppedPiece.bIsPromoted = false; // dropped pieces are always unpromoted

	BoardArray[DropIndex] = DroppedPiece;
	PieceActorArray[DropIndex] = DropPieceActor;

	HandActors.RemoveAt(HandIndex);
	if (CapturedPieces.IsValidIndex(HandIndex))
	{
		CapturedPieces.RemoveAt(HandIndex);
	}

	DropPieceActor->bIsCaptured = false;
	DropPieceActor->PieceData = DroppedPiece;
	DropPieceActor->SetActorLocation(GetWorldLocationForIndex(DropIndex));
	DropPieceActor->UpdateAppearance();

	UpdateStandLayout();
	AdvanceTurn();
}

void AShogiBoardManager::OnRep_BoardArray()
{
	RefreshBoardVisual();
}

void AShogiBoardManager::OnRep_PieceActorArray()
{
	RefreshBoardVisual();
}

void AShogiBoardManager::OnRep_HandPieceActors_Sente()
{
	UpdateStandLayout();
}

void AShogiBoardManager::OnRep_HandPieceActors_Gote()
{
	UpdateStandLayout();
}

void AShogiBoardManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShogiBoardManager, BoardArray);
	DOREPLIFETIME(AShogiBoardManager, PieceActorArray);
	DOREPLIFETIME(AShogiBoardManager, CapturedPieces_Sente);
	DOREPLIFETIME(AShogiBoardManager, CapturedPieces_Gote);
	DOREPLIFETIME(AShogiBoardManager, HandPieceActors_Sente);
	DOREPLIFETIME(AShogiBoardManager, HandPieceActors_Gote);
}
