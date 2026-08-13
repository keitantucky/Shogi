# Shogi Game Specification

## 1. 概要

- エンジン: Unreal Engine 5.6(`Shogi.uproject` の `EngineAssociation: "5.6"`)
- アーキテクチャ: 将棋のコアゲームロジック(盤・駒・移動判定・駒取り・持ち駒・手番・成り)は**C++**(`Source/Shogi/`)で実装されている。オンライン対戦基盤(EOSCore経由のセッション/ロビー/フレンドUI)・タイトル画面・カメラ配置・移動先マーカーの見た目は引き続き Blueprint(`Content/BP/`)のまま。
- `Source/Shogi/`は機能別サブフォルダに分割されている: `Core/`(列挙型・構造体・移動/成りルールなど、アクターを持たないロジック)、`Gameplay/`(`AShogiBoardManager`/`AShogiPiece`/`AShogiPlayerController`/`AShogiGameState`などのアクター)、`GameModes/`(`AShogiGameMode`とそのシングルプレイ派生)、`UI/`(UMGウィジェットのC++基底クラス)。各サブフォルダは`Shogi.Build.cs`の`PublicIncludePaths`に明示的に登録されており、`#include "Foo.h"`のようにフォルダ名なしで参照できる
- 有効プラグイン: `EOSCore`(オンライン対戦基盤)、`ModelingToolsEditorMode`、`BlueprintScreenshotTool`(エディタ用、ゲームプレイ無関係)。
- 本ドキュメントは会話ベースで段階的に策定した仕様をまとめたもの。最初にBlueprint実装の仕様として書かれ、その後コアロジックをC++へ全面移植した経緯を反映している。

## 2. 盤・初期配置

- 9x9の盤。`AShogiBoardManager`(`Source/Shogi/Gameplay/ShogiBoardManager.h/.cpp`)が `BoardArray`(`TArray<FShogiPieceData>`、81要素)として盤面状態を保持する。
- **盤面インデックス規約**: `BoardIndex = Y*9 + X`(X=筋/file 0-8, Y=段/rank 0-8)。この規約は既存Blueprintのバイナリ解析(`BFL_ShogiRules`の`CSV_Row`/`CSV_Col`計算ロジック、初期配置DataTableの`Y0`~`Y8`行構造)から高い確度で復元した
- **陣地の方向(確定済み)**: 初期配置DataTableの実データにより、**先手(Sente)の陣地はGridY=8側、後手(Gote)の陣地はGridY=0側**と判明している(先手玉はY8、後手玉はY0に配置)
- 初期配置は `AShogiBoardManager::InitialBoardSetupTable`(RowStruct=`FShogiBoardSetupRow`、列`X0`~`X8`)からロードされる。セル値は`"{PlayerSide}_{PieceType}"`形式の文字列(例: `"Sente_Fu"`)または`"None"`
- 盤上のワールド座標は `AShogiBoardManager::GetWorldLocationForIndex()` が `BoardCellSizeX`/`BoardCellSizeY`(筋方向・段方向を別々に設定可能、エディタで既存盤メッシュに合わせて調整が必要)を使い、中央マス(4,4)を基準に算出する

## 3. 駒

- `EPieceType`(`Source/Shogi/Core/ShogiTypes.h`、`UENUM`): `Fu, Kyou, Kei, Gin, Kin, Kaku, Hisha, Ou, None`(旧Blueprint列挙型`E_PieceType`とインデックス0~8を完全一致させてある)
- `EPlayerSide`(同ファイル): `Sente, Gote, None`(旧`E_PlayerSide`とインデックス0~2を一致)
- `FShogiPieceData`(`USTRUCT`): `PieceType`, `PlayerSide`, `bIsPromoted`
- 駒アクター `AShogiPiece`(`Source/Shogi/Gameplay/ShogiPiece.h/.cpp`): `PieceData`(レプリケート)、`PieceMesh`(`UStaticMeshComponent`)、`bIsCaptured`。コンストラクタで以下の既存メッシュアセットを直接ロードする(原アセットの`Shilver`というtypoも含めそのまま):
  ```
  /Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPiecePawn          (Fu)
  /Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceLance         (Kyou)
  /Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceKnight        (Kei)
  /Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceShilverGeneral(Gin)
  /Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceGoldGeneral   (Kin)
  /Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceBishop        (Kaku)
  /Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceRook          (Hisha)
  /Game/Shogi_JapaneseChess/Meshes/Pieces/SM_ShogiPieceKing          (Ou)
  ```
  `UpdateAppearance()`が`PieceType`に応じてメッシュを切り替え、`PlayerSide==Gote`の場合は180度Yaw回転する(現物の将棋駒が相手側を向く慣習を再現)
