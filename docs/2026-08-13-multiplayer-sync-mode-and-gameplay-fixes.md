# サーバー・クライアント盤面同期とシングル/マルチプレイ切り替え、王手表示

本ドキュメントは、2026-08-13の会話で行った将棋プロジェクト(`Source/Shogi/`)への一連の修正・追加を仕様書としてまとめたもの。個別のバグ修正から機能追加まで6件を扱う。全体の背景仕様は`docs/GameSpec.md`を参照。

## 1. 背景・問題

### 1.1 盤面の状況がサーバーとクライアントで共有されていない
`AShogiBoardManager`(盤面の権威データを持つActor)で、持ち駒(駒台)関連の4配列`CapturedPieces_Sente`/`CapturedPieces_Gote`/`HandPieceActors_Sente`/`HandPieceActors_Gote`が`UPROPERTY(Replicated)`未指定・`DOREPLIFETIME`未登録だった。これらは駒を取る/持ち駒を打つ処理(`ApplyMove`/`ApplyDrop`、サーバー権威のみで実行)でサーバー側のみ更新されるため、盤面本体(`BoardArray`/`PieceActorArray`、こちらはレプリケート済み)は同期していても、持ち駒台の状態がクライアントに一切反映されない状態だった。シングルプレイ/PIEの単一プロセス実行では症状が出ないため見落とされやすい構造だった。

### 1.2 後手の駒台で駒が逆方向に積まれてはみ出る
`AShogiBoardManager::UpdateStandLayout()`が、持ち駒の並び位置を`Komadai->GetComponentLocation() + (Index%3 * BoardCellSizeX, Index/3 * BoardCellSizeY, 0)`というワールド空間の生の座標加算で計算していた。後手側の駒台(`GoteKomadai`)は先手と向き合うよう盤上で反転配置されているが、このオフセット計算は駒台自身の回転を無視していたため、後手駒台から見るとX/Yが逆方向に積まれ、盤の外へはみ出していた。

### 1.3 シングルプレイ/マルチプレイの起動方式が未整理
以下の要件があった:
- シングルプレイ: `Main`マップを直接起動、またはモード選択画面で選択した場合に発生させたい。1ウィンドウで先手後手を交互にクリックするホットシート形式。
- マルチプレイ: `Title`マップから直接起動、またはモード選択画面で選択した場合に発生させたい。EOSCore経由で適切に通信し、Standalone起動で運用する予定。

`Main.umap`のWorld Settingsは既にオンライン対戦用の`BP_ShogiGameMode`(`AShogiGameMode`の子)に設定済みで、`AShogiSinglePlayerHotSeatGameMode`/`AShogiSinglePlayerVsAIGameMode`という別クラスも存在するが、これらは`Main.umap`の既定GameModeではなく、`OpenLevel`のOptions文字列で明示的に切り替える設計だった(`docs/GameSpec.md` §10.3、旧版)。この配線はBlueprint側でユーザーが行う前提で、まだ着手されていなかった。

また、副次的な調査として、PIEの「New Editor Window (PIE)」モードで追加クライアントを起動しようとすると`Couldn't Launch PIE Client:`エラーが発生する事象を確認した。ログ(`Saved/Logs/Shogi.log`)には`LogEOSCoreSubsystem: Error: [UNetDriverEOSCore::InitBase] You are connecting to a regular IPV4 Addr while using EOS Sockets.`と出力されており、`Config/DefaultEngine.ini`の`bIsUsingP2PSockets=true`(EOS P2Pソケット使用)と、PIEが追加クライアントを`127.0.0.1:17777`という生IPv4アドレスで接続しようとする挙動が競合していたことが原因と判明した。ただし、実際のマルチプレイはPIEのループバック接続ではなくEOSCore経由のセッション接続(Standalone起動)で運用する方針であるため、`bIsUsingP2PSockets`は`true`のまま維持する(EOSCoreの本来の通信にはP2Pソケットが必要)という結論に至った。

