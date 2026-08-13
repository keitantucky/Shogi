// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiPlayerController.h"
#include "ShogiBoardManager.h"
#include "ShogiGameState.h"
#include "ShogiPiece.h"
#include "ShogiRulesLibrary.h"
#include "ShogiPromotionLibrary.h"
#include "ShogiPromotionPromptWidgetBase.h"
#include "ShogiCardEffectLibrary.h"
#include "ShogiCardHandWidgetBase.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// All 10 implemented card types, used to draw uniformly at random from an effectively
	// infinite deck (see docs/2026-08-14-card-system-phase-b.md 2.2, revised: the hand is
	// fully redrawn - 3 fresh cards - at the start of every turn rather than incrementally
	// drawn from a finite, depleting deck).
	const ECardType AllCardTypes[] =
	{
		ECardType::FuRocket,
		ECardType::InstantAwakening,
		ECardType::TenpenChii,
		ECardType::Hyena,
		ECardType::RentalReservation,
		ECardType::PetrifyCurse,
		ECardType::PositionSwap,
		ECardType::MegatonImpact,
		ECardType::SelfDestructBomb,
		ECardType::TemporaryInvincibility,
	};
}

AShogiPlayerController::AShogiPlayerController()
{
	bReplicates = true;
}

void AShogiPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	GetOrFindBoardManager();

	if (AShogiGameState* GS = GetOrFindGameState())
	{
		GS->OnTurnChanged.AddDynamic(this, &AShogiPlayerController::HandleTurnChanged);
	}

	if (HasAuthority())
	{
		InitializeInitialHands();
	}

	if (IsLocalController() && TurnWidgetClass)
	{
		if (UUserWidget* TurnWidget = CreateWidget<UUserWidget>(this, TurnWidgetClass))
		{
			TurnWidget->AddToViewport();
		}
	}

	if (IsLocalController() && CardHandWidgetClass)
	{
		if (UShogiCardHandWidgetBase* CardWidget = CreateWidget<UShogiCardHandWidgetBase>(this, CardHandWidgetClass))
		{
			CardWidget->AddToViewport();
		}
	}
}

void AShogiPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AShogiPlayerController::HandleLeftClick);
	}
}

AShogiBoardManager* AShogiPlayerController::GetOrFindBoardManager()
{
	if (!BoardManager)
	{
		BoardManager = Cast<AShogiBoardManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AShogiBoardManager::StaticClass()));
	}
	return BoardManager;
}

AShogiGameState* AShogiPlayerController::GetOrFindGameState()
{
	if (!GameState)
	{
		GameState = GetWorld() ? GetWorld()->GetGameState<AShogiGameState>() : nullptr;
	}
	return GameState;
}

EPlayerSide AShogiPlayerController::GetControllableSide() const
{
	if (bControlBothSides)
	{
		return GameState ? GameState->CurrentTurn : PlayerSide;
	}
	return PlayerSide;
}

bool AShogiPlayerController::IsMyTurn() const
{
	return GameState && GameState->CurrentTurn == GetControllableSide();
}

bool AShogiPlayerController::TryComputeClickedBoardIndex(int32& OutIndex) const
{
	if (!BoardManager || BoardManager->BoardCellSizeX <= 0.f || BoardManager->BoardCellSizeY <= 0.f)
	{
		return false;
	}

	FVector WorldLocation, WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return false;
	}

	const FVector PlaneOrigin = BoardManager->GetActorLocation();
	const FVector PlaneNormal = BoardManager->GetActorUpVector();
	const FVector Intersection = FMath::LinePlaneIntersection(WorldLocation, WorldLocation + WorldDirection * 100000.f, PlaneOrigin, PlaneNormal);
	const FVector Local = Intersection - PlaneOrigin;

	const int32 GridX = FMath::RoundToInt(Local.X / BoardManager->BoardCellSizeX + 4.f);
	const int32 GridY = FMath::RoundToInt(Local.Y / BoardManager->BoardCellSizeY + 4.f);
	if (GridX < 0 || GridX > 8 || GridY < 0 || GridY > 8)
	{
		return false;
	}

	OutIndex = UShogiRulesLibrary::XYToIndex(GridX, GridY);
	return true;
}