- **成り駒用メッシュは既存アセットに存在しない**(`SM_ShogiPieceKing2`という未使用の2つ目の王メッシュはあるが、成り駒用ではない)。そのため現状、成っても見た目は変化しない(§7・§9参照)

## 4. 移動

- `UShogiRulesLibrary`(`Source/Shogi/Core/ShogiRulesLibrary.h/.cpp`)が旧`BFL_ShogiRules`を置き換える
- **移動範囲DataTableのエンコード**: 駒種ごとに19x19(オフセット-9~+9を0~18にマッピング)の到達可能マス表。行名`"Y{dY+9}"`、列`"X{dX+9}"`のセルに`〇`(合法)。**不可のセルは空文字ではなく`×`という文字列が入っている**(元のBP資産では既定値として保存省略されていたため当初「空=不可」と誤認していたが、DataTableのCSVエクスポート/再インポート後は明示的に`×`が入ることが実機ログで判明。合法判定は必ず`セル == "〇"`で厳密に行う必要がある。詳細は`Source/Shogi/Core/ShogiMoveMatrixRow.h`のコメント参照)。飛び駒(香・角・飛とその成り)は経路上の全マスが事前にマークされており、実際の遮蔽チェックは別途行う
  - `CheckCanMove()`: 上記ロジックで単純な到達可否を判定。**後手の駒は移動範囲テーブルが先手視点で1枚しかないため、判定前にdX・dYの符号を反転させて参照する**(先手の駒はそのまま)
  - `IsPathClear()`: 飛び駒の経路上に駒がないか確認(実座標間を直接走査するためテーブルの向きに依存しない)
  - `GetMovableIndices()`: 上記2つを組み合わせ、味方駒のマスを除外(敵駒のマスは捕獲可能として含む)し、全合法手を返す
- DataTable行構造体は`FShogiMoveMatrixRow`(`Source/Shogi/Core/ShogiMoveMatrixRow.h`、列`X0`~`X18`、既存`F_MoveMatrixRow`と列名完全一致)
- `AShogiBoardManager::GetMoveDataTableFor()`が駒種・成り状態に応じて適切なDataTable参照(`UnpromotedMoveTables`マップ、`PromotedBishopMoveTable`、`PromotedRookMoveTable`、`PromotedGenericMoveTable`=成金相当)を返す
- **デバッグ用**: `AShogiBoardManager::DebugLogAllMoveTables()`(Detailsパネルの`Shogi|Debug`カテゴリにボタンとして表示、PIE不要)で全移動範囲DataTableの`〇`セルを一括でOutput Logにダンプできる。移動範囲がおかしいと感じたときはまずこれで実データを確認するとよい
- 手番管理: `AShogiGameState::CurrentTurn`(レプリケート、`OnRep_CurrentTurn`で`OnTurnChanged`デリゲートを発火。旧BPは毎フレームポーリングだったのに対し、イベント駆動に改善)

## 5. 駒取り

- `AShogiBoardManager::ApplyMove()`内で、移動先に敵駒があれば捕獲: 所有者を捕獲側に変更・不成り化した上で`CapturedPieces_Sente`/`CapturedPieces_Gote`と`HandPieceActors_Sente`/`HandPieceActors_Gote`に追加し、該当アクターの`bIsCaptured`を`true`にする

