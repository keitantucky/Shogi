# カードシステム Phase A（山札・手札・4種カード効果）

`docs/GameConcept.md`（企画全体）・`docs/GameSpec.md`（将棋コアロジックの背景仕様）を前提とする。本ドキュメントはその上に乗せる最初のカード実装パスをまとめたもの。

## 1. 背景・問題

### 1.1 企画とのギャップ
`docs/GameConcept.md`は「将棋×カード」の企画・カード効果10選を定義しているが、`Source/Shogi/`のC++実装には山札・手札・カード使用フェーズに相当するコードが一切存在しない（`AShogiGameState::CurrentTurn`のOnRep駆動でターンが進むのみ）。

### 1.2 カード効果の複雑さのばらつき
10種のカードは効果の性質が大きく異なる（永続ステータス変更、即時効果、ターン数を跨ぐ遅延効果、盤面座標操作、範囲全滅、時限的な耐性など）。すべてを一度に実装すると仕様の詰めと実装検証が同時に走り破綻しやすい。

### 1.3 手札の秘匿性とネットワーク権威
既存アーキテクチャは、クライアント発行の操作は必ずオーナー付き`AShogiPlayerController`のServer RPCから`AShogiBoardManager`のプレーン関数を呼ぶ設計（`docs/GameSpec.md` §8.1）。手札は対戦相手から見えてはならない秘匿情報のため、共有アクターである`AShogiBoardManager`や`AShogiGameState`にそのまま置くと非公開化ができない。

### 1.4 ホットシート/CPU対戦との整合性
`AShogiPlayerController::bControlBothSides`（ホットシート、1コントローラーが両陣営を操作）と`AShogiSinglePlayerVsAIGameMode`（AI側はコントローラーを介さず`BoardManager`を直接呼ぶ、`docs/GameSpec.md` §10.1）という2つの特殊モードが既に存在する。カードのターン強制（「カード使用」→「駒の移動」の順序固定）を導入すると、これらのモードでカードフェーズが正しく解決されないと駒が一切動かせなくなる恐れがある。

### 1.5 山札30枚という前提と実装枚数の不一致
企画書の山札30枚は10種類のカードが揃っている前提の数字。Phase Aで4種類しか実装しない場合、残り6種分の穴をどう埋めるかを決めておく必要がある。

## 2. 仕様

### 2.1 実装範囲
今回実装する4種（`docs/GameConcept.md` §3 の No.1/2/4/10）:

| カード名 | 分類 | 効果 |
| --- | --- | --- |
| 歩兵ロケット | 永続 | 自分の未成り歩を選択し、前方+2マスへの移動を恒久追加（駒を飛び越えてジャンプ可） |
| 即時覚醒 | 即時 | 自分の未成り・成れる駒種（歩香桂銀角飛）を選択し、条件無視でその場で即座に成る |
| 天変地異 | 即時 | 盤上（駒台は対象外）の先手・後手すべての成れる駒種の成り状態を反転。対象選択不要 |
| ハイエナ | 即時 | 相手の駒台から1枚選び、自分の駒台へ移す。ステータス（永続バフ含む）はそのまま引き継ぐ |

残り6種（貸し出し予約・石化の呪い・位置シャッフル・メガトンインパクト・道連れボム・一時無敵）と15秒ターンタイマーはPhase B以降に実装する。

### 2.2 山札・手札
- 各プレイヤーがそれぞれ30枚の山札を持つ（中身は同一構成）。Phase Aは実装済み4種のみなので **歩兵ロケット8／ハイエナ8／即時覚醒7／天変地異7 = 30枚** に拡張した構成で埋める。Phase Bでカード種が増えたら比率を再調整する。
- 対局開始時に初期手札3枚を配布。以降、毎ターン開始時に1枚ドロー。手札が既に上限（3枚）ならドローしない（スキップ、捨て札は発生しない）。
- 自分の手札は自分にしか見えない（オンライン対戦で相手に非公開）。ホットシート/CPU対戦は1人プレイのため実質影響なし。