### 1.4 成った駒の見た目が変化しない
`docs/GameSpec.md`に既知のギャップとして記載の通り、成り駒用メッシュ/マテリアルが存在せず、成っても駒の見た目が一切変化しなかった。現実の将棋駒は表裏に文字が彫られているため、駒を裏返す表現で代替したい。

### 1.5 王手の表示がない
現状は簡易勝敗判定(王を取った時点で終局)のみが実装されており、本格的な王手・詰み判定はスコープ外(`docs/GameSpec.md` §9.2)。ただし、王手がかかっていること自体を画面に表示したいという要望があった。

## 2. 仕様

### 2.1 持ち駒台のレプリケーション
- `CapturedPieces_Sente`/`CapturedPieces_Gote`/`HandPieceActors_Sente`/`HandPieceActors_Gote`をレプリケート対象に追加する。
- `HandPieceActors_Sente`/`HandPieceActors_Gote`はアクター配列のため、レプリケート反映時にクライアント側でも駒台の再配置(`UpdateStandLayout()`)が呼ばれるようにする。

### 2.2 駒台のレイアウト方向
- 持ち駒の積み位置オフセットは、駒台(`SenteKomadai`/`GoteKomadai`)自身のローカル軸(向き)に従って計算する。ワールド空間の生のX/Y加算にしない。

### 2.3 シングルプレイ/マルチプレイの自動判定
- `Main.umap`のWorld Settings GameMode Override(`BP_ShogiGameMode`)は変更しない。
- 同一の`AShogiGameMode`が、ネットワーク接続状態(`NetMode`)に応じて挙動を自動的に切り替える:
  - `NM_Standalone`(`Main`を直接開く、EOSCoreセッションを介さない) → 唯一のローカルプレイヤーを自動的にホットシート(`bControlBothSides = true`)にする。
  - `NM_ListenServer`/`NM_Client`(`Title`からEOSCoreセッションをホスト/参加して`Main`へ遷移) → 通常のオンライン対戦(先着1人目=先手、2人目=後手)。
- CPU対戦(`AShogiSinglePlayerVsAIGameMode`)は自動判定の対象外とし、引き続き`OpenLevel`のOptions文字列での明示選択とする。
- EOSCoreのP2Pソケット設定(`bIsUsingP2PSockets=true`)は変更せず維持する。

### 2.4 成り駒の裏返り表現
- 駒が成った状態(`PieceData.bIsPromoted == true`)のとき、駒の見た目を軸回転で裏返す。
- 先手/後手の向き(Yaw 180度)とは独立した軸で裏返す必要がある。実装過程で回転軸をRoll(駒の向いている方向の軸)からPitch(左右方向の軸)に修正した。

### 2.5 王手表示
- 手番側の玉が敵駒の利きに入っている(王手されている)かどうかを判定できるようにする。
- 判定結果をサーバー・クライアント間で共有し、既存の手番表示UI上に反映する。
- 判定は表示目的のみとし、王手放置や自殺手の禁止(合法手の絞り込み)は行わない(詰み判定と同様、引き続きスコープ外)。

## 3. 設計

### 3.1 持ち駒台のレプリケーション
`Source/Shogi/Gameplay/ShogiBoardManager.h`:
```cpp
UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shogi")
TArray<FShogiPieceData> CapturedPieces_Sente;

UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shogi")
TArray<FShogiPieceData> CapturedPieces_Gote;

UPROPERTY(ReplicatedUsing = OnRep_HandPieceActors_Sente, BlueprintReadOnly, Category = "Shogi")
TArray<TObjectPtr<AShogiPiece>> HandPieceActors_Sente;

UPROPERTY(ReplicatedUsing = OnRep_HandPieceActors_Gote, BlueprintReadOnly, Category = "Shogi")
TArray<TObjectPtr<AShogiPiece>> HandPieceActors_Gote;
```
`ShogiBoardManager.cpp`の`GetLifetimeReplicatedProps`に4配列すべてを`DOREPLIFETIME`登録。`OnRep_HandPieceActors_Sente`/`OnRep_HandPieceActors_Gote`は`UpdateStandLayout()`を呼び、クライアント側でも駒台の再配置を反映する。

