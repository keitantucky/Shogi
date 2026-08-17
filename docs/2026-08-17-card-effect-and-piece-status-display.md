# カード効果発動表示 & 駒の状態表示

## 動機

カード効果が発動しても、発動した本人以外のクライアントには何が起きたか通知されず、盤面の変化からしか推測できなかった。また `FShogiPieceData`（`Source/Shogi/Core/ShogiTypes.h`）には `bHasFuRocketBoost`／`bIsPetrified`／`bIsInvincible`／レンタル予約関連フィールドがカードシステム実装（Phase A/B、`docs/2026-08-14-card-system-phase-a.md`・`docs/2026-08-14-card-system-phase-b.md`）で追加済みだったが、これらは移動可否のロジックにのみ影響し、見た目には一切反映されていなかった。

表示周りを強化するため、以下2点を実装する。

- カード効果発動時、画面上にカード名と効果を演出付きで表示する。
- 将棋の駒に、現在の状態（石化・無敵・歩兵ロケット強化・貸出中など）をアイコンで常時表示する。

## 仕様

### カード効果発動バナー

- カード効果が発動すると、画面右からカード名＋効果説明文が書かれたバナーがスライドインする。
- 一定時間表示された後、自動的にスライドアウトして消える。

### 駒の状態アイコン

- 各駒に、現在有効な状態をアイコンで表示する。
  - 対象: 石化（Petrify Curse）・無敵（Temporary Invincibility）・歩兵ロケット強化（Fu Rocket）・貸出中（Rental Reservation、発動後のみ）。
  - レンタル予約の「予約中（pending）」フェーズは、既存のゲームデザイン意図（発動するまで見えない「隠しトラップ」、`AShogiBoardManager::ApplyCardEffect` の RentalReservation ケース）を維持し、アイコン化しない。
- 表示位置は駒の右下あたりに小さく表示する。カメラの角度によって表示位置が駒から大きくズレないようにする（3Dワールド上の大きなオフセットによるパースの影響を避ける）。

## 設計

### カード効果発動バナー

- **通知経路**: `AShogiBoardManager::ApplyCardEffect`（サーバー専用）はこれまでクライアントへの通知手段を持たなかった。`AShogiGameState` に `FOnShogiCardEffectActivated`（`ECardType`, `EPlayerSide`, `int32 TargetBoardIndex`）delegate と `Multicast_NotifyCardEffectActivated` NetMulticast RPC を追加し、`ApplyCardEffect` が成功した際に呼び出す。GameState は全クライアントに関連性があるため（`OnTurnChanged`/`OnGameOver` と同じパターン）、ここに置いた。
- **表示ウィジェット**: 新規 `UShogiCardEffectBannerWidgetBase`（`Source/Shogi/UI/ShogiCardEffectBannerWidgetBase.h/.cpp`）。`AShogiPlayerController::BeginPlay` で一度だけ生成・`AddToViewport`（`TurnWidgetClass`/`CardHandWidgetClass` と同じパターン）し、`GS->OnCardEffectActivated` を購読する。
  - `ShowCardEffect(CardType, Side)`: カード情報を記録し、`OnCardEffectActivatedEvent()`（`BlueprintImplementableEvent`）を発火してBP側でスライドインの Widget Animation を再生させ、`AutoDismissSeconds`（既定3秒）後に `OnCardEffectDismissEvent()` を呼ぶタイマーを開始する。
  - `OnCardEffectDismissEvent()`（`BlueprintNativeEvent`）: デフォルト実装は即座に `HideCardEffect()` を呼ぶ。BP側でオーバーライドしてスライドアウトの Widget Animation を再生し、その "Finished" イベントから `HideCardEffect()` を呼ぶことで、演出と実際の非表示化を同期できる。
  - `GetCardNameText()`/`GetCardDescriptionText()` は `UShogiCardEffectLibrary::GetCardDisplayName`/`GetCardDescription` をラップし、既存の日本語カードテキストを再利用する。
- **エディタ側の対応**: `Content/BP/UMG/WBP_CardEffectBanner`（`UShogiCardEffectBannerWidgetBase` を継承）を新規作成し、`AShogiPlayerController`（`BP_ShogiPlayerController`）の `CardEffectBannerWidgetClass` に割り当てた。スライドイン/アウトの Widget Animation を作成し、`OnCardEffectActivatedEvent`/`OnCardEffectDismissEvent` に配線した。

### 駒の状態アイコン

- 新規 `UShogiPieceStatusWidgetBase`（`Source/Shogi/UI/ShogiPieceStatusWidgetBase.h/.cpp`）。`FShogiPieceData` の各状態フラグを読む `bool` 版ゲッター（`GetIsPromoted`/`GetHasFuRocketBoost`/`GetIsPetrified`/`GetIsInvincible`/`GetIsLoanActive`）と、UMGの `Visibility` プロパティに直接バインドできる `ESlateVisibility` 版ゲッター（`Get*Visibility()`、Visible/Collapsedを返す）を用意した。UMGの "Bind" は戻り値の型が完全一致する関数しか候補に出さないため、bool版だけでは `Visibility` に直接バインドできない。
- `AShogiPiece` に `StatusWidgetComponent`（`UWidgetComponent`, `EWidgetSpace::Screen`, `DrawSize=32x32`, `Pivot=(0,0)`）を追加。
  - Widget Blueprint（`WBP_PieceStatusIcon`）はまだ存在しない状態から実装を始めたため、`ConstructorHelpers::FClassFinder`（アセットが無いとコンストラクタで即座にクラッシュする）ではなく `TSoftClassPtr::LoadSynchronous()` でロードし、アセットが後から `/Game/BP/UMG/WBP_PieceStatusIcon` に作成された時点で自動的に有効になるようにした。
  - 表示位置は `AShogiPiece::UpdateAppearance()` 内で、`GetActorLocation()` に駒メッシュのバウンズ中心（`Mesh->GetBounds().Origin.Z`）を加えた**ワールド座標**に `SetWorldLocation` している。`StatusWidgetComponent` は `PieceMesh` の子としてアタッチされているため、`SetRelativeLocation` を使うと成り駒（Pitch180°）や後手（Yaw180°）の回転を継承して意図しない位置にズレる問題があった。ワールド座標指定に変えることでこれを回避した。
  - `Pivot=(0,0)` により、上記アンカー点がウィジェット矩形の左上に来る＝アイコンがアンカー点から右下方向に描画されるため、「駒の右下にちょこっと表示する」演出を実現している。
  - 当初 `RelativeLocation.Z = 80`（駒の上空へ固定オフセット）で配置していたが、駒スケールへの根拠のない決め打ちだったため見えない/カメラ角度でズレる問題が起きた。メッシュ実バウンズから動的に計算する現行方式に修正した。
- **エディタ側の対応**: `Content/BP/UMG/WBP_PieceStatusIcon`（`UShogiPieceStatusWidgetBase` を継承）を新規作成し、石化・無敵・歩兵ロケット強化・貸出中の各アイコン画像（`Content/Textures/T_Status_*`, `T_LoanActive_01`）を配置、各アイコンの `Visibility` を対応する `Get*Visibility()` にバインドした。