## 6. 持ち駒・打つ

- 持ち駒は`HandPieceActors_Sente`/`HandPieceActors_Gote`(配列)と対応する`CapturedPieces_*`配列で管理し、`SenteKomadai`/`GoteKomadai`(`USceneComponent`、駒台の基準位置)周りに`UpdateStandLayout()`でレイアウトする
- `AShogiBoardManager::ApplyDrop()`が打つ処理を行う。**打った駒は常に不成り**(`bIsPromoted=false`固定)
- **二歩(実装済み)**: `UShogiRulesLibrary::IsNifuViolation(BoardArray, Side, DropIndex)`が、DropIndexの筋(列)にSideの不成り歩が既にあるかどうかを判定する(成り歩=と金はカウントしない)。`ApplyDrop()`が歩を打つ場合にサーバー側で必ずこれをチェックし、違反時はドロップを拒否する。`AShogiPlayerController::HandleLeftClick()`側でも、歩の持ち駒を選択した際の移動先マーカー候補から二歩になる筋を除外する(表示上の一致のため。合法性の最終判定は常にサーバー側)

## 7. 成り(昇格)

### 7.1 ルール(標準将棋ルール準拠)
- 成れる駒: 歩・香・桂・銀・角・飛。王・金は成らない(`UShogiPromotionLibrary::GetPromotionRuleForPieceType()`)
- 任意成り: 移動の移動元または移動先が敵陣(相手から見て最奥の3段)に含まれる場合、成るかどうかを選択できる
- 強制成り: 歩・香が最奥の1段、桂が最奥の2段に移動する場合は必ず成る
- 打った駒は常に不成り

### 7.2 C++実装
`Source/Shogi/Core/ShogiPromotionLibrary.h/.cpp`(`UShogiPromotionLibrary`)。`IsInPromotionZone`/`CanPromote`/`MustPromote`/`EvaluatePromotion`/`GetPromotionRuleForPieceType`を実装。陣地方向は`ShogiBoardConvention::bSenteHomeIsRowZero = false`(§2の確定事項に基づき修正済み)。

`AShogiBoardManager::ApplyMove()`がサーバー側で`EvaluatePromotion`を呼び、`ForcedPromotion`は強制、`NoPromotion`はクライアントの申告を無視、`OptionalPromotion`のみクライアントの選択(`bPromote`引数)を尊重する。

`AShogiPlayerController::TryMoveOrDropTo()`がクライアント側でも同じ判定を行い、`OptionalPromotion`の場合は`UShogiPromotionPromptWidgetBase`ベースの確認ウィジェットを表示する。

### 7.3 既知のギャップ
- 成り駒用メッシュ/マテリアルが存在しない(§3参照)
- `WBP_PromotionPrompt`ウィジェットのレイアウト自体は未作成(ロジック基底クラス`UShogiPromotionPromptWidgetBase`のみ用意、`YesButton`/`NoButton`という名前のボタンをバインドする必要がある)

## 8. ネットワーク/レプリケーション

- `AShogiBoardManager`: `BoardArray`・`PieceActorArray`をレプリケート
- `AShogiPiece`: `PieceData`・`bIsCaptured`をレプリケート
- `AShogiGameState`: `CurrentTurn`をレプリケート
- `AShogiPlayerController`: `PlayerSide`をレプリケート

### 8.1 設計変更(元のBlueprint実装からの重要な差分)
元のBlueprintでは`Server_RequestMovePiece`/`Server_RequestDropPiece`が`BP_BoardManager`上のカスタムイベント(=Server RPC)として定義されていた。しかし**Unrealの仕様上、Server RPCは呼び出すクライアントがそのアクターを所有(Owner Chain)している場合のみ正しくルーティングされる**。`BoardManager`はどちらのクライアントにも所有されない共有アクターのため、そのままではRPCが機能しない可能性が高い。

