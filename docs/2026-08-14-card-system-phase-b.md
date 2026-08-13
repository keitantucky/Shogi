# カードシステム Phase B（残り6種カード・15秒タイマー）

`docs/GameConcept.md`（企画全体）・`docs/2026-08-14-card-system-phase-a.md`（Phase A: カードフェーズ基盤＋歩兵ロケット/即時覚醒/天変地異/ハイエナ）を前提とする。本ドキュメントはPhase Aで「既知の未対応事項」としていた残り6種カードと15秒タイマーの設計をまとめたもの。

## 1. 背景・問題

### 1.1 残された6種カードの多様性
貸し出し予約（遅延効果）・石化の呪い（永続デバフ）・位置シャッフル（座標操作）・メガトンインパクト（座標操作＋衝突判定）・道連れボム（範囲全滅）・一時無敵（時限耐性）は、Phase Aの4種（永続バフ1種／即時系3種）よりも状態管理が複雑で、ターンをまたぐ状態（貸し出し予約・一時無敵）や、盤面上の複数マスへの同時作用（道連れボム）を新たに扱う必要がある。

### 1.2 15秒タイマー未実装
企画書の「テンポ爆速化」は、カードフェーズ・将棋フェーズの双方に制限時間を設けることを前提としているが、Phase Aではターン内フローの強制（カード→移動の順序）のみ実装し、実際の時間制限とタイムアウト時の強制発動・強制着手は未実装だった。

### 1.3 CPU対戦との整合性
`AShogiSinglePlayerVsAIGameMode`のAI側はカードフェーズ自動解決（Phase A実装）に加え、既存の`MakeAIMove`で着手そのものは行うため、新設する将棋フェーズタイマーがAI側でも二重に発火して事故を起こさないよう配慮が必要。

## 2. 仕様

### 2.1 貸し出し予約（時限）
- 対象: 相手の駒1枚（種別制限なし、王も含む）。既に予約中の駒・無敵状態の駒は対象外。
- タイミング（使用ターンを1として数える）:
  1. 使用ターン: 対象駒に「予約済み」状態を付与するのみ（何も起こらない）
  2. 相手の次のターン: カウント1（何も起こらない）
  3. 自分の次のターン開始時: カウント2に到達し、所有権が一時的に自分に移る（このターン中はその駒を自分の駒として使用可能）
  4. その次のターン（相手のターン）開始時: 所有権が自動的に元の所有者に戻り、予約状態は消滅
- 対象駒は実体（アクター）に紐づき、盤上を移動しても追跡し続ける。予約中または貸与中に駒が捕獲された場合、予約/貸与状態はその時点で消滅する（捕獲した側がそのまま通常通り持ち駒にする）。
- 貸与中の駒を貸与側がさらに動かした場合、位置はそのまま返却される(所有権のみ戻る)。

### 2.2 石化の呪い（永続）
- 対象: 相手の駒のみ（王も含む）。無敵状態の駒は対象外。
- 効果: 対象駒の移動可能マスを常に空にする（歩兵ロケットのブースト等、他の移動許可より優先して無効化）。
- 永続効果のため、捕獲されて駒台に移っても、再度打たれても効果は継続する（歩兵ロケットのブーストと同じ永続化パターン）。

### 2.3 位置シャッフル（即時）
- 対象: 自分の「王」（固定）と、選択した自分の「歩」（盤上、成不成問わず）。無敵状態の歩は対象外。
- 効果: 王と選択した歩の盤上の位置を即座に入れ替える。

### 2.4 メガトンインパクト（即時）
- 対象: 自分・相手どちらの駒も選択可能（王も対象に含む）。無敵状態の駒は対象外。
- 効果: 対象駒をその「所有者自身の陣地方向」（後退方向、先手なら盤の先手陣地側、後手なら後手陣地側）へ最大3マスノックバックする。
  - 経路上（1〜3マス目）に別の駒がある場合、その手前で停止する（捕獲は発生しない、単に押し戻すのみ）。
  - 盤の端を超える場合は、盤内に収まる最大マス数だけ押す。
  - 1マス目で即座に障害物/盤端に当たる場合は何も起こらない(位置据え置き)。

