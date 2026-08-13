---
description: UEプロジェクトをビルドし、成功したらUnreal Editorを起動する
---

以下の PowerShell スクリプトを実行してください。

```
powershell -NoProfile -ExecutionPolicy Bypass -File .claude/scripts/build-and-launch-ue.ps1
```

スクリプトの標準出力1行目のプレフィックスに応じて、結果をユーザーに簡潔に報告してください。

- `SKIP:` で始まる → その理由（Editor起動中、または環境不足）をそのまま伝え、自動ビルドは行われなかったことを明示する
- `SUCCESS:` で始まる → ビルドが成功し、Unreal Editor を起動したことを伝える
- `SUCCESS_NO_LAUNCH:` で始まる → ビルドは成功したが Editor は起動していないことを伝える（既に起動中、または UnrealEditor.exe が見つからない）
- `FAILURE:` で始まる → ビルドが失敗したことを伝え、詳細ログ（`Saved/BuildLogs/auto_build.log`）を確認するよう案内する