そのため本C++実装では、RPCのエントリポイントを常にクライアントに所有されている`AShogiPlayerController`に置き、そこから`BoardManager`のサーバー専用関数(`ApplyMove`/`ApplyDrop`、RPCではない通常関数)を呼び出す設計に変更した:
- `AShogiPlayerController::Server_RequestMovePiece(From, To, bPromote)` → `BoardManager->ApplyMove(From, To, PlayerSide, bPromote)`
- `AShogiPlayerController::Server_RequestDropPiece(DropIndex, DropPieceActor, DropPieceType)` → `BoardManager->ApplyDrop(...)`

また、`PlayerSide`はクライアントから送らせず、サーバー権威の`AShogiPlayerController::PlayerSide`(サーバーのみが書き込む)を直接使うようにしたため、元のBP実装より改善されている(クライアントが偽の`PlayerSide`を送ってなりすます余地がない)。

## 9. 勝敗判定

### 9.1 簡易勝敗判定(王取り、実装済み)
本格的な王手・詰み判定の代わりに、**相手の王を取った時点で終局**という最小限のルールを実装している(オンライン対戦・シングルプレイ共通):
- `AShogiGameState`: `bGameOver`(レプリケート)、`Winner`(レプリケート)、`OnGameOver`デリゲート
- `AShogiBoardManager::ApplyMove()`が捕獲した駒が`EPieceType::Ou`の場合、`bGameOver=true`・`Winner=`捕獲した側 を設定し、手番交代(`AdvanceTurn`)をスキップする
- `ApplyMove`/`ApplyDrop`は冒頭で`bGameOver`をチェックし、終局後は一切の操作を拒否する
- `AShogiPlayerController::HandleLeftClick()`も終局後はクリック自体を無視する
- `UShogiTurnWidgetBase::GetCurrentTurnText()`が終局時に`"Sente wins!"`/`"Gote wins!"`を表示する(既存のTextバインドをそのまま使い回せる)

### 9.1.1 王手表示(表示のみ、実装済み)
`AShogiBoardManager::IsKingInCheck(Side)`が、Sideの玉の位置に敵駒のいずれかが到達可能(`UShogiRulesLibrary::GetMovableIndices`)かどうかで王手を判定する。`AdvanceTurn()`が手番交代のたびに次の手番側についてこれを呼び、結果を`AShogiGameState::bInCheck`(レプリケート)に保存する。`UShogiTurnWidgetBase::GetCurrentTurnText()`が`bInCheck`のとき`"Sente - Check!"`/`"Gote - Check!"`を表示する。**判定のみで、王手放置や自殺手を防ぐ機能はない**(§9.2参照)。

### 9.2 スコープ外(今回は実装しない)

- **詰み(checkmate)判定・王手放置の禁止**(王手の表示自体は§9.1.1で実装したが、自分の王が王手されている状態のまま別の手を指すことや、相手の駒を取れば普通に取られてしまう王手放置は引き続き許容される。合法手をその都度王手放置にならないよう絞り込む処理は未実装)
- **禁じ手ルール**: 打ち歩詰め、行き所のない駒(打つ際の制限)。二歩は§6で実装済み
- オンライン対戦基盤(EOSCore, ロビー, フレンドUI, タイトル画面)・メニューUIはBlueprintのまま変更していない
- カメラ(`BP_SenteCamera`/`BP_GoteCamera`)・移動先マーカー(`BP_MoveTileMarker`)は見た目のみでロジックを持たないためBlueprintのまま。`AShogiBoardManager::MoveMarkerClass`・`AShogiPlayerController::GoteCameraClass`/`SenteCameraClass`から`TSubclassOf`で参照する
- 元の`BP_MyPlayerController`が`BeginPlay`で生成していた`WBP_Login`/`WBP_Main`(EOSCoreセッション/メニュー用ウィジェット)は、新しい`AShogiPlayerController`では生成しない(オンライン対戦基盤はスコープ外のため)。必要な場合はユーザー側で別途対応が必要

## 10. シングルプレイモード

