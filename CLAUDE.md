# 3D_FreamWork

DirectX11 + PhysX 4.1 で作った、Unity風(GameObject + Component)のC++ゲームフレームワーク。学生チーム(C++初心者を含む)で開発している。詳しい使い方は `README.md` にある。

## 構成

- `3D_FreamWork/3D_FreamWork.sln` に3プロジェクト(x64のみ、Debug/Release)
  - `Engine/` : 静的ライブラリ。全サンプル共通の機能
  - `Sample2D/` : トップダウンの2Dアリーナシューター
  - `Sample3D/` : 3D空間のショーケース
- 各サンプルの中身は `Game/Objects/`(工場関数) / `Game/Components/`(動き・ルール) / `Game/Scenes/`(配置)
- `Assets/` と `Shader*.hlsl` は全サンプル共有。PhysXは `ThirdParty/PhysX/`(設定は `3D_FreamWork/PhysX.props`)

## 必ず守るルール

### Engineを変更する時は必ず確認を取る
`3D_FreamWork/Engine/` 以下と、全サンプル共通の `Shader.hlsl` / `Shader3D.hlsl` / `PhysX.props` は、Sample2D/Sample3Dの両方に影響する。
- 変更する前に、**①何を変えるか ②なぜ必要か ③両サンプルへの影響** を説明して、ユーザーの了承をもらってから手を付ける
- まず「Sample側(Game/のComponentなど)だけで解決できないか」を検討し、できるならそちらを提案する
- 了承はその変更1回分だけ。前に別の変更で了承をもらっていても、新しい変更では改めて聞く
- Edit/Writeツールでの変更は `.claude/settings.json` の設定で毎回確認ダイアログが出る。PowerShellなどでファイルを書き換える場合も同じルールに従う(ダイアログが出ないからといって確認を省かない)

### 命名規則
名前は英語で付ける(ローマ字にしない)。略語は一般的なもの(HP/UI/HUD/ID)だけ。

| 対象 | 書き方 | 例 |
|---|---|---|
| クラス・構造体・`enum class` | PascalCase | `PlayerController`, `MeshVertex`, `BodyType` |
| `enum class` の値 | PascalCase | `BodyType::Dynamic`, `Mode::ThirdPerson` |
| 関数(メンバ関数・自由関数) | PascalCase、動詞から始める | `Update`, `SetVelocity`, `Spawn`, `CreatePlayer` |
| ゲッター / セッター | `Get○○` / `Set○○`、boolを返すものは `Is○○` | `GetHp`, `SetTag`, `IsGameOver` |
| メンバ変数・ローカル変数・引数 | camelCase。`m_` や `_` などの接頭辞は付けない | `speed`, `invincibleTimer`, `mouseWorldPos` |
| bool の変数 | 状態がそのまま読める名前 | `gameOver`, `pressed`, `touchingEnemy` |
| 定数(`static constexpr`) | camelCase | `maxHp`, `spawnInterval` |
| マクロ(`#define`) | UPPER_SNAKE_CASE | `KEY_W`, `WINDOW_W` |
| ファイル名 | 中のクラス名と同じ PascalCase | `PlayerController.h` / `.cpp` |
| 工場関数とそのファイル | `Create○○`、`Game/Objects/○○.h/.cpp` | `CreateEnemySpawner` (`Objects/EnemySpawner.h`) |
| EventBusのイベント名・タグ・オブジェクト名 | PascalCase の文字列 | `"EnemyDefeated"`, `"Bullet"`, `"Player"` |

Componentの名前は役割で語尾をそろえる:
- `○○Controller`: 1つのオブジェクトの動き・入力・当たった時の処理(`PlayerController`)
- `○○Manager`: 見た目の無い管理役(スコア・出現・発射など)(`GameManager`, `EnemyManager`)
- `○○Renderer`: 描画(Engine: `SpriteRenderer`, `MeshRenderer`)
- `○○Component`: どのオブジェクトにも付けられる汎用の機能(`RigidbodyComponent`, `SpinComponent`)