### 2.5 道連れボム（即時）
- 対象: 自分の駒1枚（盤上、王も含む）。無敵状態の駒は対象外。
- 効果: 対象駒と、その周囲1マス（8方向、盤外は無視）にある駒を敵味方問わずすべて破壊する。無敵状態の駒は巻き添えから除外(破壊されない)。
- 破壊された駒は通常の「捕獲」とは異なり、どちらの駒台にも入らずそのまま消滅する。
- 巻き込まれた駒に王が含まれる場合、その王を持つ側がその時点で敗北する（通常の王手による終局と同じ扱い）。双方の王が同時に巻き込まれた場合は引き分け（`Winner = None`のまま`bGameOver = true`）とする。

### 2.6 一時無敵（時限）
- 対象: 自分の駒のみ、王を除く。既に無敵状態の駒への再使用は許容する(何も変化しない)。
- 効果: 使用したターンから、自分の次のターンが来るまでの間（＝相手の1ターン分）、その駒は捕獲されなくなり、あらゆるカード効果の対象にも選べなくなる。自分の次のターン開始時に自動的に効果が切れる。

### 2.7 15秒タイマー
- カードフェーズ・将棋フェーズはそれぞれ独立して15秒の制限時間を持つ。
- カードフェーズ開始（ターン開始時のドロー直後）で15秒タイマー開始。時間切れの場合:
  - 使用可能なカードが1枚でもあれば、その中からランダムに1枚選び、対象が必要なカードは合法な対象からランダムに1つ選んで自動発動する
  - 使用可能なカードが1枚もなければ自動的にパスする
- カードフェーズが解決した時点で将棋フェーズの15秒タイマーを開始。時間切れの場合、合法な移動・打つ手からランダムに1つ選んで自動実行する（`AShogiSinglePlayerVsAIGameMode::MakeAIMove`と同じ完全ランダム選択ロジックを共通化して使う）。
- どちらのタイマーも、その側を実際に操作するコントローラーが存在しない場合（CPU対戦のAI側）は起動しない。AI側は既存の`MakeAIMove`（`AIThinkDelaySeconds`後に着手）がそのまま機能する。

## 3. 設計

### 3.1 `FShogiPieceData`（`ShogiTypes.h`）への追加フィールド
```cpp
// 石化の呪い: 永続。移動可能マスを常に空にする(GetMovableIndicesで最優先チェック)。
bool bIsPetrified = false;

// 一時無敵: 時限。true の間、捕獲対象・カード対象のいずれからも除外される。
bool bIsInvincible = false;

// 貸し出し予約: 時限。None = 予約なし。
EPlayerSide PendingLoanCasterSide = EPlayerSide::None;
int32 PendingLoanTurnsElapsed = 0;
bool bLoanActive = false;
EPlayerSide LoanOriginalOwnerSide = EPlayerSide::None;
```
いずれも`FShogiPieceData`が構造体ごとコピーされる既存の移動/捕獲/打つロジックにそのまま乗るため、`bHasFuRocketBoost`と同様に駒の実体に追従する。捕獲時（`ApplyMove`の捕獲処理）は`PendingLoan*`系フィールドを明示的にクリアする（石化・無敵は仕様上その捕獲経路に到達しない、または効果終了済みのため対象外）。

### 3.2 `ECardType`（`ShogiCardTypes.h`）
Phase B用としてプレースホルダだった6値を実装対象に格上げする（コメント更新のみ、値自体は変更なし）。

### 3.3 `UShogiCardEffectLibrary`の拡張
Phase Aと同じパターンで、6種それぞれに`IsValidXxxTarget`（個別マス判定）を追加し、`HasValidTarget`のswitch文に組み込む。無敵チェック（`!Piece.bIsInvincible`）を各判定に共通で追加する。

- `ApplyPetrifyCurse(BoardArray&, Index)`: `bIsPetrified = true`
- `ApplyPositionSwap(BoardArray&, KingIndex, FuIndex)`: 2マスの`FShogiPieceData`を入れ替え
- `ApplyMegatonImpact(BoardArray&, Index) -> int32`: 新しいインデックスを計算して駒データを移動、実際に移動した新インデックスを返す（呼び出し元がアクターの位置・`PieceActorArray`を同期するため）
- `ApplyTemporaryInvincibility(BoardArray&, Index)`: `bIsInvincible = true`
- 道連れボム・貸し出し予約は、駒アクターの破壊/駒台移動やターン跨ぎの状態管理を伴うため、Phase Aのハイエナと同様に`AShogiBoardManager`側に直接実装する。