オンライン対戦(`AShogiGameMode`、1人目→先手・2人目→後手をネットワーク経由で割り当て)とは別に、1人で遊べる2つのGameModeを追加した。どちらも`AShogiGameMode`を継承しているため、`GameStateClass`/`PlayerControllerClass`はコンストラクタで設定済みのものがそのまま引き継がれる。

### 10.1 CPU対戦: `AShogiSinglePlayerVsAIGameMode`(`Source/Shogi/GameModes/ShogiSinglePlayerVsAIGameMode.h/.cpp`)
- `LocalHumanSide`(既定`Sente`): 人間側の陣営。AI側は自動的にその逆
- `AIThinkDelaySeconds`(既定`0.5`): AIが着手するまでのディレイ(即着手だと不自然なため)
- `PostLogin`で唯一のローカルプレイヤーに`LocalHumanSide`を割り当て、`GameState->OnTurnChanged`にAIの着手処理をバインドする
- AIロジック(`MakeAIMove`): AI側の全駒の移動可能マス(`UShogiRulesLibrary::GetMovableIndices`)と、持ち駒がある場合は全空きマスへの打ち手を候補として集め、`FMath::RandRange`で**完全ランダムに1手選択**して`BoardManager->ApplyMove`/`ApplyDrop`を直接呼ぶ(探索・評価関数なし)。任意成りが発生する場合は常に成りを選ぶ。合法手が1つもない場合は何もしない(詰み相当の状態への対応は§9.2の通りスコープ外)

### 10.2 ローカル対面対戦: `AShogiSinglePlayerHotSeatGameMode`(`Source/Shogi/GameModes/ShogiSinglePlayerHotSeatGameMode.h/.cpp`)
- `PostLogin`で唯一のローカルプレイヤーの`AShogiPlayerController::bControlBothSides`を`true`に設定する
- `bControlBothSides=true`の場合、`AShogiPlayerController::GetControllableSide()`が`PlayerSide`ではなく`GameState->CurrentTurn`を返すようになり、1つのコントローラーで先手後手を交互に操作できる
- 手番が変わるたびに`GameState->OnTurnChanged`経由で`AShogiPlayerController::HandleTurnChanged()`が呼ばれ、`SenteCameraClass`/`GoteCameraClass`を使って現在の手番側のカメラに切り替える(選択状態もリセットされる)

### 10.3 起動方法

`Content/Map/Main.umap`のWorld Settingsは`BP_ShogiGameMode`(`AShogiGameMode`)のまま変更していない。ただし`AShogiGameMode::PostLogin`が`GetNetMode() == NM_Standalone`を検知した場合、唯一のローカルプレイヤーを自動的に`bControlBothSides = true`(ホットシート)にする。そのため:

- **`Main`を直接開く/Standaloneで起動する**(EOSCoreセッションを介さない) → 自動的にホットシートのシングルプレイになる。GameMode切り替えのBlueprint配線は不要
- **`Title`からEOSCoreセッションをホスト/参加して`Main`へ渡る** → `NM_ListenServer`/`NM_Client`になるため、通常のオンライン対戦(先着1人目=先手・2人目=後手)になる。こちらもGameMode切り替え不要(セッションのホスト/参加そのものはBlueprint側のEOSCore配線、`docs/GameSpec.md`スコープ外)

`AShogiSinglePlayerVsAIGameMode`(CPU対戦)は上記の自動判定に含まれないため、引き続き明示的に選択する必要がある場合はメニューBPから`OpenLevel`のOptions文字列でGameModeをオーバーライドすること:

§11の手順5と同じ理由(`AShogiPlayerController`のカメラ/ウィジェットクラス参照をClass Defaultsで設定する必要がある)で、これらのGameModeも**それぞれ薄いBlueprint子クラスを作り、`PlayerControllerClass`を`BP_ShogiPlayerController`に上書きしてから使うこと**:
- `BP_ShogiSinglePlayerVsAIGameMode`(親: `AShogiSinglePlayerVsAIGameMode`) — 併せて`LocalHumanSide`/`AIThinkDelaySeconds`もここで調整可能
- `BP_ShogiSinglePlayerHotSeatGameMode`(親: `AShogiSinglePlayerHotSeatGameMode`)