### ファイル
- 日本語を含む `.h`/`.cpp` は **UTF-8 BOM付き** で保存する。BOMが無いと、日本語ロケールのMSVCで C4819 + 意味不明な構文エラーになる。Writeツールで作った/書き直した後は `.claude/scripts/check-files.ps1 -FixBom` を実行する
- 新しいファイルは、所属プロジェクトの `.vcxproj` と `.vcxproj.filters` の **両方** に登録する
- ビルド確認は **Debug と Release の両方**(`.claude/scripts/build.ps1`)
- ユーザーが手で変更したファイルは意図的な変更として扱う。勝手に元に戻さず、おかしいと思ったら指摘だけする

### 設計
- **Sceneには「何を置くか」だけを書く**(`Create○○(*this)` を並べるだけ)。動き・ルールは各オブジェクトのComponentに書く
- オブジェクトは `Game/Objects/○○.h/.cpp` の工場関数 `Create○○(Scene&, 依存...)` で組み立てる。Componentと同じ名前のファイルを作らない(例: `Player` ↔ `PlayerController`、`GameSystem` ↔ `GameManager`、`EnemySpawner` ↔ `EnemyManager`)
- 他のComponentの情報が必要な時は、工場関数の引数でポインタを受け取る。シングルトン(`Get()`)はアプリ全体で1つのサービス(EventBus/SceneManager/Input/Audio)だけ
- お互いを知らない管理役同士は `EventBus` でつなぐ。`this` をキャプチャしたラムダを登録したら、そのSceneの `Uninit()` で `EventBus::Get().Clear()` する
- `ObjectPool` は、そのシーンのGameObjectにアタッチしたComponentのメンバとして持つ(static変数にしない)
- PhysXのDynamicな剛体は、Transformを直接書き換えずに `RigidbodyComponent::SetVelocity()` で動かす
- 2Dでマウスとオブジェクトを比べる時は `Input::GetMouseWorldPosition()`(カメラ位置を考慮済み、Y+=上)

### コメントの書き方
`Sample2D/Game/Components/PlayerController.cpp` に合わせて短く書く。
- `.cpp`: 処理のまとまりごとに1行のラベル(`// 無敵時間`、`// 移動処理: ...`)。理由が分かりにくい所だけ一言足す
- `.h`: クラスの役割を1行、メンバ変数には行末コメント
- 工場関数の `.h`: 何を作るかを1行

### 作業の進め方
- 「コミット文を考えて」と言われたら `commit-message` スキルを使う(README.mdも同じターンで更新する)
- ゲームの操作確認(キー入力・マウス操作)はユーザーにお願いする。合成したキー入力を送らない(別のウィンドウに入力されてしまったことがある)
- 回答・コメント・README は日本語で、初心者にも分かる言葉で書く

## 役割分担(エージェント)

`.claude/settings.json` の `"agent": "navigator"` により、メインの会話は **navigator** として動く。

| 名前 | 役割 | できること |
|---|---|---|
| `navigator` | 話す役(メインの会話) | 相談・説明・計画。ファイルは書き換えず、作業は implementer、確認は reviewer に任せる |
| `implementer` | 作業役(サブエージェント) | 実装・ビルド・ビルドエラーの解決・README更新 |
| `reviewer` | レビュー役(サブエージェント) | バグ・ルール違反・命名・Engine変更のチェック(読むだけ) |

作業の流れ: navigator が依頼を確認 → implementer が作業 → reviewer がレビュー →(要修正なら implementer が直す)→ navigator が報告

数行の修正やコメント・README・文言だけの変更は、reviewer を省いてよい。ただし Engine の変更、新しいファイルの追加、ロジックの変更は必ずレビューする。

## スキル(作業手順書)

| 名前 | 用途 |
|---|---|
| `add-object` | 新しいオブジェクト(工場関数+Component)を規約どおりに追加 |
| `build` | ファイルチェック + Debug/Releaseビルド |
| `commit-message` | コミット文を考えて、READMEも更新 |