`GetCardDisplayName`/`GetCardDescription`を10種フルに拡張する（`docs/GameConcept.md`の名称・効果文をそのまま使用）。

### 3.4 `UShogiRulesLibrary::GetMovableIndices`
関数の先頭で`SelectedPiece.bIsPetrified`をチェックし、trueなら即座に空配列を返す（歩兵ロケットのブースト処理より前に判定するため、石化が常に優先される）。

### 3.5 `AShogiBoardManager::ApplyCardEffect`の拡張
`ECardType`のswitch文に6ケースを追加。

- **PetrifyCurse/PositionSwap/MegatonImpact/TemporaryInvincibility**: Phase Aの`FuRocket`/`InstantAwakening`と同じパターン（ライブラリ関数を呼び、`BoardArray`を更新後、対応する`PieceActorArray`の`PieceData`を同期し`UpdateAppearance()`／`SetActorLocation()`）。
- **RentalReservation**: 対象マスの`FShogiPieceData`に`PendingLoanCasterSide = RequestingSide`, `PendingLoanTurnsElapsed = 0`をセットするのみ（このカード自体はその場では所有権を変更しない）。
- **SelfDestructBomb**: 対象マス＋周囲8マス（盤内かつ`PlayerSide != None`かつ`!bIsInvincible`）を走査し、該当する`PieceActorArray`のアクターを`Destroy()`、`BoardArray`の該当セルを`FShogiPieceData()`（空）にリセットする。巻き込まれた駒に王が含まれていた場合、`GameState->bGameOver = true`、`Winner`を「巻き込まれなかった側の王の持ち主」に設定（両陣営の王が同時に巻き込まれた場合は`Winner = EPlayerSide::None`のまま）。

`ApplyMove`の捕獲処理・ターゲットセル判定の先頭に、`TargetCell.bIsInvincible`が true の場合は捕獲そのものを拒否するチェックを追加する（無敵駒は「取られなくなる」の実現）。

### 3.6 `AShogiBoardManager::AdvanceTurn`の拡張
`CurrentTurn`を反転した直後、以下を追加で行う（カードドロー処理の前後どちらでもよいが、ドローの前に実施する）:

1. **一時無敵の解除**: 盤上の全駒を走査し、`PlayerSide == 新CurrentTurn && bIsInvincible`の駒について`bIsInvincible = false`にする（自分の次のターンが来た時点で失効）。
2. **貸し出し予約のティック**: 盤上の全駒を走査し、`PendingLoanCasterSide != None`の駒について:
   - `bLoanActive == true`（貸与中）なら、所有権を`LoanOriginalOwnerSide`に戻し、予約関連フィールドを全てリセットする（貸与ターンが終わり、次のターンが来たタイミングで返却）
   - そうでなければ`PendingLoanTurnsElapsed`をインクリメントし、`>= 2 && 新CurrentTurn == PendingLoanCasterSide`なら`LoanOriginalOwnerSide = 現在のPlayerSide`、`PlayerSide = PendingLoanCasterSide`、`bLoanActive = true`に切り替える（発動）
3. 上記の走査後、`PieceActorArray`の該当アクターの`PieceData`/`UpdateAppearance()`を同期する。

### 3.7 15秒タイマー
`AShogiBoardManager`に`FTimerHandle CardPhaseTimerHandle`と`FTimerHandle MovePhaseTimerHandle`を追加する。