### 2.3 ターン内フローの強制
「カード使用」→「駒の移動」の順序を固定する。カードを使わない場合は明示的に「パス」を選択する必要があり、自動パスにはしない（15秒タイマーはPhase B以降のため、今回は必ずUI操作で意思表示させる）。カードフェーズが解決するまで、そのターンの駒移動・打ちのリクエストはサーバー側で拒否する。

### 2.4 対象なしカードの扱い
盤面上に有効な対象が1つもないカード（例: 歩が全部取られた/成っている）は手札上で選択不可（グレーアウト）にする。クライアントの表示判定とサーバーの受理判定は同じ判定関数を使い、「使えるが不発に終わる事故」は起こさない。

### 2.5 各カードの詳細仕様
- **歩兵ロケット**: 前+1マス目に駒があっても無視して前+2マスへジャンプできる（2マス先が敵駒なら捕獲、味方駒なら移動不可、空マスなら移動）。既存の前+1マスの通常移動はそのまま残る。捕獲されて相手の持ち駒になっても、このバフは駒のデータに紐づいたまま残る（`docs/GameConcept.md`の「カードで強化されたステータスもそのまま引き継がれる」ルール通り）。
- **即時覚醒**: 対象は「盤上・自分の駒・未成り・成れる駒種（歩香桂銀角飛）」のみ。王・金・既に成っている駒・持ち駒（駒台）は対象外。
- **天変地異**: 先手・後手両方の盤上の駒が対象。駒台（持ち駒）は対象外（「打った駒は常に不成り」という既存ルールと矛盾させないため）。反転後は王手判定（`IsKingInCheck`）を再計算し、表示を更新する。
- **ハイエナ**: 相手の駒台が空の場合は選択不可。

### 2.6 CPU対戦モードの扱い
`AShogiSinglePlayerVsAIGameMode`のAI側には、今回カード選択ロジックを実装しない。AI側のターンが来た時点でカードフェーズは自動的に解決済み扱いとし、既存の`MakeAIMove`による直接`ApplyMove`/`ApplyDrop`呼び出しをブロックしない。

## 3. 設計

### 3.1 Core型の追加
`Source/Shogi/Core/ShogiCardTypes.h`（新規）に以下を追加する。

```cpp
UENUM(BlueprintType)
enum class ECardType : uint8
{
	None,
	FuRocket,               // 歩兵ロケット（Phase A実装）
	InstantAwakening,       // 即時覚醒（Phase A実装）
	TenpenChii,             // 天変地異（Phase A実装）
	Hyena,                  // ハイエナ（Phase A実装）
	RentalReservation,      // 貸し出し予約（Phase B）
	PetrifyCurse,           // 石化の呪い（Phase B）
	PositionSwap,           // 位置シャッフル（Phase B）
	MegatonImpact,          // メガトンインパクト（Phase B）
	SelfDestructBomb,       // 道連れボム（Phase B）
	TemporaryInvincibility  // 一時無敵（Phase B）
};

USTRUCT()
struct FShogiCardHandState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<ECardType> Hand;
};
```

Phase B用の6種は列挙値のみ用意し、`UShogiCardEffectLibrary`側は`default`ケースで未実装として扱う（山札にも含めない）。

