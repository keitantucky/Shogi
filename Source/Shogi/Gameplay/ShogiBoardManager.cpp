// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiBoardManager.h"
#include "ShogiPiece.h"
#include "ShogiBoardSetupRow.h"
#include "ShogiMoveMatrixRow.h"
#include "ShogiRulesLibrary.h"
#include "ShogiPromotionLibrary.h"
#include "ShogiCardEffectLibrary.h"
#include "ShogiGameState.h"
#include "ShogiGameMode.h"
#include "ShogiPlayerController.h"
#include "Components/SceneComponent.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"
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

void AShogiBoardManager::TickTimedCardEffects(EPlayerSide NewCurrentTurn)
{
	// See docs/2026-08-14-card-system-phase-b.md 3.6. Runs once per AdvanceTurn call, right
	// after CurrentTurn flips, so a reservation activating/reverting can affect this turn's
	// check status before it's computed below.
	for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
	{
		FShogiPieceData& Piece = BoardArray[Index];
		bool bChanged = false;

		// Temporary Invincibility expires the moment its owner's own next turn begins.
		if (Piece.PlayerSide == NewCurrentTurn && Piece.bIsInvincible)
		{
			Piece.bIsInvincible = false;
			bChanged = true;
		}

		// Rental Reservation: tick / activate / revert.
		if (Piece.PendingLoanCasterSide != EPlayerSide::None)
		{
			if (Piece.bLoanActive)
			{
				// The loan turn just ended - return the piece to its original owner.
				Piece.PlayerSide = Piece.LoanOriginalOwnerSide;
				Piece.PendingLoanCasterSide = EPlayerSide::None;
				Piece.PendingLoanTurnsElapsed = 0;
				Piece.bLoanActive = false;
				Piece.LoanOriginalOwnerSide = EPlayerSide::None;
				bChanged = true;
			}
			else
			{
				++Piece.PendingLoanTurnsElapsed;
				if (Piece.PendingLoanTurnsElapsed >= 2 && NewCurrentTurn == Piece.PendingLoanCasterSide)
				{
					Piece.LoanOriginalOwnerSide = Piece.PlayerSide;
					Piece.PlayerSide = Piece.PendingLoanCasterSide;
					Piece.bLoanActive = true;
					bChanged = true;
				}
			}
		}

		if (bChanged)
		{
			if (AShogiPiece* Actor = PieceActorArray.IsValidIndex(Index) ? PieceActorArray[Index] : nullptr)
			{
				Actor->PieceData = Piece;
				Actor->UpdateAppearance();
			}
		}
	}
}

