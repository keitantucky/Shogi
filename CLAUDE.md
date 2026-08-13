# UE 自動ビルド運用ルール

このプロジェクトには C++ 変更をビルドし、成功時に Unreal Editor を起動するカスタムコマンド `/build-ue` が用意されている。

- `.claude/commands/build-ue.md` : `/build-ue` コマンド本体
- `.claude/scripts/build-and-launch-ue.ps1` : 実際にビルド・起動を行う PowerShell スクリプト
  - Unreal Editor がこの `.uproject` を開いた状態で起動中なら、ビルドは行わない
  - ビルドが成功した場合のみ Unreal Editor を起動する
  - ビルドに失敗した場合は `Saved/BuildLogs/auto_build.log` にログが残る

## 自動ビルドの実行ルール

自分（Claude）が `Source/` 配下の `.cpp`/`.h`/`.hpp`/`.c`/`.cc`/`.cxx`/`.inl`、または `.uproject`/`*.Build.cs`/`*.Target.cs` を編集したターンでは、応答の最後に `/build-ue` コマンドを実行すること（ユーザーによる手動編集や、C++ に無関係な変更では実行しない）。

`/build-ue` の実行結果は必ずユーザーに報告する。

- Unreal Editor が起動中で自動ビルドが行われなかった場合 → その旨を伝える（Editor を閉じてから再実行するよう案内する）
- ビルドが成功し Editor を起動した場合 → その旨を伝える
- ビルドが失敗した場合 → その旨と、詳細ログの場所（`Saved/BuildLogs/auto_build.log`）を伝える
