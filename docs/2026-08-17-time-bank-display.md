# 持ち時間の「60+15」形式表示

## 動機

`docs/2026-08-17-time-bank.md` で実装した持ち時間（60秒＋1ターン15秒の秒読み方式）は、内部データ（`AShogiGameState::TimeBankSeconds_Sente/_Gote`・`CurrentPhaseTimerEndTime`）としては存在していたが、それらを組み合わせて将棋の秒読み時計らしく「持ち時間+秒読み」形式（例: `60+15`）で見せるUIがまだ無かった。また、Sente/Gote両方の持ち時間を常時表示すると画面が煩雑になるため、表示するタイミングも制御したい。

## 仕様

- 各側の持ち時間を `<持ち時間>+<秒読み>` の形式（例: `60+15`）でテキスト表示する。
  - 手番側かつフリー期間（15秒）内: 秒読み側が `15`→`0` とカウントダウンし、持ち時間側は静止（減らない）。
  - 手番側かつフリー期間超過（持ち時間消費中）: 持ち時間側が実時間でカウントダウンし、秒読み側は `0` 固定。
  - 手番でない側: `<保持している持ち時間>+15`（静的表示）。
- 2つのTextBlock（Sente用・Gote用）のうち、**現在の手番側のTextBlockのみ表示**し、手番でない側は非表示にする。視聴しているプレイヤー（コントローラー）の所属や、ホットシート/CPU対戦/オンライン対戦といったモードには依存せず、常に「今指す側」だけを表示する。

## 設計

### テキスト表示

`UShogiTurnWidgetBase`（`Source/Shogi/UI/ShogiTurnWidgetBase.h/.cpp`、既存の手番表示・15秒カウントダウン表示を担うウィジェット基底クラス）に追加。

- `GetTimeBankDisplayText(EPlayerSide Side) const`: `AShogiGameState::bIsDrawingFromTimeBank`（後述）を見て、持ち時間消費中かどうかで表示する2つの数値の出どころを切り替え、`"%d+%d"` 形式の `FText` を組み立てる。
- `GetSenteTimeBankDisplayText()`/`GetGoteTimeBankDisplayText()`: UMGの "Bind" は引数なし関数しか候補に出せないため、`GetTimeBankDisplayText` を各側で固定引数呼び出しするラッパー（既存の `GetHandSlot0DisplayName()` 等と同じパターン）。

「手番でない側の秒読み表示」は仕様上15秒固定で表示する設計だが、コード側は `AShogiBoardManager::CardPhaseTimeoutSeconds`/`MovePhaseTimeoutSeconds`（15.f、`Source/Shogi/Gameplay/ShogiBoardManager.h` private定数）と値が重複している。既にこの2つも別々の定数として存在しており、共有化していない既存の許容範囲に倣った。

### 表示モードの判定（フリー期間 or 持ち時間消費中）

`AShogiGameState` に `bool bIsDrawingFromTimeBank`（レプリケート済み）を追加。`AShogiBoardManager` 側で管理する。

- `BeginBankDepletion`: 持ち時間が残っている場合に `true` にする（フリー期間を超えて持ち時間を消費し始めるタイミング）。
- `EndBankDepletion`: フェーズが正常に解決した際、持ち時間を確定的に差し引いた上で `false` に戻す。
- `HandleTimeUp`・`StartCardPhaseForCurrentTurn`: ゲーム終了時・新しいフェーズ開始時の防御的リセットとして `false` に戻す。

### Visibility（手番側のみ表示）

`GetTimeBankVisibility(EPlayerSide Side) const`（および `GetSenteTimeBankVisibility()`/`GetGoteTimeBankVisibility()` ラッパー）を追加。

当初は「視聴しているプレイヤー自身の側だけ表示し、相手は隠す」（`AShogiPlayerController::PlayerSide`/`bControlBothSides` を参照）という設計で実装したが、シングルPC・スタンドアロン起動（`AShogiGameMode::PostLogin` が `NM_Standalone` で自動的に `bControlBothSides = true` にする、ホットシート的な既定挙動）で試したところ、両方のTextBlockが常に表示されてしまい意図と異なった。ユーザーからのフィードバックで「両プレイヤーの表示は不要、`CurrentTurn` のプレイヤーの表示だけで良い」との方針が確定したため、視聴者やコントローラーの状態は一切参照せず、`AShogiGameState::CurrentTurn` とのみ比較する実装に変更した。

```cpp
ESlateVisibility UShogiTurnWidgetBase::GetTimeBankVisibility(EPlayerSide Side) const
{
    const AShogiGameState* GS = GetWorld() ? GetWorld()->GetGameState<AShogiGameState>() : nullptr;
    if (!GS || GS->bGameOver)
    {
        return ESlateVisibility::Hidden;
    }
    return (GS->CurrentTurn == Side) ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
}
```

この変更に伴い、当初追加した「ウィジェットの所有プレイヤーを `NativeConstruct` でキャッシュする」仕組み（`OwningShogiController` メンバ、`UShogiCardHandWidgetBase` と同じパターンで一度導入した）は不要になったため削除した。

### エディタ側の対応

`Content/BP/UMG/WBP_TurnUI` に、Sente用・Gote用の持ち時間TextBlockを2つ配置（同じ場所に重ねて配置してもよい。手番側以外は非表示になるため）。それぞれの `Text` を `Get Sente Time Bank Display Text`/`Get Gote Time Bank Display Text` に、`Visibility` を `Get Sente Time Bank Visibility`/`Get Gote Time Bank Visibility` にバインドした。