```
UGameplayStatics::OpenLevel(this, FName("Main"), true, TEXT("game=BP_ShogiSinglePlayerVsAIGameMode"))   // CPU対戦
UGameplayStatics::OpenLevel(this, FName("Main"), true, TEXT("game=BP_ShogiSinglePlayerHotSeatGameMode")) // ローカル対面対戦
```

Blueprintから使う場合は「Open Level (by Object Reference)」または「Open Level (by Name)」ノードの`Options`ピンに同様の文字列を指定すればよい(BPクラス名はプロジェクトで実際に付けた名前に合わせること)。§11の移行チェックリスト(DataTable・アクター配置・`BP_ShogiPlayerController`の作成等)は先に完了させておくこと。

## 11. 移行チェックリスト(エディタでの作業、ユーザー実施)

C++側の実装(`Source/Shogi/`)はビルド済みだが、以下はUnreal Editor上での手動作業が必要(ClaudeはBlueprintグラフ・DataTableのRowStruct切替・レベル上のアクター配置を直接操作できないため)。**上から順に行うことを推奨**(後の手順が前の手順の成果物に依存するため)。

1. **DataTableのRowStruct切替**(初期配置1個+移動範囲11個: 歩・香車・桂馬・銀・金・角行・飛車・角行成り・飛車成り・成金・王将):
   各DataTableを開き「Export as CSV」→ 新規DataTableをRowStruct `FShogiMoveMatrixRow`(または`FShogiBoardSetupRow`)で作成 → CSVを再インポート(列名`X0`~`X18`/`X0`~`X8`を完全一致させてあるため値はそのまま流用できるはず)
2. **`Content/Map/Main.umap`のアクター差し替え**: 配置済みの`BP_BoardManager`を削除し`AShogiBoardManager`を配置(`AShogiBoardManager`は普通のアクターなのでレベルに直接置き、Detailsパネルで設定すればよい)。`InitialBoardSetupTable`・`UnpromotedMoveTables`・`PromotedBishopMoveTable`・`PromotedRookMoveTable`・`PromotedGenericMoveTable`・`MoveMarkerClass`(既存`BP_MoveTileMarker`)・`PieceActorClass`(`AShogiPiece`)・`BoardCellSizeX`/`BoardCellSizeY`を設定
3. **`WBP_TurnUI`のReparent**: 親クラスを`UShogiTurnWidgetBase`に変更し、`Txt_TurnInfo`のTextプロパティのBindを`GetCurrentTurnText`に差し替え
4. **新規`WBP_PromotionPrompt`ウィジェット作成**: 親を`UShogiPromotionPromptWidgetBase`にし、`YesButton`/`NoButton`という名前でボタンを配置
5. **`BP_ShogiPlayerController`を新規作成**(親クラス`AShogiPlayerController`を選択してBlueprint Classを作成、グラフは空のまま): Class Defaultsで`SenteCameraClass`(既存`BP_SenteCamera`)・`GoteCameraClass`(既存`BP_GoteCamera`)・`TurnWidgetClass`(手順3の`WBP_TurnUI`)・`PromotionPromptWidgetClass`(手順4の`WBP_PromotionPrompt`)を設定する。
   - **なぜ必要か**: `AShogiPlayerController`はレベルに配置するアクターではなくGameModeが自動スポーンするため、これらの`EditAnywhere`プロパティを設定できる場所(Class Defaults)を持つBPクラスが別途必要になる。C++の生クラスのままではエディタ上に値を設定する場所がない