### 3.2 駒台のレイアウト方向
`ShogiBoardManager.cpp::UpdateStandLayout()`のオフセット計算を、駒台のワールド回転で変換してから加算する形に変更:
```cpp
const FVector LocalOffset((Index % 3) * BoardCellSizeX, (Index / 3) * BoardCellSizeY, 0.f);
const FVector WorldOffset = Komadai->GetComponentRotation().RotateVector(LocalOffset);
Piece->SetActorLocation(Komadai->GetComponentLocation() + WorldOffset);
```

### 3.3 シングルプレイ/マルチプレイの自動判定
`Source/Shogi/GameModes/ShogiGameMode.cpp::PostLogin`に、1人目のログイン(`JoinedPlayerCount == 0`、先手として割り当て)の直後にNetMode判定を追加:
```cpp
if (GetNetMode() == NM_Standalone)
{
    ShogiPC->bControlBothSides = true;
}
```
`AShogiPlayerController::bControlBothSides`(既存)がtrueの場合、`GetControllableSide()`が`PlayerSide`ではなく`GameState->CurrentTurn`を返すようになり、1つのコントローラーで先手後手を交互に操作できる(既存のホットシート機構をそのまま流用)。`Config/DefaultEngine.ini`の`bIsUsingP2PSockets`は`true`のまま変更なし。`docs/GameSpec.md` §10.3を新しい起動フローに合わせて更新した。

### 3.4 成り駒の裏返り表現
`Source/Shogi/Gameplay/ShogiPiece.cpp::UpdateAppearance()`:
```cpp
const float Yaw = (PieceData.PlayerSide == EPlayerSide::Gote) ? 180.f : 0.f;
const float Pitch = PieceData.bIsPromoted ? 180.f : 0.f;
SetActorRelativeRotation(FRotator(Pitch, Yaw, 0.f));
```
`FRotator(Pitch, Yaw, Roll)`はRoll→Pitch→Yawの順で合成されるため、Pitchによる裏返しが駒自身の向きに関わらず正しく適用される。

### 3.5 王手表示
`Source/Shogi/Gameplay/ShogiBoardManager.h/.cpp`に判定関数を追加:
```cpp
bool AShogiBoardManager::IsKingInCheck(EPlayerSide Side) const
{
    // Side の玉の BoardIndex を探索
    // 敵駒それぞれについて GetMovableIndices() が玉の BoardIndex を含むか確認
    // 含む敵駒が1つでもあれば true
}
```
`AdvanceTurn()`で手番交代のたびに次の手番側について呼び出し、結果を`AShogiGameState::bInCheck`(新規、レプリケート)に保存:
```cpp
GS->CurrentTurn = ...;
GS->bInCheck = IsKingInCheck(GS->CurrentTurn);
GS->OnRep_CurrentTurn();
```
`UShogiTurnWidgetBase::GetCurrentTurnText()`(既存、`Txt_TurnInfo`にBind済み)を拡張し、`bInCheck`のとき`"Sente - Check!"`/`"Gote - Check!"`を表示する。新規UIウィジェットの作成は不要。

## 4. 既知の未対応事項
- モード選択画面(シングルプレイ/マルチプレイを選ぶUI)自体がまだ存在しない。Blueprintでの新規作成が必要。
- `WBP_CreateSession`のセッションホスト時の`OpenLevel`遷移先が`/Game/Map/Main`になっているか未確認(EOSCoreプラグイン付属サンプルのデモマップのままになっている可能性がある)。
- 成り駒メッシュの裏面に実際に別の文字/テクスチャが存在するかは未確認。無地の場合は追加の見た目対応が必要。
- 王手放置・自殺手の禁止、詰み判定は引き続きスコープ外。