void AShogiBoardManager::AdvanceTurn()
{
	if (AShogiGameState* GS = GetShogiGameState())
	{
		GS->CurrentTurn = (GS->CurrentTurn == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;

		TickTimedCardEffects(GS->CurrentTurn);

		GS->bInCheck = IsKingInCheck(GS->CurrentTurn);

		StartCardPhaseForCurrentTurn();

		GS->OnRep_CurrentTurn();
	}
}

void AShogiBoardManager::StartCardPhaseForCurrentTurn()
{
	AShogiGameState* GS = GetShogiGameState();
	if (!GS)
	{
		return;
	}

	// (Re)start CurrentTurn's card phase (see docs/2026-08-14-card-system-phase-a.md, hand
	// mechanic revised per user request: redraw a fresh 3-card hand into whichever controller
	// currently plays that side, and require it to resolve (play a card or pass) before a
	// move/drop is accepted. If nobody controls that side (the AI's side in
	// AShogiSinglePlayerVsAIGameMode, which moves via ApplyMove/ApplyDrop directly rather than
	// through a controller), auto-resolve so the AI isn't blocked.
	GS->bCardPhaseResolved = false;

	// Any previous phase's timeout timer is no longer relevant (see
	// docs/2026-08-14-card-system-phase-b.md 3.7).
	GetWorldTimerManager().ClearTimer(CardPhaseTimerHandle);
	GetWorldTimerManager().ClearTimer(MovePhaseTimerHandle);

	if (AShogiGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AShogiGameMode>() : nullptr)
	{
		if (AShogiPlayerController* PC = GM->GetControllerForSide(GS->CurrentTurn))
		{
			PC->RedrawHandForSide(GS->CurrentTurn);
			GetWorldTimerManager().SetTimer(CardPhaseTimerHandle, this, &AShogiBoardManager::HandleCardPhaseTimeout, CardPhaseTimeoutSeconds, false);
			SetPhaseTimerEndTime(CardPhaseTimeoutSeconds);
			return;
		}
	}

	// No controller exists for CurrentTurn (the AI's side in AShogiSinglePlayerVsAIGameMode, or
	// called before any controller has logged in) - leave bCardPhaseResolved false here; the
	// lazy check in ApplyMove/ApplyDrop resolves it the moment the AI's direct ApplyMove/
	// ApplyDrop call arrives. No timer is started - AShogiSinglePlayerVsAIGameMode's own
	// MakeAIMove timer/flow already covers it, and there's no countdown to display for it either.
	ClearPhaseTimerEndTime();
}

void AShogiBoardManager::OnCardPhaseResolved(EPlayerSide Side)
{
	GetWorldTimerManager().ClearTimer(CardPhaseTimerHandle);

	if (AShogiGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AShogiGameMode>() : nullptr)
	{
		if (GM->GetControllerForSide(Side))
		{
			GetWorldTimerManager().SetTimer(MovePhaseTimerHandle, this, &AShogiBoardManager::HandleMovePhaseTimeout, MovePhaseTimeoutSeconds, false);
			SetPhaseTimerEndTime(MovePhaseTimeoutSeconds);
			return;
		}
	}

	ClearPhaseTimerEndTime();
}

void AShogiBoardManager::SetPhaseTimerEndTime(float DurationSeconds)
{
	if (AShogiGameState* GS = GetShogiGameState())
	{
		GS->CurrentPhaseTimerEndTime = GS->GetServerWorldTimeSeconds() + DurationSeconds;
	}
}

void AShogiBoardManager::ClearPhaseTimerEndTime()
{
	if (AShogiGameState* GS = GetShogiGameState())
	{
		GS->CurrentPhaseTimerEndTime = -1.f;
	}
}

void AShogiBoardManager::HandleCardPhaseTimeout()
{
	AShogiGameState* GS = GetShogiGameState();
	if (!GS || GS->bGameOver || GS->bCardPhaseResolved)
	{
		return;
	}

	if (AShogiGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AShogiGameMode>() : nullptr)
	{
		if (AShogiPlayerController* PC = GM->GetControllerForSide(GS->CurrentTurn))
		{
			PC->ForceRandomCardOrPass();
		}
	}
}

void AShogiBoardManager::HandleMovePhaseTimeout()
{
	AShogiGameState* GS = GetShogiGameState();
	if (!GS || GS->bGameOver || !GS->bCardPhaseResolved)
	{
		return;
	}

	FShogiLegalAction Chosen;
	if (!PickRandomLegalAction(GS->CurrentTurn, Chosen))
	{
		// Stalemate-equivalent - matches AShogiSinglePlayerVsAIGameMode::MakeAIMove's handling.
		return;
	}

	if (Chosen.bIsDrop)
	{
		ApplyDrop(Chosen.To, Chosen.DropActor, Chosen.DropPieceType, GS->CurrentTurn);
	}
	else
	{
		ApplyMove(Chosen.From, Chosen.To, GS->CurrentTurn, /*bClientRequestedPromote=*/true);
	}
}

bool AShogiBoardManager::ResolveCardPhaseIfUncontrolled(AShogiGameState* GS, EPlayerSide Side)
{
	if (!GS)
	{
		return false;
	}
	if (GS->bCardPhaseResolved)
	{
		return true;
	}

	AShogiGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AShogiGameMode>() : nullptr;
	AShogiPlayerController* PC = GM ? GM->GetControllerForSide(Side) : nullptr;
	if (PC)
	{
		// A controller plays this side - it must resolve the card phase itself first.
		return false;
	}

	// Nobody controls this side yet/at all (AI's direct-call side, or evaluated before any
	// controller has logged in) - auto-resolve so the caller (e.g. MakeAIMove) isn't blocked.
	GS->bCardPhaseResolved = true;
	return true;
}

bool AShogiBoardManager::ApplyCardEffect(ECardType CardType, EPlayerSide RequestingSide, int32 TargetBoardIndex, AShogiPiece* TargetHandPieceActor)
{
	if (!HasAuthority())
	{
		return false;
	}

	AShogiGameState* GS = GetShogiGameState();
	if (!GS || GS->bGameOver || GS->CurrentTurn != RequestingSide || GS->bCardPhaseResolved)
	{
		return false;
	}

	const EPlayerSide OpponentSide = (RequestingSide == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;

	switch (CardType)
	{
		case ECardType::FuRocket:
		{
			if (!UShogiCardEffectLibrary::IsValidFuRocketTarget(BoardArray, RequestingSide, TargetBoardIndex))
			{
				return false;
			}
			UShogiCardEffectLibrary::ApplyFuRocket(BoardArray, TargetBoardIndex);
			if (AShogiPiece* Actor = PieceActorArray.IsValidIndex(TargetBoardIndex) ? PieceActorArray[TargetBoardIndex] : nullptr)
			{
				Actor->PieceData = BoardArray[TargetBoardIndex];
				Actor->UpdateAppearance();
			}
			return true;
		}

		case ECardType::InstantAwakening:
		{
			if (!UShogiCardEffectLibrary::IsValidInstantAwakeningTarget(BoardArray, RequestingSide, TargetBoardIndex))
			{
				return false;
			}
			UShogiCardEffectLibrary::ApplyInstantAwakening(BoardArray, TargetBoardIndex);
			if (AShogiPiece* Actor = PieceActorArray.IsValidIndex(TargetBoardIndex) ? PieceActorArray[TargetBoardIndex] : nullptr)
			{
				Actor->PieceData = BoardArray[TargetBoardIndex];
				Actor->UpdateAppearance();
			}
			return true;
		}

		case ECardType::TenpenChii:
		{
			UShogiCardEffectLibrary::ApplyTenpenChii(BoardArray);
			for (int32 Index = 0; Index < PieceActorArray.Num(); ++Index)
			{
				if (AShogiPiece* Actor = PieceActorArray[Index])
				{
					Actor->PieceData = BoardArray[Index];
					Actor->UpdateAppearance();
				}
			}
			GS->bInCheck = IsKingInCheck(GS->CurrentTurn);
			return true;
		}

		case ECardType::Hyena:
		{
			TArray<FShogiPieceData>& OpponentCaptured = (OpponentSide == EPlayerSide::Sente) ? CapturedPieces_Sente : CapturedPieces_Gote;
			TArray<TObjectPtr<AShogiPiece>>& OpponentHand = (OpponentSide == EPlayerSide::Sente) ? HandPieceActors_Sente : HandPieceActors_Gote;
			TArray<FShogiPieceData>& OwnCaptured = (RequestingSide == EPlayerSide::Sente) ? CapturedPieces_Sente : CapturedPieces_Gote;
			TArray<TObjectPtr<AShogiPiece>>& OwnHand = (RequestingSide == EPlayerSide::Sente) ? HandPieceActors_Sente : HandPieceActors_Gote;

			if (!UShogiCardEffectLibrary::HasValidTarget(CardType, BoardArray, OpponentCaptured, RequestingSide))
			{
				return false;
			}

			const int32 StolenIndex = OpponentHand.IndexOfByKey(TargetHandPieceActor);
			if (StolenIndex == INDEX_NONE || !OpponentCaptured.IsValidIndex(StolenIndex))
			{
				return false;
			}

			FShogiPieceData Stolen = OpponentCaptured[StolenIndex];
			Stolen.PlayerSide = RequestingSide;

			OpponentHand.RemoveAt(StolenIndex);
			OpponentCaptured.RemoveAt(StolenIndex);

			TargetHandPieceActor->PieceData = Stolen;
			TargetHandPieceActor->UpdateAppearance();

			OwnHand.Add(TargetHandPieceActor);
			OwnCaptured.Add(Stolen);

			UpdateStandLayout();
			return true;
		}

		case ECardType::PetrifyCurse:
		{
			if (!UShogiCardEffectLibrary::IsValidPetrifyCurseTarget(BoardArray, RequestingSide, TargetBoardIndex))
			{
				return false;
			}
			UShogiCardEffectLibrary::ApplyPetrifyCurse(BoardArray, TargetBoardIndex);
			if (AShogiPiece* Actor = PieceActorArray.IsValidIndex(TargetBoardIndex) ? PieceActorArray[TargetBoardIndex] : nullptr)
			{
				Actor->PieceData = BoardArray[TargetBoardIndex];
				Actor->UpdateAppearance();
			}
			return true;
		}

		case ECardType::PositionSwap:
		{
			if (!UShogiCardEffectLibrary::IsValidPositionSwapTarget(BoardArray, RequestingSide, TargetBoardIndex))
			{
				return false;
			}

			int32 KingIndex = INDEX_NONE;
			for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
			{
				if (BoardArray[Index].PlayerSide == RequestingSide && BoardArray[Index].PieceType == EPieceType::Ou)
				{
					KingIndex = Index;
					break;
				}
			}
			if (KingIndex == INDEX_NONE || KingIndex == TargetBoardIndex)
			{
				return false;
			}

			UShogiCardEffectLibrary::ApplyPositionSwap(BoardArray, KingIndex, TargetBoardIndex);

			if (PieceActorArray.IsValidIndex(KingIndex) && PieceActorArray.IsValidIndex(TargetBoardIndex))
			{
				AShogiPiece* KingActor = PieceActorArray[KingIndex];
				AShogiPiece* FuActor = PieceActorArray[TargetBoardIndex];
				PieceActorArray[KingIndex] = FuActor;
				PieceActorArray[TargetBoardIndex] = KingActor;

				if (KingActor)
				{
					KingActor->PieceData = BoardArray[TargetBoardIndex];
					KingActor->SetActorLocation(GetWorldLocationForIndex(TargetBoardIndex));
					KingActor->UpdateAppearance();
				}
				if (FuActor)
				{
					FuActor->PieceData = BoardArray[KingIndex];
					FuActor->SetActorLocation(GetWorldLocationForIndex(KingIndex));
					FuActor->UpdateAppearance();
				}
			}
			return true;
		}

		case ECardType::MegatonImpact:
		{
			if (!UShogiCardEffectLibrary::IsValidMegatonImpactTarget(BoardArray, TargetBoardIndex))
			{
				return false;
			}

			AShogiPiece* Actor = PieceActorArray.IsValidIndex(TargetBoardIndex) ? PieceActorArray[TargetBoardIndex] : nullptr;
			const int32 NewIndex = UShogiCardEffectLibrary::ApplyMegatonImpact(BoardArray, TargetBoardIndex);

			if (NewIndex != TargetBoardIndex)
			{
				PieceActorArray[TargetBoardIndex] = nullptr;
				PieceActorArray[NewIndex] = Actor;
				if (Actor)
				{
					Actor->PieceData = BoardArray[NewIndex];
					Actor->SetActorLocation(GetWorldLocationForIndex(NewIndex));
					Actor->UpdateAppearance();
				}
			}
			// Still counts as a used card even if fully blocked (0-square knockback) - see
			// docs/2026-08-14-card-system-phase-b.md 2.4.
			return true;
		}

		case ECardType::SelfDestructBomb:
		{
			if (!UShogiCardEffectLibrary::IsValidSelfDestructBombTarget(BoardArray, RequestingSide, TargetBoardIndex))
			{
				return false;
			}

			int32 CenterX, CenterY;
			UShogiRulesLibrary::IndexToXY(TargetBoardIndex, CenterX, CenterY);

			TArray<int32> BlastIndices;
			BlastIndices.Add(TargetBoardIndex);
			for (int32 DX = -1; DX <= 1; ++DX)
			{
				for (int32 DY = -1; DY <= 1; ++DY)
				{
					if (DX == 0 && DY == 0)
					{
						continue;
					}
					const int32 NeighborX = CenterX + DX;
					const int32 NeighborY = CenterY + DY;
					if (NeighborX < 0 || NeighborX >= UShogiRulesLibrary::BoardDimension
						|| NeighborY < 0 || NeighborY >= UShogiRulesLibrary::BoardDimension)
					{
						continue;
					}
					const int32 NeighborIndex = UShogiRulesLibrary::XYToIndex(NeighborX, NeighborY);
					if (BoardArray[NeighborIndex].PlayerSide != EPlayerSide::None && !BoardArray[NeighborIndex].bIsInvincible)
					{
						BlastIndices.Add(NeighborIndex);
					}
				}
			}

			bool bSenteKingDestroyed = false;
			bool bGoteKingDestroyed = false;

			for (int32 Index : BlastIndices)
			{
				const FShogiPieceData& Victim = BoardArray[Index];
				if (Victim.PieceType == EPieceType::Ou)
				{
					if (Victim.PlayerSide == EPlayerSide::Sente)
					{
						bSenteKingDestroyed = true;
					}
					else if (Victim.PlayerSide == EPlayerSide::Gote)
					{
						bGoteKingDestroyed = true;
					}
				}

				if (AShogiPiece* Actor = PieceActorArray.IsValidIndex(Index) ? PieceActorArray[Index] : nullptr)
				{
					Actor->Destroy();
				}
				PieceActorArray[Index] = nullptr;
				BoardArray[Index] = FShogiPieceData();
			}

			if (bSenteKingDestroyed || bGoteKingDestroyed)
			{
				GS->bGameOver = true;
				if (bSenteKingDestroyed && bGoteKingDestroyed)
				{
					// Both Kings caught in the same blast - see docs/2026-08-14-card-system-phase-b.md 2.5.
					GS->Winner = EPlayerSide::None;
				}
				else
				{
					GS->Winner = bSenteKingDestroyed ? EPlayerSide::Gote : EPlayerSide::Sente;
				}
				GS->OnRep_GameOver();
				GetWorldTimerManager().ClearTimer(CardPhaseTimerHandle);
				GetWorldTimerManager().ClearTimer(MovePhaseTimerHandle);
				ClearPhaseTimerEndTime();
			}

			return true;
		}

		case ECardType::TemporaryInvincibility:
		{
			if (!UShogiCardEffectLibrary::IsValidTemporaryInvincibilityTarget(BoardArray, RequestingSide, TargetBoardIndex))
			{
				return false;
			}
			UShogiCardEffectLibrary::ApplyTemporaryInvincibility(BoardArray, TargetBoardIndex);
			if (AShogiPiece* Actor = PieceActorArray.IsValidIndex(TargetBoardIndex) ? PieceActorArray[TargetBoardIndex] : nullptr)
			{
				Actor->PieceData = BoardArray[TargetBoardIndex];
				Actor->UpdateAppearance();
			}
			return true;
		}

		case ECardType::RentalReservation:
		{
			if (!UShogiCardEffectLibrary::IsValidRentalReservationTarget(BoardArray, RequestingSide, TargetBoardIndex))
			{
				return false;
			}
			BoardArray[TargetBoardIndex].PendingLoanCasterSide = RequestingSide;
			BoardArray[TargetBoardIndex].PendingLoanTurnsElapsed = 0;
			if (AShogiPiece* Actor = PieceActorArray.IsValidIndex(TargetBoardIndex) ? PieceActorArray[TargetBoardIndex] : nullptr)
			{
				// No appearance change - the reservation is invisible until it activates
				// (see AShogiBoardManager::AdvanceTurn).
				Actor->PieceData = BoardArray[TargetBoardIndex];
			}
			return true;
		}

		default:
			return false;
	}
}

bool AShogiBoardManager::PickRandomLegalAction(EPlayerSide Side, FShogiLegalAction& OutAction) const
{
	TArray<FShogiLegalAction> Options;

	for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
	{
		const FShogiPieceData& Piece = BoardArray[Index];
		if (Piece.PlayerSide != Side)
		{
			continue;
		}

		UDataTable* MoveTable = GetMoveDataTableFor(Piece);
		const TArray<int32> Movable = UShogiRulesLibrary::GetMovableIndices(BoardArray, Piece, Index, MoveTable);
		for (int32 To : Movable)
		{
			FShogiLegalAction Option;
			Option.From = Index;
			Option.To = To;
			Options.Add(Option);
		}
	}

	const TArray<TObjectPtr<AShogiPiece>>& HandActors = (Side == EPlayerSide::Sente) ? HandPieceActors_Sente : HandPieceActors_Gote;
	for (AShogiPiece* HandActor : HandActors)
	{
		if (!HandActor)
		{
			continue;
		}
		for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
		{
			if (BoardArray[Index].PlayerSide != EPlayerSide::None)
			{
				continue;
			}
			if (HandActor->PieceData.PieceType == EPieceType::Fu && UShogiRulesLibrary::IsNifuViolation(BoardArray, Side, Index))
			{
				continue;
			}

			FShogiLegalAction Option;
			Option.bIsDrop = true;
			Option.To = Index;
			Option.DropActor = HandActor;
			Option.DropPieceType = HandActor->PieceData.PieceType;
			Options.Add(Option);
		}
	}

	if (Options.Num() == 0)
	{
		return false;
	}

	OutAction = Options[FMath::RandRange(0, Options.Num() - 1)];
	return true;
}

bool AShogiBoardManager::IsKingInCheck(EPlayerSide Side) const
{
	if (Side == EPlayerSide::None)
	{
		return false;
	}

	int32 KingIndex = INDEX_NONE;
	for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
	{
		if (BoardArray[Index].PlayerSide == Side && BoardArray[Index].PieceType == EPieceType::Ou)
		{
			KingIndex = Index;
			break;
		}
	}
	if (KingIndex == INDEX_NONE)
	{
		return false;
	}

	const EPlayerSide EnemySide = (Side == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;
	for (int32 Index = 0; Index < BoardArray.Num(); ++Index)
	{
		const FShogiPieceData& Piece = BoardArray[Index];
		if (Piece.PlayerSide != EnemySide)
		{
			continue;
		}

		UDataTable* MoveTable = GetMoveDataTableFor(Piece);
		if (UShogiRulesLibrary::GetMovableIndices(BoardArray, Piece, Index, MoveTable).Contains(KingIndex))
		{
			return true;
		}
	}

	return false;
}

void AShogiBoardManager::ApplyMove(int32 From, int32 To, EPlayerSide RequestingSide, bool bClientRequestedPromote)
{
	if (!HasAuthority())
	{
		return;
	}

	if (AShogiGameState* GS = GetShogiGameState())
	{
		if (GS->bGameOver || GS->CurrentTurn != RequestingSide)
		{
			return;
		}
		if (!ResolveCardPhaseIfUncontrolled(GS, RequestingSide))
		{
			// bCardPhaseResolved gates the move/drop phase behind the card phase (see
			// docs/2026-08-14-card-system-phase-a.md): a play-a-card-or-pass action must
			// resolve first each turn.
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
	// Validate against the same GetMovableIndices the client's move markers and IsKingInCheck
	// use (rather than calling CheckCanMove/IsPathClear separately), so that per-piece card
	// buffs affecting move range (e.g. Fu Rocket's forward+2 jump, see
	// docs/2026-08-14-card-system-phase-a.md) are honored here too - CheckCanMove alone only
	// knows the DataTable-driven base move set and would reject a card-boosted destination.
	if (!UShogiRulesLibrary::GetMovableIndices(BoardArray, MovingPiece, From, MoveTable).Contains(To))
	{
		return;
	}

	const FShogiPieceData TargetCell = BoardArray[To];
	if (TargetCell.PlayerSide == MovingPiece.PlayerSide)
	{
		// Occupied by a friendly piece.
		return;
	}

	if (TargetCell.PlayerSide != EPlayerSide::None && TargetCell.bIsInvincible)
	{
		// Temporary Invincibility (see docs/2026-08-14-card-system-phase-b.md): this square's
		// occupant cannot be captured right now, so the move itself is illegal.
		return;
	}

	const bool bCapturedKing = (TargetCell.PieceType == EPieceType::Ou);

	// Capture.
	if (TargetCell.PlayerSide != EPlayerSide::None)
	{
		FShogiPieceData Captured = TargetCell;
		Captured.PlayerSide = MovingPiece.PlayerSide; // ownership flips to the capturing side
		Captured.bIsPromoted = false; // captured pieces always return to hand unpromoted
		// Rental Reservation (see docs/2026-08-14-card-system-phase-b.md 2.1): a pending or
		// active loan vanishes once the piece is captured, regardless of by whom.
		Captured.PendingLoanCasterSide = EPlayerSide::None;
		Captured.PendingLoanTurnsElapsed = 0;
		Captured.bLoanActive = false;
		Captured.LoanOriginalOwnerSide = EPlayerSide::None;

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
		GetWorldTimerManager().ClearTimer(CardPhaseTimerHandle);
		GetWorldTimerManager().ClearTimer(MovePhaseTimerHandle);
		ClearPhaseTimerEndTime();
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

	if (AShogiGameState* GS = GetShogiGameState())
	{
		if (GS->bGameOver || GS->CurrentTurn != RequestingSide)
		{
			return;
		}
		// See ApplyMove's matching check: card phase must resolve before a drop is legal too.
		if (!ResolveCardPhaseIfUncontrolled(GS, RequestingSide))
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

	if (DropPieceType == EPieceType::Fu && UShogiRulesLibrary::IsNifuViolation(BoardArray, RequestingSide, DropIndex))
	{
		// 二歩: can't drop a second unpromoted Fu onto a file that already has one.
		return;
	}

	TArray<FShogiPieceData>& CapturedPieces = (RequestingSide == EPlayerSide::Sente) ? CapturedPieces_Sente : CapturedPieces_Gote;
	TArray<TObjectPtr<AShogiPiece>>& HandActors = (RequestingSide == EPlayerSide::Sente) ? HandPieceActors_Sente : HandPieceActors_Gote;

	const int32 HandIndex = HandActors.IndexOfByKey(DropPieceActor);
	if (HandIndex == INDEX_NONE)
	{
		return;
	}

	// Start from the hand actor's existing data (not a fresh default) so permanent card buffs
	// like Fu Rocket's bHasFuRocketBoost (see docs/2026-08-14-card-system-phase-a.md) survive
	// being captured and later re-dropped, not just sitting in hand.
	FShogiPieceData DroppedPiece = DropPieceActor->PieceData;
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
