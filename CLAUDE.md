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

## 使えるスキル・サブエージェント

| 名前 | 種類 | 用途 |
|---|---|---|
| `add-object` | スキル | 新しいオブジェクト(工場関数+Component)を規約どおりに追加 |
| `build` | スキル | ファイルチェック + Debug/Releaseビルド |
| `commit-message` | スキル | コミット文を考えて、READMEも更新 |
| `convention-reviewer` | サブエージェント | 変更が上のルールを守れているかレビュー(読むだけ) |
| `build-doctor` | サブエージェント | ビルドエラーの原因調査と修正案(読むだけ) |
| `engine-guide` | サブエージェント | Engineの機能の使い方を初心者向けに解説(読むだけ) |