6. **`BP_ShogiGameMode`を新規作成**(親クラス`AShogiGameMode`): Class Defaultsで`PlayerControllerClass`を手順5の`BP_ShogiPlayerController`に上書きする。World SettingsのGameMode Overrideを`BP_MyGameMode`→この`BP_ShogiGameMode`に変更する
7. **座標系・向きの最終確認**: `BoardCellSizeX`/`BoardCellSizeY`とボード配置を実際の盤メッシュに合わせて調整(縦横比が違う盤メッシュでも別々の値でズレなく合わせられる)。`UShogiPromotionLibrary::GetForwardDistanceFromFarEdge`/`IsInPromotionZone`をデバッグ表示するなどして陣地方向(§2)を対局で最終確認する
8. 動作確認: 盤面表示・移動・駒取り・持ち駒打ち・成り・手番交代が実際に機能するかをEditor上で対局して確認

## 12. ファイル対応表(旧Blueprint → 新C++)

| 旧Blueprint | 新C++ |
|---|---|
| `BFL_ShogiRules` | `UShogiRulesLibrary`(`Core/ShogiRulesLibrary.h/.cpp`) |
| `BP_Piece` | `AShogiPiece`(`Gameplay/ShogiPiece.h/.cpp`) |
| `BP_BoardManager` | `AShogiBoardManager`(`Gameplay/ShogiBoardManager.h/.cpp`) |
| `BP_MyPlayerController` | `AShogiPlayerController`(`Gameplay/ShogiPlayerController.h/.cpp`) |
| `BP_ShogiGameState` | `AShogiGameState`(`Gameplay/ShogiGameState.h/.cpp`) |
| `BP_MyGameMode` | `AShogiGameMode`(`GameModes/ShogiGameMode.h/.cpp`)、シングルプレイ用に`AShogiSinglePlayerVsAIGameMode`/`AShogiSinglePlayerHotSeatGameMode`(`GameModes/`、§10)を追加 |
| `WBP_TurnUI`(ロジック部分) | `UShogiTurnWidgetBase`(`UI/ShogiTurnWidgetBase.h/.cpp`、レイアウトはReparentで再利用) |
| (新規) | `UShogiPromotionPromptWidgetBase`(`UI/ShogiPromotionPromptWidgetBase.h/.cpp`、レイアウトは新規作成が必要) |
| `E_PieceType` | `EPieceType`(`Core/ShogiTypes.h`) |
| `E_PlayerSide` | `EPlayerSide`(`Core/ShogiTypes.h`) |
| `F_PieceData` | `FShogiPieceData`(`Core/ShogiTypes.h`) |
| `F_MoveMatrixRow` | `FShogiMoveMatrixRow`(`Core/ShogiMoveMatrixRow.h`) |
| `F_BoardRowInput` | `FShogiBoardSetupRow`(`Core/ShogiBoardSetupRow.h`) |
| `BP_SenteCamera`/`BP_GoteCamera` | 変更なし(見た目のみ、Blueprintのまま) |
| `BP_MoveTileMarker` | 変更なし(見た目のみ、Blueprintのまま) |
| `ST_PieceData`/`DT_ShogiPieceBase`/`Content/Actors/`等 | 未使用の旧世代アセット、今回も未整理(技術的負債として残存) |

上表のC++側パスはすべて`Source/Shogi/`配下(例: `Core/ShogiRulesLibrary.h/.cpp` = `Source/Shogi/Core/ShogiRulesLibrary.h/.cpp`)。

## 13. 技術的負債(参考、今回は対応しない)

- `Content/Actors/`配下、`Content/DataTypes/temp/`に旧世代の未使用アセットが残存
- `ST_PieceData.MoveOffsets`/`DT_ShogiPieceBase`という未使用の並行データパスが残存
- `DefaultEngine.ini`にEOSの認証情報が平文でコミットされている
- 移行完了後、不要になった旧Blueprintアセット(`BP_BoardManager`, `BP_Piece`, `BFL_ShogiRules`, `BP_MyPlayerController`, `BP_ShogiGameState`, `BP_MyGameMode`)の削除は本ドキュメントでは扱わない(動作確認後にユーザー判断で削除を推奨)