`Source/Shogi/Core/ShogiTypes.h`の`FShogiPieceData`に永続バフ用フィールドを追加する。

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shogi")
bool bHasFuRocketBoost = false;
```

既存の`ApplyMove`の捕獲処理・`ApplyDrop`は`FShogiPieceData`を構造体ごとコピー/代入するため、追加フィールドも自動的に持ち駒化・打ち直しを経て引き継がれる（`bIsPromoted`のように明示的にリセットする処理は入れない）。

### 3.2 `UShogiCardEffectLibrary`（新規）
`Source/Shogi/Core/ShogiCardEffectLibrary.h/.cpp`。`UShogiRulesLibrary`/`UShogiPromotionLibrary`と同じ`UBlueprintFunctionLibrary`パターンのステートレスな判定・適用ヘルパー群。クライアント（グレーアウト判定）とサーバー（実際の適用前再検証）が同じ関数を使うことで判定基準を一致させる。

- `HasValidTarget(ECardType, BoardArray, OpponentCapturedPieces, Side) -> bool`: カードが今使用可能か（対象が1つでもあるか）
- `IsValidFuRocketTarget(BoardArray, Side, Index) -> bool` / `IsValidInstantAwakeningTarget(BoardArray, Side, Index) -> bool`: 個別マスの対象判定
- `ApplyFuRocket(BoardArray&, Index)`: `bHasFuRocketBoost = true`
- `ApplyInstantAwakening(BoardArray&, Index)`: `bIsPromoted = true`
- `ApplyTenpenChii(BoardArray&)`: 全マスを走査し、`UShogiPromotionLibrary::GetPromotionRuleForPieceType`で成れると判定された駒だけ`bIsPromoted`を反転
- `GetCardDisplayName(ECardType) -> FText` / `GetCardDescription(ECardType) -> FText`: UI表示用テキスト（`docs/GameConcept.md`のカード名・効果内容を転記）

ハイエナは`AShogiPiece`アクターの駒台間移動を伴うため、このライブラリではなく`AShogiBoardManager`側に直接実装する。

### 3.3 `AShogiBoardManager`
新規関数`ApplyCardEffect(ECardType CardType, EPlayerSide RequestingSide, int32 TargetBoardIndex, AShogiPiece* TargetHandPieceActor) -> bool`を追加する。

- `ApplyMove`/`ApplyDrop`と同じガード（`HasAuthority`・`bGameOver`・`GameState->CurrentTurn == RequestingSide`）に加え、`GameState->bCardPhaseResolved`が既にtrueなら拒否（二重使用防止）。
- `UShogiCardEffectLibrary`の判定関数で再検証してから適用する。`FuRocket`/`InstantAwakening`は`BoardArray[TargetIndex]`を書き換えた後、`PieceActorArray[TargetIndex]->PieceData`を同期し`UpdateAppearance()`を呼ぶ。
- `TenpenChii`は全マスに適用後、`PieceActorArray`を`BoardArray`に同期して`RefreshBoardVisual()`を呼び、`GameState->bInCheck = IsKingInCheck(GameState->CurrentTurn)`を再計算する。
- `Hyena`は相手側の`HandPieceActors_*`/`CapturedPieces_*`から`TargetHandPieceActor`を検索し、見つかれば両配列から削除して自分側の配列へ追加する（駒データの`PlayerSide`を要求側に書き換える。`ApplyMove`の捕獲時と同じ考え方）。`UpdateStandLayout()`を呼ぶ。
- 成功時のみ`true`を返す。呼び出し元（`AShogiPlayerController`）はこれを見てから手札を消費する。

`AdvanceTurn()`を拡張する。`CurrentTurn`反転後、`GameState->bCardPhaseResolved = false`とし、`AShogiGameMode::GetControllerForSide(CurrentTurn)`でその側を担当するコントローラーを探す。見つかれば`Controller->DrawCardForSide(CurrentTurn)`を呼んで自動ドローする。見つからない場合（CPU対戦のAI側）は即座に`bCardPhaseResolved = true`にし、移動をブロックしないようにする。

### 3.4 `AShogiGameState`
`UPROPERTY(Replicated) bool bCardPhaseResolved = false;`を追加する。「今`CurrentTurn`の側がカードフェーズを終えたか」だけを表す共有情報で、手札の中身とは異なり非公開にする必要がないため`COND_OwnerOnly`は付けない。`GetLifetimeReplicatedProps`に登録する。

### 3.5 `AShogiPlayerController`
- 新規メンバ`CardState_Sente` / `CardState_Gote`（`FShogiCardHandState`、`UPROPERTY(ReplicatedUsing = OnRep_CardState_Sente 等)`、`DOREPLIFETIME_CONDITION(..., COND_OwnerOnly)`）。ホットシート（`bControlBothSides`）では1つのコントローラーが両方を保持して使う。通常オンライン対戦では自分の`PlayerSide`側だけが使われ、もう一方は空のまま（レプリケートされても実害なし）。
- 非レプリケート・サーバー専用の`TArray<ECardType> Deck_Sente;` / `Deck_Gote;`。
- `BeginPlay()`で`HasAuthority()`なら両サイド分の山札（8/8/7/7構成）をシャッフルして構築し、初期手札3枚を配る。
- `void DrawCardForSide(EPlayerSide Side)`（`AdvanceTurn`から呼ばれるプレーン関数）: 手札が3枚未満ならデッキ先頭から1枚引く。デッキが空なら何もしない。
- Server RPC `Server_RequestPlayCard(ECardType CardType, int32 TargetBoardIndex, AShogiPiece* TargetHandPieceActor)`: `GetControllableSide()`を要求側とし、手札に該当カードがあるか確認した上で`BoardManager->ApplyCardEffect(...)`を呼ぶ。成功したら手札から1枚除去し`GameState->bCardPhaseResolved = true`にする。
- Server RPC `Server_PassCardPhase()`: 自分のターンかつ未解決なら`bCardPhaseResolved = true`にする。
- `BlueprintPure`な`GetMyHand()` / `IsCardPlayable(ECardType)`をUIバインド用に公開する。
- `HandleLeftClick`のクリック状態機械を拡張する。新規メンバ`SelectedCardType`。UIの手札ボタンから`SelectCardToPlay(ECardType)`が呼ばれるとセットされ、対象不要な`TenpenChii`は即座に`Server_RequestPlayCard`を送る。対象が必要なカードは次のクリックをターゲット選択として解釈する。`Hyena`は相手の駒台上の捕獲駒クリックをターゲットとして扱う（既存の捕獲駒クリック判定を流用）。

### 3.6 `AShogiGameMode`
新規`AShogiPlayerController* GetControllerForSide(EPlayerSide Side) const`を追加する。

```cpp
if (SentePlayer && (SentePlayer->bControlBothSides || SentePlayer->PlayerSide == Side)) return SentePlayer;
if (GotePlayer && GotePlayer->PlayerSide == Side) return GotePlayer;
return nullptr;
```

`PlayerSide`の実値で判定するため、`AShogiSinglePlayerVsAIGameMode`が`LocalHumanSide`を後から上書きするケース（変数名は`SentePlayer`でも実際は`Gote`担当、といったケース）でも正しく解決できる。既存の`SentePlayer`/`GotePlayer`フィールドをそのまま流用する。

### 3.7 移動可能マスへの永続バフ反映
`UShogiRulesLibrary::GetMovableIndices`を拡張する。通常のDataTableベース計算の後、`SelectedPiece.PieceType == EPieceType::Fu && SelectedPiece.bHasFuRocketBoost`なら、`SelectedPiece.PlayerSide`に応じた前方2マス先（Sente: dY=-2、Gote: dY=+2、dX=0）を計算し、盤内かつ味方駒がなければ結果に追加する（`IsPathClear`は呼ばない＝飛び越え許可）。関数シグネチャの変更は不要。

### 3.8 UI（バックエンド優先、最小限）
新規`Source/Shogi/UI/ShogiCardHandWidgetBase.h/.cpp`（`UShogiTurnWidgetBase`と同じ思想の薄いC++基底クラス）。手札表示・使用可否・カード使用・パスの各操作をC++関数として公開し、実際の見た目（`WBP_CardHand`）はエディタでの手動作成を前提とする。カード用メッシュ/画像アセットが存在しないため、テキストのみのボタンで動作確認する。

## 4. 既知の未対応事項

- 貸し出し予約・石化の呪い・位置シャッフル・メガトンインパクト・道連れボム・一時無敵の6種カードは未実装（Phase B）。
- 15秒のターンタイマー・時間切れ時のランダム強制発動は未実装（Phase B）。
- `AShogiSinglePlayerVsAIGameMode`のAIはカードを一切使用しない（カードフェーズは自動解決のみ）。
- カード用の見た目（メッシュ/画像/`WBP_CardHand`のレイアウト）は未作成。エディタでの手動作成が必要（`docs/GameSpec.md` §11の移行手順と同様の立て付け）。
- カードの残り枚数・捨て札の可視化、山札切れ時の挙動（今回は「ドローしない」で無音スキップ）などの細かいUXはPhase B以降で検討する。