void AShogiPlayerController::HandleLeftClick()
{
	if (!GetOrFindBoardManager())
	{
		return;
	}

	if (const AShogiGameState* GS = GetOrFindGameState())
	{
		if (GS->bGameOver)
		{
			return;
		}
	}

	FHitResult Hit;
	const bool bHitSomething = GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit);
	AShogiPiece* HitPiece = bHitSomething ? Cast<AShogiPiece>(Hit.GetActor()) : nullptr;

	if (SelectedCardType != ECardType::None)
	{
		// Card-target-selection mode (see SelectCardToPlay) takes priority over normal piece
		// selection/movement - the next click supplies this card's target instead.
		if (SelectedCardType == ECardType::Hyena)
		{
			const EPlayerSide OpponentSide = (GetControllableSide() == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;
			if (HitPiece && HitPiece->bIsCaptured && HitPiece->PieceData.PlayerSide == OpponentSide)
			{
				Server_RequestPlayCard(SelectedCardType, INDEX_NONE, HitPiece);
			}
		}
		else
		{
			int32 CardTargetIndex = INDEX_NONE;
			if (TryComputeClickedBoardIndex(CardTargetIndex))
			{
				Server_RequestPlayCard(SelectedCardType, CardTargetIndex, nullptr);
			}
		}
		SelectedCardType = ECardType::None;
		return;
	}

	if (HitPiece && HitPiece->bIsCaptured && HitPiece->PieceData.PlayerSide == GetControllableSide() && IsMyTurn())
	{
		ClearSelection();
		SelectedHandPieceActor = HitPiece;
		SelectedHandPieceType = HitPiece->PieceData.PieceType;

		CurrentMovableList.Reset();
		for (int32 Index = 0; Index < BoardManager->BoardArray.Num(); ++Index)
		{
			if (BoardManager->BoardArray[Index].PlayerSide != EPlayerSide::None)
			{
				continue;
			}
			if (SelectedHandPieceType == EPieceType::Fu
				&& UShogiRulesLibrary::IsNifuViolation(BoardManager->BoardArray, GetControllableSide(), Index))
			{
				// 二歩: don't offer this file as a drop target for a second unpromoted Fu.
				continue;
			}
			CurrentMovableList.Add(Index);
		}

		UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug] Hand piece selected: Type=%s Side=%s -> %d droppable squares"),
			*UEnum::GetValueAsString(SelectedHandPieceType),
			*UEnum::GetValueAsString(GetControllableSide()),
			CurrentMovableList.Num());
		for (int32 DropIdx : CurrentMovableList)
		{
			int32 DropX, DropY;
			UShogiRulesLibrary::IndexToXY(DropIdx, DropX, DropY);
			UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug]     -> Index=%d (X=%d, Y=%d)"), DropIdx, DropX, DropY);
		}

		BoardManager->ShowMoveMarkers(CurrentMovableList);
		return;
	}

	int32 Clicked = INDEX_NONE;
	if (!TryComputeClickedBoardIndex(Clicked))
	{
		return;
	}
	ClickedIndex = Clicked;

	if (SelectedHandPieceActor)
	{
		TryMoveOrDropTo(Clicked);
		return;
	}

	if (SelectedPieceIndex == INDEX_NONE)
	{
		TrySelectAt(Clicked);
	}
	else if (CurrentMovableList.Contains(Clicked))
	{
		TryMoveOrDropTo(Clicked);
	}
	else
	{
		ClearSelection();
		TrySelectAt(Clicked);
	}
}