- `AdvanceTurn`の末尾（ドロー・貸し出し予約ティック等がすべて終わった後）で、`AShogiGameMode::GetControllerForSide(新CurrentTurn)`が非nullの場合のみ`CardPhaseTimerHandle`を15秒でセットし`HandleCardPhaseTimeout()`を呼ぶ。コントローラーが存在しない場合（AI側）はタイマーを起動しない。
- カードフェーズが解決した瞬間（`Server_RequestPlayCard_Implementation`/`Server_PassCardPhase_Implementation`の成功時、および`ResolveCardPhaseIfUncontrolled`の自動解決時）に、`AShogiBoardManager::OnCardPhaseResolved(EPlayerSide Side)`を呼ぶ。この関数は`CardPhaseTimerHandle`をクリアし、同じくコントローラーが存在する場合のみ`MovePhaseTimerHandle`を15秒でセットし`HandleMovePhaseTimeout()`を呼ぶ。
- `ApplyMove`/`ApplyDrop`が成功した時点で`MovePhaseTimerHandle`をクリアする（`AdvanceTurn`が呼ばれ次のカードフェーズタイマーに切り替わるため）。
- `HandleCardPhaseTimeout()`: `AShogiGameMode::GetControllerForSide(CurrentTurn)`経由でコントローラーを取得し、`Controller->ForceRandomCardOrPass()`（新規、`AShogiPlayerController`側にPhase A同様プレーン関数として実装）を呼ぶ。
- `HandleMovePhaseTimeout()`: 3.8で共通化する合法手抽選ロジックを使い、ランダムな1手を`ApplyMove`/`ApplyDrop`に直接渡す(AIの`MakeAIMove`と同じ要領、ただしBoardManager内から)。
- `bGameOver`になった時点、および`BeginPlay`のリスタート等では両タイマーをクリアする。

### 3.8 合法手抽選ロジックの共通化
`AShogiSinglePlayerVsAIGameMode::MakeAIMove`内にある「全駒の移動可能マス＋持ち駒の打てるマスを集めてランダムに1つ選ぶ」ロジックを、`UShogiRulesLibrary`に静的関数として抽出する（例: `PickRandomLegalMoveOrDrop`、移動元/移動先または打つ駒/マスを返す小さな結果構造体を返す）。`AShogiSinglePlayerVsAIGameMode::MakeAIMove`と`AShogiBoardManager::HandleMovePhaseTimeout`の両方から呼ぶ。

### 3.9 `AShogiPlayerController`の拡張
- **（Phase B完了後にユーザー要望で変更）** 有限の山札（10種×3枚＝30枚）は廃止し、山札は実質無限にした。`RedrawHandForSide(EPlayerSide Side)`（旧`DrawCardForSide`）が、実装済み10種の固定配列からランダムに3枚選んで手札をまるごと入れ替える方式に変更。対局開始時の初期手札もこの関数を流用する（`InitializeInitialHands`）。詳細は`docs/2026-08-14-card-system-phase-a.md` §2.2/§3.5参照。
- 新規`void ForceRandomCardOrPass()`（プレーン関数、`AdvanceTurn`と同様にサーバー側からのみ呼ばれる）: 現在の手札から`UShogiCardEffectLibrary::HasValidTarget`が真になるカードを列挙し、あれば1枚ランダムに選択、対象が必要なら合法な対象からランダムに1つ選んで`BoardManager->ApplyCardEffect`を直接呼ぶ。手札から該当カードを消費し`GameState->bCardPhaseResolved = true`にする。使用可能なカードが1枚もなければ`GameState->bCardPhaseResolved = true`にするだけ（パス）。
- クライアント側のクリック導線はPhase Aの汎用「カード選択→次のクリックでターゲット決定」フローがそのまま流用できる（対象は盤上のマスをクリックするだけで、所有者を問わないため新規の分岐は不要）。

## 4. 既知の未対応事項

- 貸し出し予約・一時無敵とネットワーク遅延の組み合わせ（クライアント側の楽観的表示など）は考慮していない。サーバー権威の状態変化がレプリケートされるまでの間、UI表示が一瞬ずれる可能性がある。
- 道連れボムで王が同時に巻き込まれた場合の「引き分け」表示は`AShogiGameState::Winner = None`のままにするのみで、UI側（`UShogiTurnWidgetBase`）に専用の「引き分け」文言は追加しない（既存の`GetCurrentTurnText`が`Winner`未設定時にどう表示するかは既存実装依存）。
- 15秒タイマーの残り時間をUIに表示する仕組み（プログレスバー等）は今回のスコープ外。タイムアウト時の強制発動/着手のみ実装する。
- カード用の見た目（メッシュ/画像）は引き続き未作成。
