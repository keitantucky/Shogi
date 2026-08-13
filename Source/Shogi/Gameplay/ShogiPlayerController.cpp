// Fill out your copyright notice in the Description page of Project Settings.

#include "ShogiPlayerController.h"
#include "ShogiBoardManager.h"
#include "ShogiGameState.h"
#include "ShogiPiece.h"
#include "ShogiRulesLibrary.h"
#include "ShogiPromotionLibrary.h"
#include "ShogiPromotionPromptWidgetBase.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

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

	if (IsLocalController() && TurnWidgetClass)
	{
		if (UUserWidget* TurnWidget = CreateWidget<UUserWidget>(this, TurnWidgetClass))
		{
			TurnWidget->AddToViewport();
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

	if (HitPiece && HitPiece->bIsCaptured && HitPiece->PieceData.PlayerSide == GetControllableSide() && IsMyTurn())
	{
		ClearSelection();
		SelectedHandPieceActor = HitPiece;
		SelectedHandPieceType = HitPiece->PieceData.PieceType;

		CurrentMovableList.Reset();
		for (int32 Index = 0; Index < BoardManager->BoardArray.Num(); ++Index)
		{
			if (BoardManager->BoardArray[Index].PlayerSide == EPlayerSide::None)
			{
				CurrentMovableList.Add(Index);
			}
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
	if (bControlBothSides)
	{
		ClearSelection();
		if (AShogiGameState* GS = GetOrFindGameState())
		{
			SwitchCameraForSide(GS->CurrentTurn);
		}
	}
}

void AShogiPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShogiPlayerController, PlayerSide);
}