void AShogiPlayerController::TrySelectAt(int32 Index)
{
	if (!BoardManager || !BoardManager->BoardArray.IsValidIndex(Index))
	{
		return;
	}

	const FShogiPieceData& Piece = BoardManager->BoardArray[Index];
	const EPlayerSide ControllableSide = GetControllableSide();
	if (Piece.PlayerSide != ControllableSide || ControllableSide == EPlayerSide::None || !IsMyTurn())
	{
		return;
	}

	SelectedPieceIndex = Index;
	SelectedPieceType = Piece.PieceType;

	UDataTable* MoveTable = BoardManager->GetMoveDataTableFor(Piece);
	CurrentMovableList = UShogiRulesLibrary::GetMovableIndices(BoardManager->BoardArray, Piece, Index, MoveTable);

	UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug] TrySelectAt: Index=%d PieceType=%s PlayerSide=%s Promoted=%d Table=%s -> %d movable squares"),
		Index,
		*UEnum::GetValueAsString(Piece.PieceType),
		*UEnum::GetValueAsString(Piece.PlayerSide),
		Piece.bIsPromoted,
		MoveTable ? *MoveTable->GetName() : TEXT("NULL"),
		CurrentMovableList.Num());
	for (int32 MoveIdx : CurrentMovableList)
	{
		int32 MoveX, MoveY;
		UShogiRulesLibrary::IndexToXY(MoveIdx, MoveX, MoveY);
		UE_LOG(LogTemp, Warning, TEXT("[ShogiDebug]     -> Index=%d (X=%d, Y=%d)"), MoveIdx, MoveX, MoveY);
	}

	BoardManager->ShowMoveMarkers(CurrentMovableList);
}

void AShogiPlayerController::TryMoveOrDropTo(int32 Index)
{
	if (!BoardManager)
	{
		return;
	}

	if (GameState && GameState->CurrentTurn == GetControllableSide() && !GameState->bCardPhaseResolved)
	{
		// The server would reject this move/drop anyway (see AShogiBoardManager::ApplyMove/
		// ApplyDrop's ResolveCardPhaseIfUncontrolled gate) - catch it client-side so the player
		// gets immediate feedback instead of a silent no-op, and don't bother sending the RPC.
		bBlockedByCardPhase = true;
		ClearSelection();
		return;
	}

	if (SelectedHandPieceActor)
	{
		Server_RequestDropPiece(Index, SelectedHandPieceActor, SelectedHandPieceType);
		ClearSelection();
		return;
	}

	if (SelectedPieceIndex == INDEX_NONE)
	{
		return;
	}

	int32 FromX, FromY, ToX, ToY;
	UShogiRulesLibrary::IndexToXY(SelectedPieceIndex, FromX, FromY);
	UShogiRulesLibrary::IndexToXY(Index, ToX, ToY);

	bool bIsPromotable = false;
	int32 ForcedDepth = 0;
	UShogiPromotionLibrary::GetPromotionRuleForPieceType(SelectedPieceType, bIsPromotable, ForcedDepth);

	const bool bIsSente = (GetControllableSide() == EPlayerSide::Sente);
	const bool bAlreadyPromoted = BoardManager->BoardArray.IsValidIndex(SelectedPieceIndex)
		? BoardManager->BoardArray[SelectedPieceIndex].bIsPromoted
		: false;

	const EShogiPromotionOutcome Outcome = UShogiPromotionLibrary::EvaluatePromotion(
		FromY, ToY, bIsSente, bIsPromotable, bAlreadyPromoted, /*bIsDrop=*/false, ForcedDepth);

	switch (Outcome)
	{
		case EShogiPromotionOutcome::ForcedPromotion:
			ConfirmMove(SelectedPieceIndex, Index, true);
			break;

		case EShogiPromotionOutcome::OptionalPromotion:
			PendingMoveFrom = SelectedPieceIndex;
			PendingMoveTo = Index;
			if (PromotionPromptWidgetClass)
			{
				if (UShogiPromotionPromptWidgetBase* Prompt = CreateWidget<UShogiPromotionPromptWidgetBase>(this, PromotionPromptWidgetClass))
				{
					Prompt->OnPromotionChoiceMade.AddDynamic(this, &AShogiPlayerController::OnPromotionChoiceMade);
					Prompt->AddToViewport();
					BoardManager->ClearMoveMarkers();
					break;
				}
			}
			// No prompt widget configured in the editor yet - default to not promoting.
			ConfirmMove(SelectedPieceIndex, Index, false);
			break;

		case EShogiPromotionOutcome::NoPromotion:
		default:
			ConfirmMove(SelectedPieceIndex, Index, false);
			break;
	}
}

