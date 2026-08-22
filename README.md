# Shogi × カード カオスバカ対戦ゲーム（仮）

伝統的な「将棋」の厳密なロジックに「ぶっ壊れ効果のカード」を掛け合わせた、ハチャメチャ展開を楽しむマルチ対戦ゲーム。Unreal Engine 5.6 製。

詳細な企画意図は [`docs/GameConcept.md`](docs/GameConcept.md)、実装仕様は [`docs/GameSpec.md`](docs/GameSpec.md) を参照。

## コンセプト

- 9×9 の盤・通常の将棋の駒配置がベース。
- 各ターン「カード使用」→「駒の移動」の順で進行し、手札のカードが盤面を大きく撹乱する。
- 持ち時間は「60秒＋1ターン15秒の秒読み」方式（詳細: [`docs/2026-08-17-time-bank.md`](docs/2026-08-17-time-bank.md)）。
- 長考を防ぎテンポ重視、ボイスチャットで盛り上がりながら遊べるスケールを目指す。

## 動作環境

- Unreal Engine **5.6**（`Shogi.uproject` の `EngineAssociation`）
- Windows

## アーキテクチャ

- 将棋のコアロジック（盤・駒・移動判定・駒取り・持ち駒・手番・成り・カード効果）は **C++**（`Source/Shogi/`）で実装。
- オンライン対戦基盤（EOSCore 経由のセッション/ロビー/フレンドUI）・タイトル画面・カメラ・移動先マーカーの見た目は **Blueprint**（`Content/`）のまま。
- `Source/Shogi/` は機能別サブフォルダに分割:

  | フォルダ | 内容 |
  | --- | --- |
  | `Core/` | 列挙型・構造体・移動/成りルールなど、アクターを持たない純粋ロジック |
  | `Gameplay/` | `AShogiBoardManager` / `AShogiPiece` / `AShogiPlayerController` / `AShogiGameState` などのアクター |
  | `GameModes/` | `AShogiGameMode` とシングルプレイ派生（CPU対戦・ホットシート） |
  | `UI/` | UMG ウィジェットの C++ 基底クラス |

  各フォルダは `Shogi.Build.cs` の `PublicIncludePaths` に登録済みで、フォルダ名なしで `#include "Foo.h"` のように参照できる。

## 主な機能

- 将棋コアルール: 移動判定・駒取り・持ち駒（打つ）・二歩判定・成り（任意/強制）・王手表示
- シングルプレイ: CPU対戦（`AShogiSinglePlayerVsAIGameMode`）、ローカル対面対戦（`AShogiSinglePlayerHotSeatGameMode`）
- オンライン対戦: EOSCore 経由のセッション/ロビー（Blueprint側）
- カードシステム: 山札からの手札引き直し、複数種のカード効果（永続/即時/時限）
- 持ち時間表示: 60秒＋15秒秒読み方式のタイマーUI

現在の実装状況・既知のギャップの詳細は `docs/GameSpec.md` の各セクション、および `docs/` 配下の個別の変更ログを参照。

## プロジェクト構成

```
Source/Shogi/       将棋コアロジック・カードシステム・GameMode等（C++）
Content/            アセット・Blueprint・マップ・UMGウィジェット
Config/             Unreal Engine プロジェクト設定
docs/                企画書・仕様書・変更ログ
Shogi.uproject       プロジェクトファイル（EngineAssociation: 5.6）
```

## ビルド・起動

このプロジェクトには C++ 変更をビルドし、成功時に Unreal Editor を起動する Claude Code 用カスタムコマンド `/build-ue` が用意されている（詳細: `.claude/commands/build-ue.md`）。

手動でビルドする場合は、`Shogi.uproject` を右クリックして「Generate Visual Studio project files」を実行後、`Shogi.sln` を Visual Studio で開いてビルドするか、Unreal Editor から直接プロジェクトを開く。

## ドキュメント

- [`docs/GameConcept.md`](docs/GameConcept.md) — 企画概要（コンセプト・カード効果一覧・ゲームフロー）
- [`docs/GameSpec.md`](docs/GameSpec.md) — 実装仕様（アーキテクチャ・盤/駒/移動/成り/勝敗判定・Blueprint移行チェックリスト）
- `docs/2026-*.md` — 個別機能の変更ログ（カードシステム、持ち時間、マルチプレイ同期修正など）

## 既知のスコープ外

- 詰み判定・王手放置の禁止（王手の表示のみ実装済み）
- 打ち歩詰め等の禁じ手（二歩のみ実装済み）
- 成り駒用の専用メッシュ/マテリアル

詳細は `docs/GameSpec.md` §9.2・§13（技術的負債）を参照。