void AShogiPlayerController::ConfirmMove(int32 From, int32 To, bool bPromote)
{
	Server_RequestMovePiece(From, To, bPromote);
	ClearSelection();
}

void AShogiPlayerController::OnPromotionChoiceMade(bool bPromote)
{
	if (PendingMoveFrom != INDEX_NONE && PendingMoveTo != INDEX_NONE)
	{
		ConfirmMove(PendingMoveFrom, PendingMoveTo, bPromote);
	}
	PendingMoveFrom = INDEX_NONE;
	PendingMoveTo = INDEX_NONE;
}

void AShogiPlayerController::ClearSelection()
{
	SelectedPieceIndex = INDEX_NONE;
	SelectedPieceType = EPieceType::None;
	SelectedHandPieceActor = nullptr;
	SelectedHandPieceType = EPieceType::None;
	CurrentMovableList.Reset();

	if (BoardManager)
	{
		BoardManager->ClearMoveMarkers();
	}
}

void AShogiPlayerController::Server_RequestMovePiece_Implementation(int32 From, int32 To, bool bPromote)
{
	if (AShogiBoardManager* BM = GetOrFindBoardManager())
	{
		BM->ApplyMove(From, To, GetControllableSide(), bPromote);
	}
}

void AShogiPlayerController::Server_RequestDropPiece_Implementation(int32 DropIndex, AShogiPiece* DropPieceActor, EPieceType DropPieceType)
{
	if (AShogiBoardManager* BM = GetOrFindBoardManager())
	{
		BM->ApplyDrop(DropIndex, DropPieceActor, DropPieceType, GetControllableSide());
	}
}

FShogiCardHandState& AShogiPlayerController::GetCardState(EPlayerSide Side)
{
	return (Side == EPlayerSide::Gote) ? CardState_Gote : CardState_Sente;
}

const FShogiCardHandState& AShogiPlayerController::GetCardState(EPlayerSide Side) const
{
	return (Side == EPlayerSide::Gote) ? CardState_Gote : CardState_Sente;
}

void AShogiPlayerController::InitializeInitialHands()
{
	// Setup-phase initial hand (see docs/GameConcept.md flow §1): both sides start with a
	// fresh 3-card hand, same as every subsequent turn's redraw.
	RedrawHandForSide(EPlayerSide::Sente);
	RedrawHandForSide(EPlayerSide::Gote);
}

void AShogiPlayerController::RedrawHandForSide(EPlayerSide Side)
{
	if (Side == EPlayerSide::None)
	{
		return;
	}

	FShogiCardHandState& State = GetCardState(Side);
	State.Hand.Reset();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		State.Hand.Add(AllCardTypes[FMath::RandRange(0, UE_ARRAY_COUNT(AllCardTypes) - 1)]);
	}
}

const TArray<ECardType>& AShogiPlayerController::GetMyHand() const
{
	return GetCardState(GetControllableSide()).Hand;
}

bool AShogiPlayerController::IsCardPlayable(ECardType CardType) const
{
	if (!GameState || GameState->bGameOver || GameState->CurrentTurn != GetControllableSide() || GameState->bCardPhaseResolved)
	{
		return false;
	}

	if (!GetCardState(GetControllableSide()).Hand.Contains(CardType))
	{
		return false;
	}

	if (!BoardManager)
	{
		return false;
	}

	const EPlayerSide OpponentSide = (GetControllableSide() == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;
	const TArray<FShogiPieceData>& OpponentCaptured =
		(OpponentSide == EPlayerSide::Sente) ? BoardManager->CapturedPieces_Sente : BoardManager->CapturedPieces_Gote;

	return UShogiCardEffectLibrary::HasValidTarget(CardType, BoardManager->BoardArray, OpponentCaptured, GetControllableSide());
}

void AShogiPlayerController::SelectCardToPlay(ECardType CardType)
{
	if (!IsCardPlayable(CardType))
	{
		return;
	}

	if (CardType == ECardType::TenpenChii)
	{
		// No target needed - send immediately rather than waiting for a click.
		Server_RequestPlayCard(CardType, INDEX_NONE, nullptr);
		return;
	}

	SelectedCardType = CardType;
}

void AShogiPlayerController::Server_RequestPlayCard_Implementation(ECardType CardType, int32 TargetBoardIndex, AShogiPiece* TargetHandPieceActor)
{
	AShogiBoardManager* BM = GetOrFindBoardManager();
	AShogiGameState* GS = GetOrFindGameState();
	if (!BM || !GS)
	{
		return;
	}

	const EPlayerSide Side = GetControllableSide();
	if (GS->bGameOver || GS->CurrentTurn != Side || GS->bCardPhaseResolved)
	{
		return;
	}

	FShogiCardHandState& State = GetCardState(Side);
	if (!State.Hand.Contains(CardType))
	{
		return;
	}

	if (BM->ApplyCardEffect(CardType, Side, TargetBoardIndex, TargetHandPieceActor))
	{
		State.Hand.RemoveSingle(CardType);
		GS->bCardPhaseResolved = true;
		BM->OnCardPhaseResolved(Side);
	}
}

void AShogiPlayerController::Server_PassCardPhase_Implementation()
{
	AShogiGameState* GS = GetOrFindGameState();
	AShogiBoardManager* BM = GetOrFindBoardManager();
	if (!GS || GS->bGameOver || GS->CurrentTurn != GetControllableSide() || GS->bCardPhaseResolved)
	{
		// bCardPhaseResolved already true means the card phase is over (a card was played, a
		// pass already went through, or the timeout forced one of those) and the move/piece-
		// selection phase's own 15-second timer is running instead - without this check, a
		// stray extra Pass click here would call BM->OnCardPhaseResolved again and wrongly
		// restart that timer back to a fresh 15 seconds.
		return;
	}

	GS->bCardPhaseResolved = true;
	if (BM)
	{
		BM->OnCardPhaseResolved(GetControllableSide());
	}
}

void AShogiPlayerController::ForceRandomCardOrPass()
{
	AShogiBoardManager* BM = GetOrFindBoardManager();
	AShogiGameState* GS = GetOrFindGameState();
	if (!BM || !GS || GS->bGameOver)
	{
		return;
	}

	const EPlayerSide Side = GetControllableSide();
	if (GS->CurrentTurn != Side || GS->bCardPhaseResolved)
	{
		return;
	}

	FShogiCardHandState& State = GetCardState(Side);
	const EPlayerSide OpponentSide = (Side == EPlayerSide::Sente) ? EPlayerSide::Gote : EPlayerSide::Sente;
	const TArray<FShogiPieceData>& OpponentCaptured =
		(OpponentSide == EPlayerSide::Sente) ? BM->CapturedPieces_Sente : BM->CapturedPieces_Gote;

	TArray<ECardType> PlayableCards;
	for (ECardType CardType : State.Hand)
	{
		if (UShogiCardEffectLibrary::HasValidTarget(CardType, BM->BoardArray, OpponentCaptured, Side))
		{
			PlayableCards.Add(CardType);
		}
	}

	if (PlayableCards.Num() == 0)
	{
		// Nothing in hand is currently playable - forced pass.
		GS->bCardPhaseResolved = true;
		BM->OnCardPhaseResolved(Side);
		return;
	}

	const ECardType ChosenCard = PlayableCards[FMath::RandRange(0, PlayableCards.Num() - 1)];

	int32 TargetBoardIndex = INDEX_NONE;
	AShogiPiece* TargetHandPieceActor = nullptr;

	if (ChosenCard == ECardType::Hyena)
	{
		const TArray<TObjectPtr<AShogiPiece>>& OpponentHand =
			(OpponentSide == EPlayerSide::Sente) ? BM->HandPieceActors_Sente : BM->HandPieceActors_Gote;
		TArray<AShogiPiece*> Candidates;
		for (AShogiPiece* Actor : OpponentHand)
		{
			if (Actor)
			{
				Candidates.Add(Actor);
			}
		}
		if (Candidates.Num() > 0)
		{
			TargetHandPieceActor = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		}
	}
	else if (ChosenCard != ECardType::TenpenChii)
	{
		TArray<int32> Candidates;
		for (int32 Index = 0; Index < BM->BoardArray.Num(); ++Index)
		{
			bool bValid = false;
			switch (ChosenCard)
			{
				case ECardType::FuRocket:				bValid = UShogiCardEffectLibrary::IsValidFuRocketTarget(BM->BoardArray, Side, Index); break;
				case ECardType::InstantAwakening:		bValid = UShogiCardEffectLibrary::IsValidInstantAwakeningTarget(BM->BoardArray, Side, Index); break;
				case ECardType::PetrifyCurse:			bValid = UShogiCardEffectLibrary::IsValidPetrifyCurseTarget(BM->BoardArray, Side, Index); break;
				case ECardType::PositionSwap:			bValid = UShogiCardEffectLibrary::IsValidPositionSwapTarget(BM->BoardArray, Side, Index); break;
				case ECardType::MegatonImpact:			bValid = UShogiCardEffectLibrary::IsValidMegatonImpactTarget(BM->BoardArray, Index); break;
				case ECardType::SelfDestructBomb:		bValid = UShogiCardEffectLibrary::IsValidSelfDestructBombTarget(BM->BoardArray, Side, Index); break;
				case ECardType::TemporaryInvincibility:bValid = UShogiCardEffectLibrary::IsValidTemporaryInvincibilityTarget(BM->BoardArray, Side, Index); break;
				case ECardType::RentalReservation:		bValid = UShogiCardEffectLibrary::IsValidRentalReservationTarget(BM->BoardArray, Side, Index); break;
				default: break;
			}
			if (bValid)
			{
				Candidates.Add(Index);
			}
		}
		if (Candidates.Num() > 0)
		{
			TargetBoardIndex = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		}
	}

	if (BM->ApplyCardEffect(ChosenCard, Side, TargetBoardIndex, TargetHandPieceActor))
	{
		State.Hand.RemoveSingle(ChosenCard);
	}

	// Resolve the card phase either way - HasValidTarget already confirmed ChosenCard should
	// have succeeded, but fail safe so the turn is never stuck if it somehow didn't.
	GS->bCardPhaseResolved = true;
	BM->OnCardPhaseResolved(Side);
}

void AShogiPlayerController::OnRep_CardState_Sente()
{
	// No extra client-side bookkeeping needed - UShogiCardHandWidgetBase reads GetMyHand()
	// via its own periodic UMG text/visibility bindings, same as UShogiTurnWidgetBase does
	// for turn text.
}

void AShogiPlayerController::OnRep_CardState_Gote()
{
}

void AShogiPlayerController::SwitchCameraForSide(EPlayerSide Side)
{
	TSubclassOf<AActor> CameraClass = (Side == EPlayerSide::Gote) ? GoteCameraClass : SenteCameraClass;
	if (!CameraClass)
	{
		return;
	}
	if (AActor* Camera = UGameplayStatics::GetActorOfClass(GetWorld(), CameraClass))
	{
		SetViewTargetWithBlend(Camera, 1.0f);
	}
}

void AShogiPlayerController::OnRep_PlayerSide()
{
	if (!bControlBothSides)
	{
		SwitchCameraForSide(PlayerSide);
	}
}

void AShogiPlayerController::HandleTurnChanged()
{
	// A new turn means a fresh, not-yet-attempted card phase - don't carry a stale warning
	// from the previous turn's blocked move into this one.
	bBlockedByCardPhase = false;

	if (bControlBothSides)
	{
		ClearSelection();
		if (AShogiGameState* GS = GetOrFindGameState())
		{
			SwitchCameraForSide(GS->CurrentTurn);
		}
	}
}

FText AShogiPlayerController::GetCardPhaseWarningText() const
{
	if (bBlockedByCardPhase && GameState && !GameState->bCardPhaseResolved)
	{
		return NSLOCTEXT("Shogi", "CardPhaseWarningText", "カードフェーズ未解決です");
	}
	return FText::GetEmpty();
}

void AShogiPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShogiPlayerController, PlayerSide);

	DOREPLIFETIME_CONDITION(AShogiPlayerController, CardState_Sente, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AShogiPlayerController, CardState_Gote, COND_OwnerOnly);
}
