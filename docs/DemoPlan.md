# デモプロジェクト 設計書(Demo)

## 1. はじめに

### この文書の目的
今まで作ってきた Engine の機能を、**見て・触って分かる形で紹介する「デモプロジェクト(Demo)」**を新しく作るための設計書です。「どんなプロジェクトにするか」「画面をどう作るか」「どんなデモを作るか」「2D と 3D をどう切り替えるか」「今 Sample2D / Sample3D の中にあるデモをどう移すか」「どの順番で作るか」をまとめました。チーム(C++初心者を含む)で分担して実装する時の資料として使います。

### 決まっていること(ユーザーの要望)
1. サンプル(Sample2D / Sample3D)の中にデモを入れるのではなく、**新しいデモプロジェクトを作り**、Engine の機能を紹介する。
2. デモは **2D / 3D / 共通で分けず、「機能ごと」に分ける**。2D と 3D の両方がある機能は、デモの中で **Tab キーや画面のボタンで 2D 版 ↔ 3D 版を切り替えられる**。
3. プロジェクトは **1つ**。名前は **Demo**。ソリューション `3D_FreamWork.sln` に追加する。
4. 今 Sample2D / Sample3D にあるデモ(`Game/Scenes/Demo/`、`Game/Objects/Demo/`、`Game/Components/Demo/`、タイトルの DEMO ボタン=`CreateDemoEntryButton`)は **Demo プロジェクトへ移し、サンプルからは消す**(サンプルを元のゲームだけに戻す)。
5. 「デモの種類が足りない」と言われているので、**今まで作った全ての Engine 機能**を紹介できる、網羅的な一覧にする。

### 状態
**Batch 0(土台と既存6デモの移行)・Batch 1(Core 全8 + UI の Rect・Text・WorldUI)・Batch 2(Render の残り6機能)・Batch 3(Render3D の残り6機能)・Batch 4(Physics の P1〜P5)・Batch 5(Behavior 全6)・Batch 6(Audio の Sound・Sound3D、Data の Csv・Save)は実装済みで、これで Batch 0〜6 がすべて終わりました。ただし A2 Voice は作らないことにしました**(8章の表と「Batch 0 / 1 / 2 / 3 / 4 / 5 / 6 で決めたこと・作った物」を参照。理由は Batch 6 の節)。以下の設計は、実装しながら少し変わった所を除いて、そのまま使えます(変わった所は「Batch 0 で決めたこと・作った物」に書きました)。この文書は、実際のコード(`Sample2D/main.cpp`、`Sample3D/main.cpp`、既存のデモ、`Engine/` の全ヘッダー、`.vcxproj`、README)を読んで書いたので、既存のクラス名・関数名・ファイル名は本物です。新しく作る名前(`DemoMode`、`DemoCategory` など)は案で、実装しながら変わってよいものです。

### Engine は変えません
**この計画は、Engine(`3D_FreamWork/Engine/`)・`Shader*.hlsl`・`PhysX.props` を1行も変えずに実現できます**(調べた結果は5章)。変更が必要になりそうな点が見つかった時だけ、9章に「Engineの変更が必要なら」として書きました(今のところ必須のものはありません)。もし将来 Engine を変える必要が出たら、`CLAUDE.md` のとおり、その1回ごとにユーザーの了承をもらいます。

### 守るルール(`CLAUDE.md` のとおり)
- **Scene には「何を置くか」だけ**(`Create○○(*this)` を並べる)。動き・ルールは各オブジェクトの Component に書く。
- 工場関数は `Objects/○○.h/.cpp` の `Create○○(Scene&, 依存...)`。Component と同じ名前のファイルは作らない(`LightDemo` ↔ `LightDemoController`)。
- 名前は英語(ローマ字にしない)。クラス・関数は PascalCase、変数は camelCase、定数は `static constexpr` の camelCase。Component の語尾は `○○Controller`(1つの動き)/ `○○Manager`(見た目の無い管理役)/ `○○Component`(汎用)。
- コメントは `PlayerController.cpp` に合わせて短く。日本語を含む `.h`/`.cpp` は **UTF-8 BOM付き**。新しいファイルは **`.vcxproj` と `.vcxproj.filters` の両方**に登録。**Debug と Release の両方**でビルド確認。
- 回答・コメント・README は日本語で、初心者にも分かる言葉で書く。
- ゲームの操作確認(キー・マウス)はユーザーにお願いする。合成したキー入力は送らない。

### 進め方の方針(1つの段階ごと)
1. **implementer(作業役)が実装** + Debug/Release のビルド確認
2. **reviewer(レビュー役)がレビュー**(新しいファイル・ロジックの変更は必須)
3. **ユーザーが遊んで確認**
4. **コミット文を考える**(`commit-message` スキル。READMEも同じターンで更新)

---

## 2. 今のコードで分かっている事実(コードを読んで確認したこと)

設計の前提になる事実です。

### 2つのサンプルの `main.cpp` はほぼ同じ
`Sample2D/main.cpp` と `Sample3D/main.cpp` の違いは、**ウィンドウのタイトル(「2Dサンプル」/「3Dサンプル」)とクラス名(`DX11Game2D` / `DX11Game3D`)だけ**です。ウィンドウの大きさはどちらも `1280 x 720`。どちらも `Input::SetHwnd` → `GameEngine::Get().Init(hwnd, WINDOW_W, WINDOW_H)` → `SceneManager::Get().ChangeScene<TitleScene>()` → `Run()` → `Uninit()` という同じ流れです。つまり **Demo の `main.cpp` は、どちらかをコピーして、タイトルと最初のシーン(`DemoMenuScene`)を変えるだけ**で作れます。

### `GameEngine::Init` は2D用・3D用の区別をしない
`GameEngine::Init` は `SaveData::Init` → `Image::Init`(DirectX11)→ `Audio::Init` → `Text::Init` → `Physics::Init` → `Time::Init` を呼びます。**2D専用・3D専用の初期化はありません。** スプライト用のシェーダー(`Shader.hlsl`)も3D用(`Shader3D.hlsl` など)も、どちらのサンプルでも同じように読み込まれます。

### 物理(PhysX)は1つの3D世界。2Dは「重力を切って、Zを固定」して作っている
`Physics::Init` が作る世界の重力は、**どちらのサンプルでも `(0, -981, 0)` の同じ値**です(`Physics.cpp`)。重力を変える関数は今ありません。Sample2D は、各オブジェクト(例: `Player`)で `SetUseGravity(false)`(重力を切る)と `SetFreezePositionZ(true)`(奥に動かさない)を呼んで、2Dの動きにしています。2D用のコライダー(`CircleRigidbodyComponent` / `SquareRigidbodyComponent` / `Capsule2DRigidbodyComponent`)は、中身は3Dの形を薄くしたものです(`flat2D`)。

→ **Demo では、2Dのデモは「重力を切ったオブジェクト」、3Dのデモは「重力ありのオブジェクト」を置くだけで、同じ物理の世界を共有できます。** シーンを切り替えると、古いシーンの Component が `Uninit` で剛体を世界から外すので、2Dのオブジェクトが3Dのデモに残ることはありません。

### カメラは2D用と3D用が別々で、同居できる
- `Image::GetCamera()` = 2D用(スプライト)。`Image::GetCamera3D()` = 3D用(`MeshRenderer` など)。**別のインスタンス**です。
- 既定では `Image::BeginFrame()` が毎フレーム、3Dカメラを2Dカメラに自動で合わせます(`SetCamera3DAutoSync(true)`)。3Dカメラを自分で動かす時は `SetCamera3DAutoSync(false)` にし、**終わったら `true` に戻す**決まりです(`DemoCameraController` は、これを `Uninit` で戻しています)。
- `SpriteRenderer` は、`SetMode(SpriteMode)` で3種類の描き方を持ちます: **UI**(`SpriteMode::UI`。z で大きさが変わらず、必ず最前面。UIどうしは「drawPriority → 作った順」。どちらのシーンでも同じに使える。Sample3D のタイトルのボタンも同じ)、**2D**(`World2D`。z で大きさが変わらず、z が小さいほど手前。2Dカメラでスクロール)、**3D**(`World3D`。3Dカメラの遠近ありのビルボード)。深度バッファは2D・3Dの物だけで使います。
- 座標の変換は、2D用 `Image::WorldToScreen2D` / `ScreenToWorld2D`、3D用 `Image::WorldToScreen3D` / `ScreenPointToRay3D` があります。

### 全体の設定は、シーンをまたいで残る
次の設定は `static` な全体の状態で、**シーンを切り替えても自動では戻りません**。今の `DemoGuideController::ResetSettings()` が、デモに入る時と出る時に戻しています。

| 設定 | 戻し方(今の `ResetSettings` / 追加するもの) |
|---|---|
| 2Dカメラ | `Image::GetCamera().position` を原点へ(今ある) |
| 時間の倍率 | `Time::SetTimeScale(1.f)`(今ある) |
| 3Dカメラ | `Image::GetCamera3D() = Camera()` と `Image::SetCamera3DAutoSync(true)`(Sample3D版にある) |
| 平行光源 | `Image::GetLight() = Light()`(Sample3D版にある) |
| 影 | `Shadow::SetSettings(Shadow::Settings())`(Sample3D版にある) |
| LOD | `Lod::SetEnabled(true)` / `Lod::SetGlobalScale(1.f)`(Sample3D版にある) |
| 当たり判定の枠線 | `RigidbodyComponent::SetDebugDrawEnabled(false)`(Sample3D版にある) |
| **追加**: 視野・経路探索の枠線 | `VisionComponent::SetDebugDrawEnabled(false)` / `NavGridComponent::SetDebugDrawEnabled(false)` / `PathFollowComponent::SetDebugDrawEnabled(false)` |
| **追加**: 画面エフェクト・揺れ | `ScreenEffect::Stop()` / `CameraShake::Stop()` |
| **追加**: 音量 | `Audio::SetGroupVolume(AudioGroup::BGM, 既定)` / `(AudioGroup::SE, 既定)`(音のデモが変えた時) |
| **追加**: カーソル | `Input::SetCursorLocked(false)`(三人称/一人称カメラのデモが固定した時) |
| **追加**: 鳴っている音 | 音のデモの Component が `Uninit` で止める(`AudioSourceComponent` は `Uninit` で止まる) |

(`ScreenEffect::OnSceneChanged()` は切り替え時に自動で呼ばれます。`ScreenEffect::Stop()` を足すのは念のためです。)

### 既存のデモの中身(移行元)
README §38 と実物を確認しました。**Sample2D と Sample3D の両方に、同じ名前・ほぼ同じ内容のファイルがあります**(`DemoRegistry` / `DemoMenuScene` / `DemoGuide` / `DemoButton` / `DemoMenu` / `DemoEntryButton` / `DemoGuideController`)。Demo では1つにまとめます(`DemoGuideController::ResetSettings` は両方の和集合にする)。

| 元 | ファイル |
|---|---|
| 共通(両サンプルに同名) | `Scenes/Demo/`: `DemoRegistry` `DemoMenuScene` / `Objects/Demo/`: `DemoGuide` `DemoButton` `DemoMenu` `DemoEntryButton` / `Components/Demo/`: `DemoGuideController` |
| Sample2D | Tween: `Scenes/Demo/TweenDemoScene`、`Objects/Demo/EaseRunner`、`Components/Demo/EaseRunnerController` / Particle: `ParticleDemoScene`、`Objects/Demo/ParticleDemo` `Campfire`、`Components/Demo/ParticleDemoController` / UI: `UiDemoScene`、`Objects/Demo/UiDemo`、`Components/Demo/UiDemoController` |
| Sample3D | 共通の部品: `Objects/Demo/DemoCamera` `DemoLights` `DemoWalker`、`Components/Demo/DemoCameraController` `DemoWalkerController` / Light: `LightDemoScene`、`Objects/Demo/LightDemo` `MaterialShowcase`、`Components/Demo/LightDemoController` / Shadow: `ShadowDemoScene`、`Objects/Demo/ShadowDemo`、`Components/Demo/ShadowDemoController` / Terrain: `TerrainDemoScene`、`Objects/Demo/TerrainDemo` `Buoy`、`Components/Demo/TerrainDemoController` `FloatOnWaterController` |

注意点: Sample3D のデモは、ゲーム側のファイルを使っています(`CreateSkybox` / `CreateGround` / `CreateHills` / `CreateWater`)。**Demo プロジェクトはサンプルのファイルを参照できない(別プロジェクト)ので、必要な物は Demo にコピーして持ちます**(名前は `DemoSkybox` のように、デモ用の部品として付ける。移行の手順は7章)。

### ビルドの仕組み(`.vcxproj`)
- `Sample2D.vcxproj` / `Sample3D.vcxproj` は、`Application`(Windows)、`PlatformToolset v143`、`CharacterSet Unicode`、`PhysX.props` を import、`/FS`、`ProjectReference` で `..\Engine\Engine.vcxproj`、という作りです。**Demo もこれをコピーして作れます。**
- **出力先は共有です**: Engine / Sample2D / Sample3D は同じ `$(OutDir)`(`3D_FreamWork\x64\Debug` など)へ出力します。`Assets/`・シェーダー・PhysX の DLL を `$(OutDir)` へコピーするのは **Engine の `CopyRuntimeFiles`(ビルド後)** だけで、サンプル側には無いのが決まりです(並行ビルドの衝突を避けるため)。**Demo も同じ `$(OutDir)` に出力する限り、そのコピーをそのまま使え、Demo 側にコピーの設定は要りません**(`ProjectReference` で Engine より後にビルドされるので、Demo が起動する時にはコピー済みです)。
- 各プロジェクトの中間ファイル(`.obj`)は、プロジェクトごとの `x64\Debug` に出ます。**同じプロジェクトの中に同じ名前の `.cpp` があると、`.obj` が衝突して失敗します** → Demo では **ファイル名をプロジェクト内で重複させない**(例: `ParticleDemo2DScene` / `ParticleDemo3DScene`)。

### 既存のデモの方式(踏襲するもの)
- `ButtonComponent` は **Arial 固定**で日本語が出ません → ボタンの文字は**短い英語**(`Light` `Shadow` `Back`)にし、日本語の説明は `TextRenderer`(Noto Sans JP)でボタンの右に出す。
- `DemoRegistry` は「一覧を1か所に持ち、1行足せばメニューに並ぶ」仕組み。`DemoMenu` が一覧を読んでボタンを並べ、数が増えると間隔が縮む。
- `DemoGuide`(左上の4行+左下の Back ボタン+Esc で戻る)を、全デモが `CreateDemoGuide(*this, {...}, OpenXxx)` の1行で使う。
- デモの Component は、自分が変えた全体の設定を `Uninit` で戻す。

---

## 3. プロジェクト構成

### 3.1 名前と置き場所
- プロジェクト名: **Demo**(`Demo.vcxproj`、`RootNamespace` も Demo)
- 置き場所: **`3D_FreamWork/Demo/`**(`Sample2D/` `Sample3D/` `Engine/` と同じ階層)
- ソリューション: `3D_FreamWork.sln` に追加(x64 の Debug / Release)。起動プロジェクトをどれにするかは、ユーザーが決めます(未確定事項 12番)。
- GUID: 新しく作る。`ProjectReference` で `..\Engine\Engine.vcxproj` を参照する(Sample と同じ)。

### 3.2 フォルダ構成(機能ごと)
`Game/Scenes/` ・ `Objects/` ・ `Components/` の下を、**機能のカテゴリごとのサブフォルダ**に分けます(今の `Demo/` サブフォルダの代わり)。`CLAUDE.md` の「Scene は配置だけ・工場関数は Objects・動きは Components」は、そのまま守ります。

```
3D_FreamWork/Demo/
  Demo.vcxproj / Demo.vcxproj.filters
  main.cpp
  Game/
    Scenes/
      Common/      DemoRegistry, DemoMenuScene(カテゴリ一覧), DemoCategoryScene(機能一覧)
      Core/        基本(C)のデモのシーン
      Render/      描画と演出(R)
      UI/          UIと文字(U)
      Render3D/    3D描画(D)
      Physics/     物理(P)
      Behavior/    動きとAI(B)
      Audio/       音(A)
      Data/        データ(T)
    Objects/       (Scenes と同じサブフォルダ。Common には DemoGuide, DemoButton, DemoMenu, DemoModeButton, DemoCamera, DemoLights, DemoWalker, DemoSkybox, DemoGround ...)
    Components/    (Scenes と同じサブフォルダ。Common には DemoGuideController, DemoCameraController, DemoWalkerController ...)
```

- **カテゴリのフォルダ名は Engine の11フォルダに近い英語**にしました(`Core` `Render` `Render3D` `Physics` `Behavior` `Audio` `Data` `UI`)。「このデモはどの Engine フォルダの機能か」がすぐ分かります。
- **`Common/` には、どのデモも使う部品**を置きます(デモの説明板・戻るボタン・2D/3D 切り替え・共通のカメラなど)。
- **フィルター(`.vcxproj.filters`)も同じ名前**にします(例: `Game\Scenes\Render3D`)。
- **2D版と3D版は、同じ機能のフォルダに並べます**(`Scenes/Render/ParticleDemo2DScene` と `ParticleDemo3DScene`)。部品(`Objects/Render/ParticleDemo`)と動き(`Components/Render/ParticleDemoController`)は、2D版・3D版で**できるだけ共有**し、違いは引数(`DemoMode`)で出します。共有できない時だけ `○○Demo2D` / `○○Demo3D` に分けます。
- **ファイル名はプロジェクト内で重複させない**(前の章の `.obj` の衝突)。Scene は `○○Demo2DScene` / `○○Demo3DScene` / `○○DemoScene`(2D・3D共通の1つだけの時)にする。

### 3.3 命名の決まり(Demo 用)
| 対象 | 名前の例 |
|---|---|
| 2D版のシーン | `ParticleDemo2DScene` |
| 3D版のシーン | `ParticleDemo3DScene` |
| 2D・3Dの区別が無い機能のシーン | `InputDemoScene` |
| 工場関数・ファイル | `CreateParticleDemo`(`Objects/Render/ParticleDemo.h/.cpp`) |
| Component | `ParticleDemoController`(`Components/Render/`) |
| 2D/3D の種類 | `enum class DemoMode { Mode2D, Mode3D };` |
| カテゴリの種類 | `enum class DemoCategory { Core, Render, UI, Render3D, Physics, Behavior, Audio, Data };` |

`enum class` の値は PascalCase(CLAUDE.md)。数字から始められないので `Mode2D` / `Mode3D` にしました。

### 3.4 ビルド設定
- `ConfigurationType`=`Application`(Subsystem=Windows)、`v143`、Unicode、`x64` の Debug / Release のみ。
- `PhysX.props` を import(`..\PhysX.props`)。`/FS` を付ける。
- `ProjectReference` で `..\Engine\Engine.vcxproj`(静的ライブラリをリンク)。
- `Assets/`・シェーダー・PhysX DLL は、Engine の `CopyRuntimeFiles` が共有 `$(OutDir)` へコピーする(Demo 側は何もしない)。
- 出力先は共有 `$(OutDir)`(`x64\Debug` / `x64\Release`)。`Demo.exe` はここに出る。
- ビルド確認: `.claude/scripts/build.ps1` は**ソリューション全体**をビルドするので、Demo を `.sln` に入れれば、Debug/Release の両方で自動的に確認されます。`check-files.ps1` が、vcxproj の登録漏れを `Demo/` についても調べるかは、Batch 0 で確認します(調べないなら、`.claude/scripts/` の変更が必要=ユーザー本人の了承が必要。未確定事項 13番)。

---

## 4. 画面の設計

### 4.1 画面の流れ
```
起動 → カテゴリ一覧(DemoMenuScene) → 機能一覧(DemoCategoryScene) → 各デモ
         Esc: アプリを終了              Esc: カテゴリ一覧へ             Esc: 機能一覧へ
```
- **起動**: ウィンドウが開いて、すぐ「カテゴリ一覧」が出ます(タイトル画面は作らない。デモなので、すぐ見られる方がよいため)。
- **カテゴリ一覧(1階層目)**: 8個のカテゴリのボタン(英語名)と、日本語の説明。上のほうに今の **2D/3D の選択**(5章)。
- **機能一覧(2階層目)**: 選んだカテゴリの機能のボタンが並ぶ。**機能ごとに「2D」「3D」「2D/3D」「共通」の印**が付く。今の選択(2D か 3D か)で、その機能がその側に対応していなければ、ボタンを暗くして「(3D のみ)」のように出す。
- **各デモ**: 説明板・戻る・2D/3D 切り替え・前/次(任意)が付く(4.2)。

### 4.2 各デモ画面の共通部品(`DemoGuide`)
今の `CreateDemoGuide` を拡張した、1行で付く共通部品です。

| 部品 | 内容 |
|---|---|
| 説明の板(左上の4行) | デモの名前 / 何の機能か / 操作方法 / 使っている API。今のまま(`DemoGuideInfo`)。名前の横に、今の側を **`[2D]` / `[3D]`** と出す |
| Back ボタン(左下)と Esc | 機能一覧へ戻る |
| **2D ⇄ 3D ボタン(右下)と Tab キー** | 今の側と逆側の版へ切り替える。逆側が無い機能では、ボタンを暗くして「2D only」「3D only」と出す(押せない) |
| 前 / 次(任意。下のボタンと PageUp / PageDown) | 同じカテゴリの前後のデモへ。**Q / E(3D カメラのズーム)や矢印キーと被らないキー**にする。最初は付けず、後から足せるように作る(未確定事項 8番) |
| 全体の設定のリセット | `DemoGuideController` が `Init` / `Uninit` で行う(5.4) |

**日本語の出し方**: `ButtonComponent` は Arial 固定で日本語が出ないので、**ボタンの文字は英語**(`Back` `2D` `3D` `Prev` `Next`)、**説明は日本語の `TextRenderer`**(Noto Sans JP)。今のデモと同じ方式です。

### 4.3 `DemoRegistry` の拡張(機能ごと+2D/3D 対応)
今は `DemoInfo { name, description, open }` で、デモ1つにつき開く関数が1つです。これを次のように変えます(案)。

```cpp
struct DemoInfo {
    DemoCategory category;        // どのカテゴリか(機能一覧の分け方)
    const wchar_t* name;          // ボタンの名前(短い英語)
    const wchar_t* description;   // 一言説明(日本語)
    void (*open2D)();             // 2D版を開く関数(2D版が無ければ nullptr)
    void (*open3D)();             // 3D版を開く関数(3D版が無ければ nullptr)
};
```
- 一覧に足す行は、**1機能につき1行**: `{ DemoCategory::Render, L"Particle", L"...", &OpenScene<ParticleDemo2DScene>, &OpenScene<ParticleDemo3DScene> },`。
- 2D と 3D の区別が無い機能(「共通」)は、**`open2D` と `open3D` に同じ関数**を入れる(どちらの側でも同じシーンが開き、切り替えボタンは「共通」と出して無効)。
- 印は `open2D` / `open3D` が `nullptr` かどうかから決まるので、**印を別に持たない**(書き間違えない)。
- 3D のみ(`open2D == nullptr`)・2D のみの機能は、反対側の選択の時に暗く出す。
- `GetDemoList()` が全件、`GetDemoListOf(DemoCategory)` がカテゴリ別を返す。`OpenDemo(index)` が今の側で開く。`OpenMenu()` / `OpenCategory(DemoCategory)` が出入り口。
- 現在の側(`DemoMode`)と、今のデモの番号は、`DemoRegistry` の中の `static` な状態にして `GetDemoMode()` / `SetDemoMode()` で読み書きする(Engine の `Image` などと同じ「名前空間+static」の流儀。`Get()` のシングルトンは作らない)。
- 新しいデモを足す手順は今と同じ(Scene を作る → 部品を作る → `DemoRegistry.cpp` に1行足す → vcxproj / filters に登録)。

### 4.4 メニューの作り
- `DemoMenuScene`(カテゴリ一覧)は、`DemoCategory` を並べるだけ。カテゴリの名前・説明は `DemoRegistry` に表で持つ。
- `DemoCategoryScene`(機能一覧)は、**1つのシーン**で、今のカテゴリを `DemoRegistry` の状態から読んで、そのカテゴリのデモを並べる(カテゴリごとにシーンを作らない)。1画面に収まらない数(例: 3D描画の11個)は、今の `DemoMenu` と同じく間隔を縮めるか、2列にする。
- ボタンの横の印(`[2D]` `[3D]` `[2D/3D]` `[ALL]`)は、`TextRenderer` で出す(日本語の説明の左に、短い英字)。

---

## 5. 1つのプロジェクトで 2D と 3D の両方を扱う方法

### 5.1 結論
**できます。Engine の変更は要りません。** 調べた結果、2D と 3D の同居を妨げるものはありませんでした(下の表)。必要なのは、**「シーンごとに全体の設定を既定へ戻す」ことを徹底すること**です。

### 5.2 障害になりそうな点の調査結果
| 調べたこと | 結果 | 解決策 |
|---|---|---|
| `main.cpp` の違い | タイトルとクラス名だけ。ウィンドウの大きさは同じ | 1つの `main.cpp` で足りる |
| `Engine::Init` | 2D/3D の区別なし | そのまま |
| シェーダー | スプライトも3Dも常に読み込み済み | そのまま |
| 物理の重力 | どちらも `(0,-981,0)` の1つの世界。2Dは各オブジェクトで重力を切る | 2Dのデモは `SetUseGravity(false)` + `SetFreezePositionZ(true)` を付ける(`Player` と同じ)。重力を変える API は要らない |
| カメラ | 2D用/3D用が別。3Dカメラは既定で2Dカメラに追従 | 3Dのデモはカメラを自分で操作する時だけ `SetCamera3DAutoSync(false)` にし、`Uninit` で戻す(`DemoCameraController` と同じ) |
| 2Dシーンに3Dの物が残る | シーンの `Uninit` で全 Component が後片付けする(剛体・光・画像) | 追加の作業なし(README §38 の「出入りの順序」で確認済み) |
| 全体設定(光・影・LOD・倍率など)が残る | **残る** | 2.の表の項目を `ResetSettings` で戻す(**リセット漏れが最大のリスク**。9章) |
| スプライトを3Dの世界に置く | `SpriteRenderer` のワールドモード(ビルボード)が3Dカメラで描かれる | 3D版のパーティクル・スプライトで使う |
| `Camera` 同士の座標の違い | 2D: Y+ が上の画面、3D: XZ が地面、Y+ が上 | 2D版は `MovePlane::XY`、3D版は `MovePlane::XZ`(`MoveComponent` などが対応済み) |

### 5.3 2D ⇄ 3D の切り替えの設計
- **機能ごとに「2D シーン」と「3D シーン」を別々のクラスで持つ**(`ParticleDemo2DScene` / `ParticleDemo3DScene`)。1つのシーンの中で2Dと3Dの物を出し入れするより、**シーン遷移で入れ替える方が、後片付け(`Uninit`)が確実**で、Scene が「何を置くか」だけで済む(CLAUDE.md の設計)。
- 切り替えの実装: `DemoGuideController::Update` が `Tab` キー(`KEY_TAB`)を見て、`SwitchDemoMode()` を呼ぶ。`SwitchDemoMode()` は、
  1. 逆側の版があるか(`open2D` / `open3D` が `nullptr` でないか)を調べる。無ければ何もしない(ボタンが暗いのと同じ)。
  2. `SetDemoMode()` で今の側を逆にする。
  3. 逆側の版のシーンを `FadeTransition` で開く(`OpenScene<T>` と同じ。暗転中に古いシーンの `Uninit`(設定のリセット)→ 新しいシーンの `Init`(もう一度リセット)が走る)。
- 画面の 2D⇄3D ボタンは、`CreateDemoModeButton`(Objects/Common)で作る。押した時の処理は Tab と同じ `SwitchDemoMode()`。ボタンの文字は英語(「To 3D」「To 2D」)、暗い時は「2D only」。
- **次のデモでも同じ側を使う**: 今の側(`DemoMode`)は `DemoRegistry` の状態に1つだけ持ち、「Particle の 3D を見ていて、次のデモへ」「Back で機能一覧へ」の時も同じ側のままにします。その機能が今の側に対応していない時(2D のみの機能を 3D の側で開こうとした時)は、メニューで暗くして押せなくします。機能ごとに側を覚える案もありますが、**最初は全体で1つ**にします(分かりやすい。未確定事項 3番)。
- 起動時の側の初期値は 3D か 2D のどちらか(未確定事項 4番)。次回の起動時に前回の側を覚える(`SaveData`)かもユーザーが決めます(未確定事項 5番)。

### 5.4 全体設定のリセット(`DemoGuideController::ResetSettings`)
**デモに入る時(`Init`)と出る時(`Uninit`)の両方**で、次をすべて既定へ戻します(2章の表のとおり)。2D版・3D版で分けず、**全部まとめて1か所**で戻します(漏れを防ぐため)。`DemoMenuScene` / `DemoCategoryScene` も `CreateDemoGuide` を使うので、メニューに戻ってきた時も戻ります。

- 2D カメラの位置、3D カメラ(`Camera()` に戻す+`SetCamera3DAutoSync(true)`)
- `Time::SetTimeScale(1.f)`
- 平行光源、影の設定、LOD の設定
- 当たり判定・視野・経路の枠線(すべて `false`)
- 画面エフェクト・カメラの揺れの停止
- BGM/SE の音量グループ、カーソルの固定
- (`ParticleRenderer` や `LightComponent` など、**オブジェクトに付く物は Component が自分で後片付けする**ので、ここには入れない)

デモ固有の変更(影のデモが `Shadow` の設定を変える、など)は、これまでどおり**そのデモの Component の `Uninit` でも戻す**(二重の安全策)。

### 5.5 Sample2D と Sample3D の初期設定の統合
差は、ウィンドウのタイトルとクラス名だけです。Demo の `main.cpp` は次のとおりにします。
- ウィンドウのタイトルは「Engineデモ」のような名前、クラス名は `DX11Demo`。大きさは `1280 x 720`(サンプルと同じ)。
- `Physics::Init` の重力はそのまま(変えない)。
- 最初のシーンは `SceneManager::Get().ChangeScene<DemoMenuScene>()`。

---

## 6. デモの一覧(Engine の全機能を紹介する)

### 6.1 印の意味
| 印 | 意味 | `DemoInfo` |
|---|---|---|
| **2D** | 2D版だけ | `open2D` のみ |
| **3D** | 3D版だけ | `open3D` のみ |
| **両方** | 2D版と3D版の両方がある(Tab / ボタンで切り替え) | 両方 |
| **共通** | 2D と 3D の区別が無い機能。シーンは1つ(どちらの側でも同じ) | 同じ関数を両方に |

「流用元」は、今 Sample にあるコード(移行して使う)です。「新規」は新しく作ります。

### 6.2 件数のまとめ
| カテゴリ | 件数 | 内訳 |
|---|---|---|
| Core(基本) | 8 | 両方 3 / 共通 5 |
| Render(描画と演出) | 8 | 両方 6 / 共通 2 |
| UI(UIと文字) | 4 | 両方 1 / 共通 3 |
| Render3D(3D描画) | 10 | 3D 9 / 両方 1 |
| Physics(物理) | 6 | 両方 5 / 3D 1 |
| Behavior(動きとAI) | 6 | 両方 5 / 3D 1 |
| Audio(音) | 2 | 共通 1 / 両方 1(A2 Voice は作らない) |
| Data(データ) | 2 | 共通 2 |
| **合計** | **46機能** | 両方 22 / 3D 11 / 共通 13。シーンの数は 22x2 + 11 + 13 = **68** |

(既存の6デモ=Tween・Particle・UI・Light・Shadow・Terrain は、この47機能に含まれます。Terrain は「地形の見た目」「地形の当たり判定」「水面」の3つに分けます。件数の詳しい確認は6.11。)

### 6.3 Core(基本)

| No | デモ名 | 印 | 見せる内容 | 使う Engine の API | 操作 | 流用元 |
|---|---|---|---|---|---|---|
| C1 | **Object** | 両方 | GameObject を作る・壊す、Component を付ける・外す・止める。物を置いて、キーで Component の付け外しをし、動きが変わるのを見る(2D: 平面で / 3D: 立方体で) | `Scene::CreateObject`、`GameObject::AddComponent` `GetComponent`、`Component::SetEnabled`、`Destroy` | 1: 動きの Component を付ける / 2: 外す / 3: 止める・再開 / クリックで物を足す | 新規 |
| C2 | **Transform** | 両方 | 位置・大きさ・回転、親子階層(親を回すと子がついてくる)。2D は Z 回転、3D は 3 軸 | `Transform`(`position` `rotate` `scale`、親子) | 矢印: 親を動かす / R: 回す / SPACE: 親子をつなぐ・切る | 新規 |
| C3 | **Time** | 共通 | 時間の倍率(スロー)、ポーズ、`Timer`(○秒ごと・1回だけ)。止まっている間に動く UI(`GetUnscaledDeltaTime`) | `Time::GetDeltaTime` `GetUnscaledDeltaTime` `SetTimeScale` `IsPaused`、`Timer` | SPACE: スロー / P: ポーズ / 1〜3: タイマーを開始 | 新規(Particle のスローを参考) |
| C4 | **Input** | 共通 | キー・マウス・ボタン・ゲームパッド(XInput)の今の状態を、画面に出す(押した瞬間・押している間・離した瞬間、スティック、トリガー、マウスの位置・動き、`GetMouseWorldPosition`) | `Input::GetKeyDown/Press/Up`(キーとパッドのボタン)、`GetMousePosition` `GetMouseDelta` `GetMouseWorldPosition`、`GetPadLeftStick` `GetPadRightStick` `GetPadTrigger`、`SetCursorLocked` | 何でも押す / パッドをつなぐ / F1: カーソル固定 | 新規 |
| C5 | **Math** | 両方 | `MathUtil` を図で見る。2D: Lerp / Clamp / 角度 / 乱数の分布 / `Distance`。3D: ベクトルの `Dot` `Cross` `Normalize` を矢印の線で | `MathUtil`(`Lerp` `Clamp` `Normalize` `Dot` `Cross` `Distance` 角度、乱数) | マウス: 点を動かす / 1〜4: 関数を選ぶ | 新規 |
| C6 | **Pool** | 共通 | `ObjectPool` で弾を出し入れする。「出した数・待機中の数・作った数」を表示。使い回すので作った数が増えない | `ObjectPool`(`Prewarm` `Rent` `Return` `GetPoolSize`) | クリック/SPACE: 弾を出す | 新規(ゲームの `BulletManager` を参考) |
| C7 | **EventBus** | 共通 | 「撃破」「得点」のイベントを、互いを知らない2つの管理役がやり取りする。送った/受け取ったログを表示 | `EventBus::Get().Subscribe` `Publish` `SubscribeObject` `PublishObject`(`Clear` は `Uninit` で) | ボタン: イベントを送る | 新規(`GameManager` を参考) |
| C8 | **Resources** | 共通 | 同じ画像を何枚置いても、読み込みは1回(参照の数)。`UnloadUnused` でデモから戻ると解放される。読み込み済みの数を表示 | `Resources::LoadTexture` `ReleaseTexture` `LoadSound` `LoadModel` `UnloadUnused` | ボタン: 同じ画像を 10 個増やす / 消す | 新規 |

(Core の件数: 両方 = C1, C2, C5 の 3、共通 = C3, C4, C6, C7, C8 の 5。計 8。)

### 6.4 Render(描画と演出)

| No | デモ名 | 印 | 見せる内容 | 使う Engine の API | 操作 | 流用元 |
|---|---|---|---|---|---|---|
| R1 | **Sprite** | 両方 | 2D: 画像・色・大きさ・回転・UI・2D・3Dの描き方(UIは作った順で必ず最前面、2Dはzの深度で前後・遠近なし、3Dは遠近あり)・左右反転など。3D: 奥行きの中に置くスプライト(ビルボード、軸ごとのロック `SetBillboardLock`) | `SpriteRenderer`(色、シート、UIモード、`SetBillboardLock`)、`Image::Draw` | 1〜4: 見た目を変える / 3D: 右ドラッグで視点 | 新規 |
| R2 | **SpriteAnim** | 両方 | コマ割りのアニメ(再生・停止・速度・ループ)。3D 版は、ビルボードのスプライトで同じ物を再生 | `SpriteAnimationComponent`(`SpriteRenderer` のシート) | SPACE: 再生/停止 / 1〜3: 速さ | 新規(ゲームの `Explosion` 等を参考) |
| R3 | **Camera** | 両方 | 2D: `CameraFollowComponent`(なめらか追従・デッドゾーン・範囲)と `CameraShake`。3D: 三人称/一人称カメラ(`CameraController`)と `CameraShake`(3D カメラの揺れ) | `CameraFollowComponent`(`SetSmoothTime` `SetBounds` `SetDeadZone` `SetOffset`)、`CameraShake::Shake`、`Image::GetCamera/GetCamera3D`、`SetCamera3DAutoSync` | WASD: 動く / SPACE: 揺らす / 3D: V で視点切替 | 新規(ゲームの `CameraController` を参考) |
| R4 | **ScreenEffect** | 共通 | 画面全体の演出: `Flash`(光る)、`Vignette`(赤い縁。点滅)、`Fade`(暗転・完了で関数)。`IsFading` の状態表示 | `ScreenEffect::Flash` `Vignette` `Fade` `IsFading` `Stop` | 1: Flash / 2: Vignette / 3: Fade(黒) / 4: Fade(白) | 新規 |
| R5 | **Transition** | 共通 | シーン切り替えの演出を見比べる: `FadeTransition`(黒/白、秒数)、`SpriteTransition`(丸・ワイプの画像。`transition_circle.png` `transition_wipe.png`)。切り替え中の `Time::IsPaused` | `SceneManager::ChangeScene<T>(FadeTransition(...))` `(SpriteTransition(...))`、`SetOpenSheet` `SetSwitchFrame` | 1〜4: 演出を選んで同じ別シーンへ往復 | 新規(`Back`/メニューで使っている `FadeTransition` と同じ) |
| R6 | **Particle** | 両方 | 2D: 爆発・火花・煙・花火・焚き火(既存)。**3D: 同じ設定を `use3D` で3D空間に(奥行き、カメラを回して見る)** | `ParticleSettings`(`additive` `gravity` `startColor` `endColor` `startSize` `endSize` `use3D`)、`CreateEffect`、`ParticleRenderer`(`Burst` `Play`) | 1〜4: 種類 / クリック: 発射(3D: 右ドラッグで視点) / SPACE: スロー | 2D=Sample2D の `ParticleDemoScene`、3D=新規(`use3D`) |
| R7 | **Tween** | 両方 | 8種類の Ease を同時に比べる(既存)。**3D 版: 8個の立方体が奥へ動く(位置・色・大きさ・回転)** | `TweenComponent`(`MoveTo` `ScaleTo` `RotateTo` `ColorTo` `TweenValue` `SetLoop` `SetDelay` `SetOnComplete`)、`Ease::*`(§12) | SPACE: やり直し | 2D=Sample2D の `TweenDemoScene`(`EaseRunner`)、3D=新規 |
| R8 | **Coord** | 両方 | 座標の変換。2D: `WorldToScreen2D` / `ScreenToWorld2D`(Z で大きさが変わる)。3D: `WorldToScreen3D`(頭の上に文字)と `ScreenPointToRay3D`(クリックした所の物を `Raycast` で探す) | `Image::WorldToScreen2D` `ScreenToWorld2D` `WorldToScreen3D` `ScreenPointToRay3D`、`Physics::Raycast` | マウス: 動かす・クリック | 新規 |

(Render の件数: 両方 = R1, R2, R3, R6, R7, R8 の 6、共通 = R4, R5 の 2。計 8。)

### 6.5 UI(UIと文字)

| No | デモ名 | 印 | 見せる内容 | 使う Engine の API | 操作 | 流用元 |
|---|---|---|---|---|---|---|
| U1 | **UI** | 共通 | スライダー→ゲージ、Damage/Heal ボタンで遅れて縮む HP ゲージ、Count UP ボタン、9か所の基準点(`Anchor`) | `ButtonComponent`、`SliderComponent`、`GaugeRenderer`(`SetValueSource` `SetDelayedBar`)、`TextRenderer`、`Anchor` / `GetAnchoredPosition` | スライダーをドラッグ / ボタンをクリック | Sample2D の `UiDemoScene` |
| U2 | **Text** | 共通 | 文字の見た目(大きさ・色・縁取り・影?・左/中央/右寄せ)、日本語フォント(`LoadFont`)、`TextRenderer` を物に付けて動かす | `Text::LoadFont` `Text::Draw`、`TextStyle`、`TextAlign`、`TextRenderer`(`SetText` `SetStyle` `SetOffset`) | 1〜4: スタイル切替 / 入力した文字を表示 | 新規(`UiDemo`・`DemoGuide` の文字を参考) |
| U3 | **Rect** | 共通 | `Image::DrawRect` で四角を塗る。`Anchor` の9か所(四隅・辺・中央)に印を出し、ウィンドウの大きさに合わせて位置が決まるのを見る | `Image::DrawRect`、`Anchor::*`、`GetAnchoredPosition` `GetAnchoredCenter` | ドラッグ: 四角を動かす | UI デモの Anchor 部分を分離 |
| U4 | **WorldUI** | 両方 | 敵の頭の上に HP バーと名前を出す(物に追従する UI)。2D は `WorldToScreen2D`、3D は `WorldToScreen3D`(カメラの後ろにある時は出さない `z > 0` の判定) | `Image::WorldToScreen2D/3D`、`GaugeRenderer`、`TextRenderer`、`Image::DrawRect` | 矢印: 物を動かす / 3D: 右ドラッグで視点 | 新規(ゲームの `HUD` を参考) |

(UI の件数: 両方 = U4 の 1、共通 = U1, U2, U3 の 3。計 4。)

### 6.6 Render3D(3D描画)

| No | デモ名 | 印 | 見せる内容 | 使う Engine の API | 操作 | 流用元 |
|---|---|---|---|---|---|---|
| D1 | **Shapes** | 3D | 図形(立方体・球・カプセル・OBJ)と `Material`(色・画像・光沢・発光・半透明・光を無視)。球の分割数の違い | `Mesh::CreateCube` `CreateSphere(rings, segments)` `CreateCapsule` `LoadOBJ`、`MeshRenderer`、`Material`(`specularStrength` `shininess` `emissiveColor` `lit` `depthWrite` ほか) | 右ドラッグ: 視点 / Q・E: ズーム / 1〜4: 見た目 | Sample3D の `MaterialShowcase` を分離 |
| D2 | **Light** | 3D | 平行光源・点光源・スポットライトを見比べる(オン/オフ、太陽の向き) | `Image::GetLight`、`LightComponent`(`Point` `Spot` `SetSpotAngle`)、`Lighting` | 1・2・3: 光のオン/オフ / 矢印: 太陽 | Sample3D の `LightDemoScene` |
| D3 | **Shadow** | 3D | 太陽が回ると影が動く。影の有無・受ける/落とす・ぼかし・スポットの影 | `Shadow::GetSettings` `SetSettings`、`Material`(`castShadow` `receiveShadow`)、`LightComponent::SetCastShadow` | 1〜5: 切替 / 矢印: 太陽 | Sample3D の `ShadowDemoScene` |
| D4 | **Outline** | 3D | 枠線(`outlineWidth`)、壁越しに見える枠線(`seeThrough`)、縁の光(リム発光) | `Material`(`outlineWidth` `outlineColor`? `seeThrough` `rimStrength` `rimColor`?)(実際の項目名は着手時に `Material.h` で確認)、`SetOutline` `SetRimLight` | 1〜3: 切替 / スライダーで太さ | 新規(Light/Shadow デモの枠線・リムを参考) |
| D5 | **Terrain** | 3D | 高さの画像(`heightmap.png`)から作った丘。CSV から作る/プログラムで作る。高さと向き(法線)を表示 | `Terrain` / `TerrainRenderer`(`GetHeightAt` `GetNormalAt` `Contains`)、`TerrainSettings` | 右ドラッグ: 視点 / 1〜3: 作り方を切替 | Sample3D の `TerrainDemoScene`(見た目の部分) |
| D6 | **Water** | 3D | 池の水面。色・波の高さ・波の速さを変える。物を波に合わせて浮かべる | `WaterRenderer`(`WaterSettings` `GetSettings` `GetHeightAt`)、`FloatOnWaterController` | 1〜5: 波・色 | Sample3D の `TerrainDemoScene`(水面の部分)、`Buoy` |
| D7 | **Model** | 3D | FBX モデルを置く(複数パーツ)。向き・大きさ・単位の変換 | `Resources::LoadModel`、`ModelRenderer`、`Model::GetBoundingRadius` | 右ドラッグ: 視点 / 1〜3: モデルの切替 | 新規(ゲームの `ModelDisplay` を参考。`TestParts.fbx`) |
| D8 | **SkinAnim** | 3D | 骨で曲がるモデルのアニメ。クリップ一覧、`Play`、`CrossFade`(なめらかに切り替え)、速さ | `ModelAnimationComponent`(`Play` `CrossFade` `GetClipNames` `SetSpeed` `IsFinished`)、`SkinMesh` | 1〜3: クリップ / SPACE: CrossFade / ↑↓: 速さ | 新規(ゲームの `SkinDisplay` を参考。`TestSkin.fbx` `TestAnim.fbx`) |
| D9 | **LOD** | 3D | 遠くの物を省く距離。柱が15x15本並ぶ場所で、LOD のオン/オフと距離の倍率を変えて、描画命令数・三角形数が変わるのを見る | `Lod::SetEnabled` `SetGlobalScale`、`RenderStats::GetDrawCallCount` `GetDrawTriangleCount` `GetLodCulledCount` ほか | 1: LOD オン/オフ / ↑↓: 距離 / 右ドラッグ: 視点 | 新規(ゲームの `WallField` を参考) |
| D10 | **Visibility** | 両方 | 画面に映っているかの判定。映っていない物は描かれない(`SetFrustumCulling`)。省かれた数を表示。2D は `IsCircleInView2D`、3D は `IsSphereInView3D` | `Visibility::IsCircleInView2D` `IsRectInView2D` `IsSphereInView3D` `IsBoxInView3D` `GetCulledCount`、`SetFrustumCulling` | 矢印/WASD: カメラを動かす | 新規 |

(Render3D の件数: 3D = D1〜D9 の 9、両方 = D10 の 1。計 10。)

### 6.7 Physics(物理)

| No | デモ名 | 印 | 見せる内容 | 使う Engine の API | 操作 | 流用元 |
|---|---|---|---|---|---|---|
| P1 | **Collider** | 両方 | コライダーの種類を並べる。2D: Circle / Square / Capsule2D。3D: Box / Sphere / Capsule。当たり判定の枠線(デバッグ表示)をオン/オフ | `CircleRigidbodyComponent` `SquareRigidbodyComponent` `Capsule2DRigidbodyComponent` / `BoxRigidbodyComponent` `SphereRigidbodyComponent` `CapsuleRigidbodyComponent`、`BodyType`、`RigidbodyComponent::SetDebugDrawEnabled` | F1: 枠線 / クリック: 物を落とす/置く | 新規 |
| P2 | **Force** | 両方 | 力と速度の違い。`AddForce`(Force / Impulse / Acceleration / VelocityChange の4種類)、`SetVelocity`、`AddTorque`。同じ物を違う方法で動かして比べる | `RigidbodyComponent::AddForce` `AddTorque` `SetVelocity` `GetVelocity`、`ForceMode` | 1〜4: 力の種類 / SPACE: 力をかける | 新規 |
| P3 | **RigidbodySettings** | 両方 | 質量・空気抵抗(`SetDrag`)・重力のオン/オフ・回転固定・位置固定(X/Y/Z)を変えて、同じ物が違う動きをするのを比べる | `SetMass` `SetDrag` `SetUseGravity` `SetFreezeRotation` `SetFreezePositionX/Y/Z` | 1〜5: 設定を切替 | 新規 |
| P4 | **Trigger** | 両方 | トリガーに入る・中にいる・出る(`OnTriggerEnter/Stay/Exit`)と、ぶつかる(`OnCollisionEnter`)の違い。何回呼ばれたかを表示 | `Component::OnTriggerEnter/Stay/Exit`(2D版は `...2D` の名前。実際の名前は着手時に `Component.h` で確認)、`OnCollisionEnter`、`isTrigger` | 矢印/WASD: 物を動かす | 新規(ゲームの当たり判定を参考) |
| P5 | **Query** | 両方 | 物理への問い合わせを可視化。`Raycast`(線と当たった点)、`OverlapSphere` / `OverlapBox`(範囲に入っている物を光らせる)、タグで絞る | `Physics::Raycast`、`RaycastHit`、`OverlapSphere`、`OverlapBox`、`QueryTriggers` | マウスで線の向き・範囲を動かす / 1〜3: 問い合わせを選ぶ | 新規 |
| P6 | **TerrainCollider** | 3D | 丘を歩いて登る(地形の高さフィールド)。足元の高さを表示。地形の外へ落ちたら戻す | `TerrainRigidbodyComponent`、`TerrainRenderer::GetHeightAt`、`RigidbodyComponent::SetVelocity` | WASD: 歩く / R: スタートへ | Sample3D の `TerrainDemoScene`(歩く部分)、`DemoWalker` |

(Physics の件数: 両方 = P1〜P5 の 5、3D = P6 の 1。計 6。)

### 6.8 Behavior(動きとAI)

| No | デモ名 | 印 | 見せる内容 | 使う Engine の API | 操作 | 流用元 |
|---|---|---|---|---|---|---|
| B1 | **Move** | 両方 | 動きの部品。`MoveComponent`(目的地へ)、`ChaseComponent`(追いかけ)、止まる距離、速さ。2D は `MovePlane::XY`、3D は `XZ` | `MoveComponent` `ChaseComponent`(`SetTarget` `SetSpeed` `SetStopDistance` `SetPlane`)、`MovePlane` | マウス: 目標を動かす / ↑↓: 速さ | 新規(ゲームの `EnemyManager` を参考) |
| B2 | **Homing** | 両方 | `AccelerateChaseComponent`(だんだん速く)と `HomingComponent`(曲がりながら追う)を `ChaseComponent` と並べて比べる | `AccelerateChaseComponent` `HomingComponent` | マウス: 目標 / 1〜3: 動きを切替 | 新規 |
| B3 | **Vision** | 両方 | 視野(扇形)の中・壁の向こう。見える/見えないで色が変わる。視野の角度・距離・目の高さ | `VisionComponent`(`SetFieldOfView` `SetRange` `SetPlane` `SetEyeOffset` `SetObstacleTag` `IsInCone` `IsBlocked`)、`SetDebugDrawEnabled` | 矢印: 目標を動かす / ←→: 見る向き / ↑↓: 視野角 | 新規(ゲームの `Guard` / `GuardController` を参考) |
| B4 | **Path** | 両方 | `NavGrid` を作り、壁を避けて目的地へ歩く(`PathFollowComponent`)。通れないマスと経路の表示 | `NavGridComponent`(`NavGridSettings` `GetGrid` `IsBaked`)、`PathFollowComponent`(`SetNavigation` `SetRepathInterval` `HasPath`)、`NavGridComponent::SetDebugDrawEnabled` `PathFollowComponent::SetDebugDrawEnabled` | クリック: 目的地 / F1: 格子の表示 | 新規(ゲームの `Navigation` / `EnemyManager` を参考) |
| B5 | **MovingWall** | 両方 | 動く壁・扉で経路が変わる(自動で焼き直し)。`SetBlockedBox`(池・柵)、`SetAutoRebakeInterval` | `NavGrid::SetBlocked` `SetBlockedBox`、`NavGridComponent::SetAutoRebakeInterval` `GetLastBakeMilliseconds`、`GetVersion` | 1: 扉を開閉 / クリック: 柵を置く | 新規 |
| B6 | **TerrainPath** | 3D | 丘の高さ・坂・段差を取り込んだ格子。急な坂を避けて歩く | `NavGridSettings`(高さ・坂の項目)、`NavGridComponent::SetTerrain`、`TerrainRenderer` | クリック: 目的地 / 右ドラッグ: 視点 | 新規(Sample3D の `Navigation` を参考) |

(Behavior の件数: 両方 = B1〜B5 の 5、3D = B6 の 1。計 6。)

### 6.9 Audio(音)

| No | デモ名 | 印 | 見せる内容 | 使う Engine の API | 操作 | 流用元 |
|---|---|---|---|---|---|---|
| A1 | **Sound** | 共通 | BGM(ループ)と SE(1回)。BGM/SE の音量グループをスライダーで変える。**フェードは Engine に専用の関数が無い**ので、`TweenValue` で音量を動かす例として見せる | `Audio::PlayOneShot`、`AudioSourceComponent`(`Play` `Stop` `SetVolume` `SetLoop` `SetGroup`)、`Audio::SetGroupVolume` `GetGroupVolume`、`AudioGroup::BGM` `SE`、`TweenValue` | 1: BGM / 2: SE / スライダー: 音量 / 3: BGM をフェードアウト | 新規(ゲームの `VolumeSlider` を参考。`TitleBGM.wav` `shot.wav` `hit.wav`) |
| A2 | **Voice** | 共通 | 直接の再生口(`LoadWave` `StartVoice` `UpdateVoice` `StopVoice`)とリソース(`Resources::LoadSound`)。同時に鳴らせる数 | `Audio::LoadWave` `StartVoice` `UpdateVoice` `StopVoice` `IsVoicePlaying`、`Resources::LoadSound` | 1〜3: 音を足す | 新規(Engine を触る人向けの高度なデモ。優先度は低い) |
| A3 | **Sound3D** | 3D | 位置で聞こえ方が変わる音。`AudioSourceComponent`(`SetSpatial` `SetDistance`)と `AudioListenerComponent`(カメラに付ける)。音の源の物が回ると、左右から聞こえる | `AudioSourceComponent`(`SetSpatial` `SetDistance`)、`AudioListenerComponent`、`Audio::SetListener` `PlayOneShotAt` | WASD: 動く(聞く側) / 右ドラッグ: 向き | 新規 |

(Audio の件数: A1・A3 の 2 件。A2 Voice は作らないことにした(Batch 6 の節)。A3 は、2D でも `AudioListenerComponent` が使えたので「両方」にした。未確定事項 9番。)

### 6.10 Data(データ)

| No | デモ名 | 印 | 見せる内容 | 使う Engine の API | 操作 | 流用元 |
|---|---|---|---|---|---|---|
| T1 | **Csv** | 共通 | CSV を読んで画面に表。ファイルを Excel/エディタで保存し直すと、**実行中に反映される**(ホットリロード)。数値・文字・列の有無 | `Csv::Load` `IsFileUpdated` `Reload` `GetInt` `GetFloat` `GetString` `FindRow` `HasColumn` | ファイル(`Assets/Data/*.csv`)を編集して保存 | 新規(ゲームの `EnemyManager` の `Enemies.csv` / `Waves.csv` を参考) |
| T2 | **Save** | 共通 | セーブデータ(`SaveData`)。整数・小数・文字(日本語)を書いて、アプリを閉じて開き直しても残る。スロットを変えて保存/読込 | `SaveData::SetInt/GetInt` `SetFloat/GetFloat` `SetString/GetString` `HasKey` `DeleteKey` `Save` `Load` | ボタン: 回数を増やす/保存/読込/消す | 新規 |

(Data の件数: 共通 = T1, T2 の 2。計 2。)

### 6.11 件数の確認
各表の印を数え直した結果です(6.2の表と同じ)。

| カテゴリ | 件数 | 両方 | 3D | 共通 |
|---|---|---|---|---|
| Core | 8 | 3 | 0 | 5 |
| Render | 8 | 6 | 0 | 2 |
| UI | 4 | 1 | 0 | 3 |
| Render3D | 10 | 1 | 9 | 0 |
| Physics | 6 | 5 | 1 | 0 |
| Behavior | 6 | 5 | 1 | 0 |
| Audio | 2 | 1 | 0 | 1 |
| Data | 2 | 0 | 0 | 2 |
| **合計** | **46** | **22** | **11** | **13** |

シーン数(クラスの数): 両方 22 x 2 + 3D 11 + 共通 13 = **68 シーン**(+ `DemoMenuScene` と `DemoCategoryScene`)。

### 6.12 Engine の機能の網羅表(README の節 → デモ)
README §1〜§37 の機能が、どのデモで見られるかの対応表です。**漏れが無いか**を確かめるために作りました。

| README の節 | 機能 | デモ |
|---|---|---|
| §1 GameObject と Component | 付け外し・有効/無効 | C1 |
| §2 工場関数 | 物を組み立てる書き方 | (デモのコードそのもの。説明は解説 HTML) |
| §3 Transform | 位置・回転・大きさ・親子 | C2 |
| §4 SpriteRenderer / MeshRenderer / ライティング | 2D/3D の描画 | R1, D1, D2 |
| §5 Camera / CameraController | 2D/3D のカメラ | R3 |
| §6 Collider | 当たり判定・枠線 | P1 |
| §7 EventBus | 通知 | C7 |
| §8 ObjectPool | 使い回し | C6 |
| §9 計算系(Math) | 数学 | C5 |
| §10 Time / Timer | 時間 | C3 |
| §11 Resources | 画像の共有 | C8 |
| §12 Ease / TweenComponent | 補間 | R7 |
| §13 移動(MoveComponent ほか) | 動きの部品 | B1, B2 |
| §14 CameraFollow / CameraShake / 座標の変換 | カメラ・座標 | R3, R8, U4 |
| §15 ScreenEffect | 画面エフェクト | R4 |
| §16 SceneTransition | 切り替え演出 | R5 |
| §17 SpriteAnimationComponent | 2Dアニメ | R2 |
| §18 SaveData | セーブ | T2 |
| §19 Text / TextRenderer | 文字 | U2 |
| §20 UI(Anchor / DrawRect / Gauge / Slider / Button) | UI | U1, U3, U4 |
| §21 開発時の注意点 | (ルール) | (解説 HTML) |
| §23 PhysX(Rigidbody / Raycast / Overlap / AddForce) | 物理 | P1〜P5 |
| §24 Audio / AudioSource / Listener / グループ音量 | 音 | A1, A2, A3 |
| §25 CSV | データ | T1 |
| §26 パーティクル | パーティクル | R6 |
| §27 FBX モデル / アニメ | モデル | D7, D8 |
| §28 Visibility / VisionComponent | 画面外判定・視野 | D10, B3 |
| §29 カプセルコライダー | カプセル | P1 |
| §30 Material | マテリアル | D1 |
| §31 LightComponent | ライト | D2 |
| §32 Shadow | 影 | D3 |
| §33 枠線とリム発光 | 枠線・リム | D4 |
| §34 Terrain / TerrainRigidbody | 地形 | D5, P6 |
| §35 WaterRenderer | 水面 | D6 |
| §36 NavGrid / PathFollowComponent | 経路探索 | B4, B5, B6 |
| §37 Lod / RenderStats | LOD・描画数 | D9 |
| Engine/Core の `Input`(キー・マウス・パッド) | 入力 | C4 |

(§22 は「サンプル構成」、§38 は今のデモの説明なので対象外。Engine の11フォルダ=Core, Render, Render3D, Physics, Audio, UI, Animation, Transition, Behavior, Data, ThirdParty の機能は、すべて上のデモに入っています。`ThirdParty/WICTextureLoader11` は画像の読み込みの内部部品なので、デモは作らず R1 の説明に一言書きます。)

### 6.13 既存の6デモの移行先と、2D/3D 版の追加
| 今のデモ(場所) | 移行先 | 追加する版 |
|---|---|---|
| **Tween**(Sample2D) | R7 Tween の 2D 版(`TweenDemo2DScene`) | **3D 版を新規**(`TweenDemo3DScene`。立方体が奥へ動く) |
| **Particle**(Sample2D) | R6 Particle の 2D 版(`ParticleDemo2DScene`) | **3D 版を新規**(`ParticleDemo3DScene`。`use3D`) |
| **UI**(Sample2D) | U1 UI(共通。`UiDemoScene`)。Anchor の部分を U3 に分ける | (UI は画面の上に描くので 2D/3D の区別なし) |
| **Light**(Sample3D) | D2 Light(`LightDemoScene`)。`MaterialShowcase` を D1 Shapes に分ける | 3D のみ(2D版は作らない。2D にライトは無い) |
| **Shadow**(Sample3D) | D3 Shadow(`ShadowDemoScene`) | 3D のみ |
| **Terrain**(Sample3D) | D5 Terrain + P6 TerrainCollider + D6 Water の3つに分ける | 3D のみ |

---

## 7. 移行の手順

### 7.1 ファイルの移し方(移行元 → Demo)
**ファイルをコピーして、Demo で動くのを確かめてから、サンプル側を消す**順番にします(途中でどちらかが壊れた状態にしないため)。移した後の中身の変更は、次の点だけです。

| 項目 | やること |
|---|---|
| 置き場所 | 6.13 の表のとおり。`Game/Scenes/Demo/○○` → `Demo/Game/Scenes/<カテゴリ>/○○` のように、**カテゴリのフォルダ**へ。共通の物(`DemoRegistry` `DemoMenuScene` `DemoGuide` `DemoButton` `DemoMenu` `DemoGuideController` `DemoCamera*` `DemoLights` `DemoWalker*`)は `Common/`。 |
| 名前空間 | 使っていない(全部グローバル)ので、そのまま。 |
| include の `../` の数 | 元は `Game/Scenes/Demo/○○.cpp` から Engine へ `../../../../Engine/...`(4つ上)。**Demo の `Game/Scenes/<カテゴリ>/○○.cpp` も同じ深さ(カテゴリのフォルダ1つ)なので、Engine への include は書き換え不要**。ただし**同じ階層のファイル同士**(`"DemoRegistry.h"`)と、**別のフォルダのファイル**(`"../../Objects/Demo/DemoGuide.h"` → `"../../Objects/Common/DemoGuide.h"`)は書き換える。 |
| ゲーム側の物 | 元が `../../Objects/Skybox.h` などゲーム側のファイルを使っている所は、Demo に `DemoSkybox` `DemoGround` `DemoHills` `DemoWater` としてコピーして、そちらを使う(`Create○○` の名前も付け替える)。 |
| Sample2D と Sample3D の同名ファイル | `DemoRegistry` `DemoMenuScene` `DemoGuide` `DemoButton` `DemoMenu` `DemoGuideController` は **1つにまとめる**(4.3 の新しい `DemoInfo` に合わせて書き直す)。`DemoEntryButton` は**移さない**(Demo には入口のボタンは要らない)。 |
| 各デモ | `...DemoScene` を `...Demo2DScene` / `...Demo3DScene` に改名(両方ある機能のみ)。`DemoRegistry` に、**2D版・3D版の関数のペア**で1行ずつ登録する。 |
| `ResetSettings` | 2D版と3D版の和集合に、2章の「追加」の項目を足した1つにする。 |
| README | 移行が終わったら README を更新する(10章)。 |

### 7.2 サンプルから消す物
Demo で動くことを確かめた**後**に、次を消します(Batch 0 の最後)。

1. `Sample2D/Game/Scenes/Demo/`、`Sample2D/Game/Objects/Demo/`、`Sample2D/Game/Components/Demo/` の全ファイル(フォルダごと)
2. `Sample3D/Game/Scenes/Demo/`、`Sample3D/Game/Objects/Demo/`、`Sample3D/Game/Components/Demo/` の全ファイル(フォルダごと)
3. `Sample2D/Game/Scenes/TitleScene.cpp` と `Sample3D/Game/Scenes/TitleScene.cpp` の **`#include "../Objects/Demo/DemoEntryButton.h"` と `CreateDemoEntryButton(*this);` の2行**(include と呼び出し。CLAUDE.md の「1行」は呼び出しの行。include も一緒に消す)
4. `Sample2D.vcxproj` / `Sample2D.vcxproj.filters`、`Sample3D.vcxproj` / `Sample3D.vcxproj.filters` の、`Demo` 関係のエントリ(`ClCompile` / `ClInclude` と、`Game\Scenes\Demo` などのフィルター定義)
5. `README.md` の **§38「デモ」の節**(Demo プロジェクトの説明に置き換える。10章)と、§22 などの「Demo」への言及
6. `x64\Debug` `x64\Release` の中の、古い Demo の `.obj`(ビルドで生成される物で、`.gitignore` 対象なら触らなくてよい)

**消してはいけない物**: `CreateSkybox` `CreateGround` `CreateHills` `CreateWater` などのゲーム側のオブジェクト(`Game/Objects/Skybox` など)。デモが使っていても、**ゲーム本体も使う**ので、そのまま残す(Demo にはコピーして持つ)。

### 7.3 サンプルが元どおり動くことの確認
1. `.claude/scripts/check-files.ps1 -FixBom`: 「vcxprojにあるがファイルが無い」「未登録」が出ないこと。
2. `.claude/scripts/build.ps1`: **Debug・Release の両方で、Sample2D・Sample3D・Demo・Engine がエラー・警告なしでビルドできる**こと。
3. サンプルの見た目の確認(**ユーザーにお願いする**): タイトル画面に **DEMO ボタンが無い**こと、`TitleScene` → `GameScene` のゲームが今までどおり動くこと。
4. `git diff` で、サンプル側の変更が「Demo 関連の削除」だけであること(ゲーム側の変更が混ざっていない)。

---

## 8. 段階分け(バッチ)

1段階ごとに「implementer → reviewer → ユーザーが遊んで確認 → コミット」を回します。

| 段階 | 内容 | デモ数 | 規模 | 理由 |
|---|---|---|---|---|
| **Batch 0 土台(実装済み)** | Demo プロジェクト作成(`.vcxproj` `.filters` `main.cpp`、`.sln` に追加)。`DemoRegistry`(カテゴリ+2D/3D の `DemoInfo`)、`DemoMenuScene`(カテゴリ)、`DemoCategoryScene`(機能)、`DemoGuide`(説明板・Back・**2D⇄3D の Tab とボタン**・`ResetSettings` の和集合)、共通部品(`DemoButton` `DemoMenu` `DemoModeButton` `DemoCamera` `DemoLights` `DemoWalker` `DemoSkybox` `DemoGround`)。**既存6デモ(Tween・Particle・UI・Light・Shadow・Terrain)を移行**し、確認後にサンプルから消す(7章) | 8(R6 Particle・R7 Tween の 2D 版、U1 UI、D2 Light、D3 Shadow、D5 Terrain、D6 Water、P6 TerrainCollider) | 大 | すべての土台。一番リスクが大きい(新しいプロジェクト・移行・サンプルの削除)ので、最初に確実に作る。2D⇄3D の切り替えの仕組みは、ここで動くことを確かめる(2D 版しか無いデモでは「2D only」の表示と、暗いボタンの動きを確認) |
| **Batch 1 基本とUI** | Core 全8 + U2 Text・U3 Rect・U4 WorldUI。**同時に R6 Particle・R7 Tween の 3D 版を足す**(機能の数は増えない) | 11 | 中 | 新しい物理や3D描画に頼らず、使い方が簡単で、デモの書き方(`Create○○`/`Controller`)をチームがつかむ練習になる。Particle・Tween の 3D 版で、「両方」のデモの作り方の型を決める |
| **Batch 2 描画と演出** | R1 Sprite・R2 SpriteAnim・R3 Camera・R4 ScreenEffect・R5 Transition・R8 Coord | 6 | 中 | 2D/3D 両対応の機能が多い。切り替えが一番生きる部分 |
| **Batch 3 3D描画** | Render3D の残り(D1 Shapes・D4 Outline・D7 Model・D8 SkinAnim・D9 LOD・D10 Visibility) | 6 | 大 | 3Dの見た目。モデルや LOD は設定が多い。D2/D3/D5/D6 は Batch 0 で済み |
| **Batch 4 物理** | Physics 全6(P6 は Batch 0 で済み) | 5 | 中 | 2D/3D 両対応が多く、重力を切る・Z を固定する 2D 版の書き方を使う。物理の問い合わせ(Raycast など)の可視化は、デバッグ用の線を使うので慎重に |
| **Batch 5 動きとAI** | Behavior 全6 | 6 | 大 | 経路探索は格子(NavGrid)・壁の用意が要る。物理(Batch 4)と、地形(Batch 0)の上に作る |
| **Batch 6 音とデータ** | Audio 全3 + Data 全2 | 5 | 小〜中 | 画面でなく音とファイルが主役で、確認(耳で聞く・ファイルを編集する)に手間がかかる。独立しているので最後 |

### 実装の状態

| 段階 | 状態 | 備考 |
|---|---|---|
| Batch 0 土台 | **実装済み** | 下の「Batch 0 で決めたこと・作った物」。ビルド(Debug/Release)は成功。ゲームの操作確認はユーザーに依頼 |
| Batch 1 基本とUI | **実装済み** | 下の「Batch 1 で決めたこと・作った物」。ビルド(Debug/Release)は成功。ゲームの操作確認はユーザーに依頼 |
| Batch 2 描画と演出 | **実装済み** | 下の「Batch 2 で決めたこと・作った物」。ビルド(Debug/Release)は成功。ゲームの操作確認はユーザーに依頼 |
| Batch 3 3D描画 | **実装済み** | 下の「Batch 3 で決めたこと・作った物」。ビルド(Debug/Release)は成功。ゲームの操作確認はユーザーに依頼 |
| Batch 4 物理 | **実装済み** | 下の「Batch 4 で決めたこと・作った物」。ビルド(Debug/Release)は成功。ゲームの操作確認はユーザーに依頼 |
| Batch 5 動きとAI | **実装済み** | 下の「Batch 5 で決めたこと・作った物」。ビルド(Debug/Release)は成功。ゲームの操作確認はユーザーに依頼 |
| Batch 6 音とデータ | **実装済み**(A2 Voice は作らない) | 下の「Batch 6 で決めたこと・作った物」。ビルド(Debug/Release)は成功。ゲームの操作確認(耳で聞く・ファイルを編集する)はユーザーに依頼 |

デモ単位の状態(Batch 0 の8機能・Batch 1 の11機能・Batch 2 の6機能・Batch 3 の6機能・Batch 4 の5機能・Batch 5 の6機能・Batch 6 の4機能が実装済み。合計 46 機能で、設計の47機能のうち A2 Voice だけを作らない):

| デモ | 状態 | 場所 |
|---|---|---|
| R1 Sprite(2D/3D) | 実装済み(Batch 2) | `Scenes/Render/SpriteDemo2DScene` `SpriteDemo3DScene` |
| R2 SpriteAnim(2D/3D) | 実装済み(Batch 2) | `Scenes/Render/SpriteAnimDemo2DScene` `SpriteAnimDemo3DScene` |
| R3 Camera(2D/3D) | 実装済み(Batch 2) | `Scenes/Render/CameraDemo2DScene` `CameraDemo3DScene` |
| R4 ScreenEffect(共通) | 実装済み(Batch 2) | `Scenes/Render/ScreenEffectDemoScene` |
| R5 Transition(共通) | 実装済み(Batch 2) | `Scenes/Render/TransitionDemoScene`(と、行き来する先の `TransitionDemoBScene`) |
| R6 Particle(2D/3D) | 実装済み | `Scenes/Render/ParticleDemo2DScene` `ParticleDemo3DScene` |
| R7 Tween(2D/3D) | 実装済み | `Scenes/Render/TweenDemo2DScene` `TweenDemo3DScene` |
| R8 Coord(2D/3D) | 実装済み(Batch 2) | `Scenes/Render/CoordDemo2DScene` `CoordDemo3DScene` |
| U1 UI(共通) | 実装済み(Anchor の部分は Batch 1 で U3 Rect へ分けた) | `Scenes/UI/UiDemoScene` |
| C1 Object(2D/3D) | 実装済み(Batch 1) | `Scenes/Core/ObjectDemo2DScene` `ObjectDemo3DScene` |
| C2 Transform(2D/3D) | 実装済み(Batch 1) | `Scenes/Core/TransformDemo2DScene` `TransformDemo3DScene` |
| C3 Time(共通) | 実装済み(Batch 1) | `Scenes/Core/TimeDemoScene` |
| C4 Input(共通) | 実装済み(Batch 1) | `Scenes/Core/InputDemoScene` |
| C5 Math(2D/3D) | 実装済み(Batch 1) | `Scenes/Core/MathDemo2DScene` `MathDemo3DScene` |
| C6 Pool(共通) | 実装済み(Batch 1) | `Scenes/Core/PoolDemoScene` |
| C7 EventBus(共通) | 実装済み(Batch 1) | `Scenes/Core/EventBusDemoScene` |
| C8 Resources(共通) | 実装済み(Batch 1) | `Scenes/Core/ResourcesDemoScene` |
| U2 Text(共通) | 実装済み(Batch 1) | `Scenes/UI/TextDemoScene` |
| U3 Rect(共通) | 実装済み(Batch 1) | `Scenes/UI/RectDemoScene` |
| U4 WorldUI(2D/3D) | 実装済み(Batch 1) | `Scenes/UI/WorldUiDemo2DScene` `WorldUiDemo3DScene` |
| D1 Shapes(3D) | 実装済み(Batch 3。`MaterialShowcase` をここへ分けた) | `Scenes/Render3D/ShapesDemoScene` |
| D2 Light(3D) | 実装済み(Batch 3 で、マテリアルの見本を Shapes へ分け、光に絞った) | `Scenes/Render3D/LightDemoScene` |
| D3 Shadow(3D) | 実装済み | `Scenes/Render3D/ShadowDemoScene` |
| D4 Outline(3D) | 実装済み(Batch 3) | `Scenes/Render3D/OutlineDemoScene` |
| D5 Terrain(3D) | 実装済み | `Scenes/Render3D/TerrainDemoScene` |
| D6 Water(3D) | 実装済み | `Scenes/Render3D/WaterDemoScene` |
| D7 Model(3D) | 実装済み(Batch 3) | `Scenes/Render3D/ModelDemoScene` |
| D8 SkinAnim(3D) | 実装済み(Batch 3) | `Scenes/Render3D/SkinAnimDemoScene` |
| D9 LOD(3D) | 実装済み(Batch 3) | `Scenes/Render3D/LodDemoScene` |
| D10 Visibility(2D/3D) | 実装済み(Batch 3) | `Scenes/Render3D/VisibilityDemo2DScene` `VisibilityDemo3DScene` |
| P1 Collider(2D/3D) | 実装済み(Batch 4) | `Scenes/Physics/ColliderDemo2DScene` `ColliderDemo3DScene` |
| P2 Force(2D/3D) | 実装済み(Batch 4) | `Scenes/Physics/ForceDemo2DScene` `ForceDemo3DScene` |
| P3 RigidbodySettings(2D/3D) | 実装済み(Batch 4) | `Scenes/Physics/RigidbodySettingsDemo2DScene` `RigidbodySettingsDemo3DScene` |
| P4 Trigger(2D/3D) | 実装済み(Batch 4) | `Scenes/Physics/TriggerDemo2DScene` `TriggerDemo3DScene` |
| P5 Query(2D/3D) | 実装済み(Batch 4) | `Scenes/Physics/QueryDemo2DScene` `QueryDemo3DScene` |
| P6 TerrainCollider(3D) | 実装済み | `Scenes/Physics/TerrainColliderDemoScene` |
| B1 Move(2D/3D) | 実装済み(Batch 5) | `Scenes/Behavior/MoveDemo2DScene` `MoveDemo3DScene` |
| B2 Homing(2D/3D) | 実装済み(Batch 5) | `Scenes/Behavior/HomingDemo2DScene` `HomingDemo3DScene` |
| B3 Vision(2D/3D) | 実装済み(Batch 5) | `Scenes/Behavior/VisionDemo2DScene` `VisionDemo3DScene` |
| B4 Path(2D/3D) | 実装済み(Batch 5) | `Scenes/Behavior/PathDemo2DScene` `PathDemo3DScene` |
| B5 MovingWall(2D/3D) | 実装済み(Batch 5) | `Scenes/Behavior/MovingWallDemo2DScene` `MovingWallDemo3DScene` |
| B6 TerrainPath(3D) | 実装済み(Batch 5) | `Scenes/Behavior/TerrainPathDemoScene` |
| A1 Sound(共通) | 実装済み(Batch 6) | `Scenes/Audio/SoundDemoScene` |
| A2 Voice(共通) | **作らない**(Batch 6 で決めた) | 理由は「Batch 6 で決めたこと・作った物」 |
| A3 Sound3D(2D/3D) | 実装済み(Batch 6。2D版も作れた) | `Scenes/Audio/Sound3DDemo2DScene` `Sound3DDemo3DScene` |
| T1 Csv(共通) | 実装済み(Batch 6) | `Scenes/Data/CsvDemoScene` |
| T2 Save(共通) | 実装済み(Batch 6) | `Scenes/Data/SaveDemoScene` |

### Batch 0 で決めたこと・作った物

**ユーザーが決めたこと**(9.3 の未確定事項):
- 1 入口: 起動してすぐカテゴリ一覧を出す(タイトル画面は作らない)。
- 2 カテゴリ: 8個(Core / Render / UI / Render3D / Physics / Behavior / Audio / Data)。
- 3 2D/3D の選択: 全体で1つ覚える。
- 4 起動時の側: 3D。
- 10 機能の数: 47機能で進める。
- 12 起動プロジェクト: 変えない(`.sln` に Demo を足しただけ。プロジェクトの順序・起動プロジェクトの設定は触っていない)。

**設計書の案(おすすめ)のとおりにしたこと**:
- 5 今の側は `SaveData` に保存しない(アプリを閉じると3Dに戻る)。
- 6 前/次のデモのボタンは後回し(作っていない)。
- 7 「共通」の機能の切り替えボタンは、押せない状態(暗く)で表示する(文字は `Common`)。
- 8 一番上のカテゴリ一覧で Esc を押すと、確認なしでアプリを終了する(ボタンは `Quit`)。
- 15 Demo の Release は `WholeProgramOptimization` を切った(リンクを速くするため。サンプルは `true` のまま)。
- 13 `.claude/scripts/check-files.ps1` の対象に `Demo` を足した(`build.ps1` は `.sln` 全体をビルドするので変更不要)。
- 9, 11, 14, 16 は Batch 0 では関係しなかったので、そのまま(未確定)。

**設計書に案が無く、安全側に決めたこと**:
- 現在は全カテゴリにデモがあり、準備中のカテゴリは無い(0件のカテゴリがあれば、カテゴリ一覧で暗くして押せなくし、説明に「(準備中)」と出す仕組みは残してある)。
- カテゴリ一覧と機能一覧でも、Tab と右下のボタンで側(2D/3D)を選べる(`Mode: 3D` / `Mode: 2D`)。画面は切り替えず、機能一覧の暗いボタンだけが変わる。
- 2D/3D の切り替え(`SwitchDemoMode`)は、暗転中(`Time::IsPaused()`)は受け付けない。
- 3D の Particle は、画面に対して小さく見えるので、粒の速さ・重力・大きさを3倍にして `use3D = true` で出す。クリックは、マウスの線(`ScreenPointToRay3D`)が地面(y=-50)に当たる場所。
- 3D の Tween は、8個の立方体が手前から奥(Z+)へ動く(位置・大きさ・向き(Y軸)・色・進み具合を同時に Tween)。
- Terrain は3つに分けた: D5 Terrain は「矢印キーで動かす黄色い玉の下の高さ・向き・範囲内かを表示」、D6 Water は「池の水面(色・波)と浮かぶ箱」、P6 TerrainCollider は「歩く箱で丘を登る」。D5 Terrain の「作り方の切り替え(CSV/プログラム)」は、この段階では作っていない。
- サンプルの `Skybox` `Ground` `Hills` `Water` `Background` は、Demo に `DemoSkybox` `DemoGround` `DemoHills` `DemoWater` `DemoBackground` としてコピーして持つ(ゲーム側の元ファイルは残している)。`GroundController` `SkyboxFollowComponent` `BackgroundController` は、元と同じ名前でコピーした。
- メニューの部品は、`DemoMenu`(`CreateDemoMenu`。カテゴリ一覧)と `DemoCategoryMenu`(`CreateDemoCategoryMenu`。機能一覧)の2つの工場関数に分け、1行(ボタン+説明)を作る共通の部分は `CreateDemoMenuRow`(`DemoMenu.h`)にした。機能一覧の行の見た目は `DemoCategoryMenuController` が側に合わせて更新する。
- `DemoScreen`(`Top` カテゴリ一覧 / `List` 機能一覧 / `Demo` デモ本体)で、戻る先と Tab の動きを決める。

**作った物**(`3D_FreamWork/Demo/`): `Demo.vcxproj` `Demo.vcxproj.filters` `main.cpp`(ウィンドウは「Engine Demo」、1280x720)と、`Game/Scenes|Objects|Components/` の `Common` `Render` `UI` `Render3D` `Physics`。Engine・シェーダー・`PhysX.props` は変えていない。サンプルからは、`Game/*/Demo/` のフォルダ全部、`TitleScene.cpp` の `DemoEntryButton` の include と呼び出し、`.vcxproj` / `.filters` の Demo のエントリを消した(README の説明は §38 に置き換えた)。

**変更した全体の設定の対象一覧**(`DemoGuideController::ResetSettings`): 2Dカメラ / 3Dカメラ(と自動追従) / `Time::SetTimeScale` / 平行光源 / 影 / LOD / `RigidbodyComponent` `VisionComponent` `NavGridComponent` `PathFollowComponent` の枠線 / `ScreenEffect::Stop` / `CameraShake::Stop` / BGM・SE の音量 / カーソルの固定。

### Batch 1 で決めたこと・作った物

Core 全8(C1〜C8)と UI の U2 Text・U3 Rect・U4 WorldUI を作りました(**Engine は変えていません**)。Particle と Tween の 3D 版は Batch 0 で作成済みです。

**方針(ユーザーが決めたこと)**:
- 「引数 `DemoMode` で 2D/3D を分ける」型(`EaseRunner` の方式)を使う。2D と 3D のシーンが同じ工場関数・Component を共有し、違いは引数で出す(`CreateObjectDemo(scene, mode)` `CreateTransformDemo` `CreateMathDemo` `CreateWorldUiDemo`)。
- 3D の舞台(`CreateDemoSkybox` + `CreateDemoGround` + `CreateDemoCamera`)は、`CreateDemoStage3D(scene, 見る点, 向き, 距離)`(`Objects/Common/`)にまとめた。**`CreateDemoGuide` より後に呼ぶ**(ガイドが `Init` で3Dカメラの設定を戻すため)。Batch 0 のシーンは挙動を変えないよう、そのままにした。
- `CreateDemoGuide` は、設定を変えるオブジェクトより**先に**作る(README の追加手順にも書いた)。

**作ったデモ**(場所は 8章の「デモ単位の状態」の表):

| デモ | 内容 | 後片付け・全体設定 |
|---|---|---|
| C1 Object(2D/3D) | 中央の物に `SpinComponent` / `BobComponent` を付ける・止める。クリックで小さな物を出し、3秒で `Destroy`。`GetComponent` で付いているかを表示 | なし(出した物はSceneが片付ける) |
| C2 Transform(2D/3D) | 親と子。親を動かす・回す・大きさを変える。子のローカル/ワールドの位置を表示。SPACE で親子を切る(切った子はその場に残す) | なし |
| C3 Time(共通) | SPACE スロー(0.3倍)/ P ポーズ。`dt` で回る物と `GetUnscaledDeltaTime` で回る物を比べる。`Timer` を3種類(1回・繰り返し・5秒) | `Uninit` で `Time::SetTimeScale(1)`。切り替え演出中のポーズ(`Time::IsPaused` が真で、デモ自身はポーズしていない間)は、キーを受け付けない |
| C4 Input(共通) | キーボード(Press/Down/Up)・マウス(位置・動き・ワールドの位置・ボタン・十字の印)・ゲームパッド(スティック・トリガー・ボタン)を表示。F1 でカーソル固定 | `Uninit` で `SetCursorLocked(false)`。パッドが無くても動く(「未接続」と表示) |
| C5 Math(2D/3D) | 1〜4 で関数を選ぶ。2D: Lerp / Clamp / 距離・向き・角度 / 乱数。3D: Lerp / 内積・外積 / Normalize / MoveTowards | なし |
| C6 Pool(共通) | 弾をクリック・SPACE で連射。「用意した数・借りた回数・使用中・待機中・作った数」とプールの中身を表示。`ObjectPool` は `PoolDemoController` のメンバ | なし |
| C7 EventBus(共通) | 送る側・受け取る側(スコア)・記録係の3つの管理役。撃破(引数なし)とコイン取得(オブジェクトを渡す)を送る | シーンの `Uninit` で `EventBus::Get().Clear()` |
| C8 Resources(共通) | 同じ画像を増やしても `LoadTexture` のIDは同じ。全部消しても `UnloadUnused` まで残る。音とモデルを2回借りる | `Uninit` で借りた音(`ReleaseSound`)とモデル(`shared_ptr` を手放す)を返す |
| U2 Text(共通) | 大きさ・色・縁取り・太字・寄せ方・フォント。キー入力した文字を `Text::Measure` の板の上に4スタイルで。動く物の `TextRenderer` | なし |
| U3 Rect(共通) | UI デモから分けた Anchor の9か所の印(`GetAnchoredPosition`)と、印に合わせた四角(`GetAnchoredCenter`)。ドラッグできる3つの四角 | なし |
| U4 WorldUI(2D/3D) | 敵3体の頭上に `GaugeRenderer`(`SetFollowTarget`)と名前・HPの板。主人公が近いと板が赤い。3D はカメラの後ろで描かない | なし |

**設計書に案が無く、安全側に決めたこと**:
- 設計書の「Core の C5 Math / C7 EventBus」などの番号はそのまま(ユーザーの依頼文にある「C5 のデモ(EventBus)」は C7)。
- **Engine に無いもの**: (1) Component を外す関数が無い → C1 の「外す」は `SetEnabled(false)`(止める)で見せる。(2) `Input` にパッドの接続を調べる関数が無い → C4 は `XInputGetState` を直接 0.5 秒ごとに呼ぶ(Engine の `Input` も毎フレーム呼んでいる)。(3) 線を引く API が無い → C5 は点を並べて線にし、3D も 3D メッシュではなく、`WorldToScreen3D` で画面に直した点の印(`DrawRect`)で描く。(4) `Resources` に読み込み数を調べる関数が無い → C8 は「返ってきたIDが同じ」「モデルが同じ物を指している」で共有を見せる。
- U1 UI デモ: Anchor の9か所の印を U3 Rect に移し、UI の説明文から「アンカー」を外した(`UiDemoController` から `Anchor` の表を削除)。
- 文字の見た目を作る `MakeDemoTextStyle(大きさ, 色, 寄せ方)` を `Components/Common/DemoTextStyle.h/.cpp` に追加(日本語フォント・黒い縁取り。Core/UI の各 Controller が使う)。
- 新しい素材は追加していない(`box.png` `bg_tile.png` `shot.wav` `TestParts.fbx` など、今ある物だけ)。
- C1 / C6 のクリックは、画面の下の Back・2D/3D ボタンの上でも反応する(切り替えるので問題は起きない)。

### Batch 2 で決めたこと・作った物

Render の残り6機能(R1 Sprite・R2 SpriteAnim・R3 Camera・R4 ScreenEffect・R5 Transition・R8 Coord)を作りました(**Engine は変えていません**)。Render カテゴリの機能は、番号の順(R1〜R8)で並びます(8件。一覧の行の間隔は自動で縮み、8件でも画面に収まる)。

**方針(ユーザーが決めたこと)**:
- Batch 0・1 の型を踏襲(2D/3D 両対応は引数 `DemoMode` で分ける。3D の舞台は `CreateDemoStage3D`(`CreateDemoGuide` の後)。日本語の文字は `MakeDemoTextStyle`)。
- 「点と線の印」を描く道具は `Components/Common/DemoMarker.h/.cpp`(`WorldToDemoScreen` / `DrawDemoPoint` / `DrawDemoLine` / `DrawDemoLabel`)に切り出して共有した(Math のデモの挙動は変えていない)。「マウスの線が地面に当たる場所」も `Components/Common/DemoMouseGround.h/.cpp`(`GetMouseGroundPoint`)にまとめ、Particle・Object のデモと新しいデモで使う(挙動は同じ)。
- R3 Camera の3D版は、カーソルを固定しない「右ドラッグで向きを変える」カメラにした(一人称も同じ)。カーソルを固定しないので、固定中のEscの問題は起きない。

**作ったデモ**:

| デモ | 内容 | 後片付け・全体設定 |
|---|---|---|
| R1 Sprite(2D/3D) | 2D: コマ・色・大きさ・回転を変える4つ(1〜4キーで入り切り)と、UI → 2D → 3Dで重なり方と大きさが変わる赤と青の2つ(5キー)。3D: 立方体の周りに、ビルボードで固定する軸の違う4つ(1〜4キーの動きは全部が受ける) | なし |
| R2 SpriteAnim(2D/3D) | `player.png` の4つのクリップ(ループ3つ・1回だけ1つ)と、`SetRandomStart(true)` で始まりをずらした5つ。SPACE で再生・停止、1〜4キーで速さ | なし |
| R3 Camera(2D/3D) | 2D: おばけを `CameraFollowComponent` が追う(1: なめらかさ / 2: デッドゾーン / 3: 範囲 / 4: ずらし / SPACE: `CameraShake`)。3D: 三人称・一人称(V)・右ドラッグ・Q/E・揺れの強さ(1〜3) | `Uninit` で 2Dカメラを原点・揺れを止める・3Dカメラの自動追従を戻す |
| R4 ScreenEffect(共通) | 1: Flash / 2: Vignette / 3・4: Fade(黒・白。暗転し終わったら `onComplete` で明転)。`IsFading()` と完了の回数を表示 | `Uninit` で `Stop` と、暗転の濃さを0へ(`Fade` はシーンをまたいで残るため) |
| R5 Transition(共通) | 1〜4: Fade(黒)・Fade(白)・丸・ワイプ(`SpriteTransition`)を選んで、シーンA(2D)とシーンB(3D)を行き来する。切り替え中の `Time::IsPaused()` と、止まる経過時間・回る箱を表示 | なし(演出は `SceneManager` が自分で終わらせる) |
| R8 Coord(2D/3D) | 2D: マウスの画面の位置 ⇔ ワールドの位置(`ScreenToWorld2D` / `WorldToScreen2D`)、クリックで印、WASD で2Dカメラ、奥行き(z)だけ違う3つ(zが違っても大きさ・位置は変わらない)。3D: 頭上に `WorldToScreen3D` で文字、マウスの下を `ScreenPointToRay3D` + `Raycast` で探してクリックで選ぶ | `Uninit` で 2Dカメラを原点へ |

**設計書に案が無く、安全側に決めたこと**:
- **2Dカメラが動くデモのボタン**: ボタンは `SpriteRenderer`(UIモード)で、2Dカメラが動くと一緒に流れてしまう。そこで `DemoScreenFixController`(`Components/Common/`)を作り、`CreateDemoButton` が全ボタンの先頭に付けるようにした(付けた時の位置を「画面の位置」として覚え、`Update` と `Draw` でカメラの分だけ足す)。カメラが動かない他のデモでは、位置は変わらない。
- **Engine に無いもの**: (1) スプライトの左右反転は無い(`scale` が0以下だと画像の大きさになるため)。R1 では扱わない。(2) `CameraFollowComponent` に範囲を解除する関数が無い → R3 の「範囲 OFF」は、とても広い範囲(±100000)を渡して見せる。
- **R5 Transition**: 行き来する先のシーンB(`TransitionDemoBScene`)は、機能一覧には出さない(`DemoRegistry` には、シーンAだけを登録)。「Esc で戻る」は、どちらのシーンでも機能一覧へ戻る。`transition_circle.png` `transition_wipe.png` は既存の素材を使い、新しい素材は追加していない。
- **R2 SpriteAnim**: 素材は既存の `player.png`(8列x2行)。上の段のクリップ名は `Walk`(0,1)・`Scared`(8,9)・`Eyes`(12〜15。1回だけ)・`All`(0〜7)。
- R3 の3D版は、2Dカメラの代わりに3Dカメラを直接動かす。移動はWASD(矢印)で、向いている方へ動く。

### Batch 3 で決めたこと・作った物

Render3D の残り6機能(D1 Shapes・D4 Outline・D7 Model・D8 SkinAnim・D9 LOD・D10 Visibility)を作りました(**Engine は変えていません**。新しい素材も足していません。FBX は既存の `TestParts.fbx`・`TestAnim.fbx`・`TestSkin.fbx`、画像は `grid.png` `box.png`、OBJ は `test_cube.obj`)。Render3D カテゴリの機能は、番号の順(D1〜D10)で10件並びます(Shapes・Light・Shadow・Outline・Terrain・Water・Model・SkinAnim・LOD・Visibility)。一覧の行の間隔は自動で縮み(10件で44)、ボタンの高さ34に30pxの文字が収まる。

**方針**: Batch 0〜2 の型を踏襲(2D/3D 両対応は引数 `DemoMode`、3D の舞台は `CreateDemoStage3D`(`CreateDemoGuide` の後)、日本語は `MakeDemoTextStyle`)。ガイド(`CreateDemoGuide`)は、設定を変えるオブジェクトより先に作る。

**作ったデモ**:

| デモ | 内容 | 後片付け・全体設定 |
|---|---|---|
| D1 Shapes(3D) | 3列の見本。手前=形(立方体・粗い球・細かい球・カプセル・OBJ。回る)、真ん中=球5個(マテリアル)、奥=立方体5個(マテリアル)。1・2・3で列のオン/オフ(`SetActive`)、4で形を回す。名前は `WorldToScreen3D` で上に出す。Light デモにあった `MaterialShowcase`(`MaterialShowcase.h/.cpp`)は削除し、中身を `ShapesDemo` へ移した | なし |
| D2 Light(3D・変更) | `MaterialShowcase` を `LightTargets`(同じ白い材質の球5個と立方体5個)に置き換え、光に絞った。光の動き・操作は変えていない | 今まで通り(`ResetSettings`) |
| D4 Outline(3D) | 立方体の枠線、壁の後ろの球の壁越しの枠線(`seeThrough`。球の `drawPriority` を壁より大きくした)、暗い球の縁の光。1・2・3でオン/オフ、上下で太さ(1〜12)、左右で縁の光の強さ(0〜3) | なし(マテリアルはオブジェクトのもの) |
| D7 Model(3D) | `TestParts.fbx` を2体。1体は `GetNodeTransform("Top")` で回す・上下・大きさ、全体の回転・大きさ(`Transform`)を切り替える。もう1体は見本。パーツ数・親子・位置・半径、「2体のモデルのデータは共有」を表示 | `ModelRenderer` が `Uninit` でモデルを返す。コントローラは `shared_ptr` を持たない(`Draw` の間だけ借りる) |
| D8 SkinAnim(3D) | 箱(`TestAnim.fbx`: Spin/Bob)、ずっと曲がる腕(`TestSkin.fbx`: Bend)、1回だけの腕(`Play("Bend", false)`。`IsFinished` を表示)。1・2: `Play`、SPACE: `CrossFade`(0.6秒)、3: 1回だけの腕を作り直して再生、4: 枠線、上下: 速さ(0〜3) | なし(作り直した古い腕は `Destroy`) |
| D9 LOD(3D) | カプセルの柱15x15本。描く3000・影2000・枠線1500 の距離で省く。1: `Lod::SetEnabled`、上下: `Lod::SetGlobalScale`(0.25〜3)、2: 影、3: 枠線。`RenderStats` の描画命令・三角形(通常・影・枠線・合計)と `GetLodCulledCount` を表示 | `Uninit` で `Lod::SetEnabled(true)` / `SetGlobalScale(1)` / `Shadow::SetSettings(Settings())`(ガイドの `ResetSettings` と二重) |
| D10 Visibility(2D/3D) | 2D: 箱99個(11x9)、3D: 球と立方体81個(9x9)。円/球と四角/箱の両方の判定で映っている数を数える。1: `SetFrustumCulling` のオン/オフ、`GetCulledCount` を表示。右上の地図(緑=映っている、灰色=映っていない、黄=カメラの範囲)。2D: WASD で2Dカメラ、3D: WASD で見る点(向いている方へ) | 2Dカメラの位置を `Uninit` で原点へ |

**設計書に案が無く、安全側に決めたこと**:
- D1: 設計書の「1〜4: 見た目」は、列の表示オン/オフ(1〜3)と形を回す(4)にした(見た目の切り替えは、マテリアルが違う見本を並べて見せるため)。`Mesh::CreateSphere` の粗い/細かいは別の見本にした。
- D4: 設計書の「スライダーで太さ」は、上下キーにした(`SliderComponent` を足すと、3D 側のボタンの配置が増えるため)。
- D7: 設計書の「1〜3: モデルの切替」は、既存の素材が `TestParts.fbx` だけなので、2体のうち1体を動かす作りにした(切り替えは、動きの種類)。
- D8: `TestSkin.fbx` のクリップは Bend だけなので、1回だけ再生する腕(ループなし)を足して `IsFinished` を見せた。
- D10: 2D のスプライトは `SetFrustumCulling` が効く(2Dカメラの画面で判定)。物が少ない画面だと省かれる数が見えにくいので、世界を広く(99個)した。3D のカメラは、見る点を WASD で動かせるようにした(カメラコントローラの `GetYaw` を使う)。
- 全体設定: D9 が `Lod`・`Shadow` を変える(既存の `ResetSettings` が元から戻す対象)。他は新しい全体設定を触らない。`ResetSettings` に足した物はない。
- **Engine に無いもの**: 線を引く API が無いので、D10 の地図は `DrawRect` の点で描いた(Batch 1 と同じ)。

### Batch 4 で決めたこと・作った物

Physics の P1〜P5(Collider・Force・RigidbodySettings・Trigger・Query)を、2D版と3D版の両方で作りました(**Engine は変えていません**。新しい素材も足していません)。Physics カテゴリの機能は、番号の順(P1〜P6)で6件並びます(Collider・Force・RigidbodySettings・Trigger・Query・TerrainCollider。一覧の行の間隔は 440÷6=73 に縮み、ボタンの高さは63)。カテゴリ一覧は8行(Core / Render / UI / Render3D / Physics / Behavior / Audio / Data)のまま、Physics は `[6]` と出る。

**方針**: Batch 0〜3 の型を踏襲(2D/3D 両対応は引数 `DemoMode`、3D の舞台は `CreateDemoStage3D`(`CreateDemoGuide` の後)、日本語は `MakeDemoTextStyle`)。**2Dの剛体付きの物は、共通の `CreateDemoBody2D`(`Objects/Common/DemoBody.h`)で作る**(重力オフ・Z固定・回転固定・2Dモードのスプライト・z=0 を1か所で付ける)。3D版は `CreateDemoBody3D`(同じ設計図 `DemoBodyDesc`)。壁は `DemoWall.h`(`CreateDemoWall2D` / `CreateDemoArena2D` / `CreateDemoWall3D`)。

**作ったデモ**:

| デモ | 内容 | 後片付け・全体設定 |
|---|---|---|
| P1 Collider(2D/3D) | 固定の見本(Static)3つ・往復する台(Kinematic)・クリックで出す動く物(Dynamic)。1・2・3 で形、F1 で枠線、C で全部消す。2D は壁で囲み、勢いをつけて出す。3D は地面の上から落とす。30個まで | `Uninit` で `SetDebugDrawEnabled(false)`(枠線は全体の設定。ガイドの `ResetSettings` でも戻る)。出した物は `Destroy` で消す(剛体は `Uninit` で解放) |
| P2 Force(2D/3D) | 軽い箱(質量100)と重い箱(質量500)を同じ値で押す。1〜4: `Force` `Impulse` `Acceleration` `VelocityChange`、SPACE: かける、V: `SetVelocity`(上書き)、T: `AddTorque`、R: 戻す。Y・Z 固定(3D は重力も切る) | なし(全体の設定を変えない) |
| P3 RigidbodySettings(2D/3D) | 同じ箱 A(ふつう)と B(設定を変えた物)を、同じ力(`Impulse`+`AddTorque`)で蹴る。1: 質量 / 2: 抵抗 / 3: 重力 / 4: 回転固定 / 5: 位置固定(もう一度で軸を変える)。Aと Bの設定を表示 | なし |
| P4 Trigger(2D/3D) | 動かせる球を、トリガー(黄色い丸)と箱(青)に近づけて、`OnTriggerEnter/Stay/Exit2D` と `OnCollisionEnter/Stay/Exit2D` の呼ばれた回数を表示(数える役は `TouchCounterController`)。球は `SetVelocity` で動かす(ゆっくり) | なし |
| P5 Query(2D/3D) | Raycast(出発点からマウスへ。当たった物・距離・場所・面の向き)、OverlapSphere / OverlapBox(マウスの周り。見つかった物を光らせる)。4: タグの絞り込み(なし・Target・Decoy)、5: トリガーも探すか(`QueryTriggers`)。調べられる物は動かない(Static) | なし(3Dは `SetOutline` を物に付けるだけ) |

**設計書に案が無く、安全側に決めたこと**:
- **2D の剛体の見た目**: 素材を足さない決まりなので、円とカプセルは `box.png`(3x3の丸い絵)の1コマ、四角と壁は `button.png`(青い四角)にした。**塗りつぶした円・四角の素材は無い**(`debug_circle.png` は細い輪、`grid.png` は線)。
- **2D で動く物を出した直後は、`SetVelocity` が消える**: 剛体は最初の `Update` で位置を合わせ直して速さを0にする(Engineの決まり。`justActivated`)。P1 は、2フレーム待ってから速さを入れる(スクラッチのテストで確認)。
- **P2**: 力の大きさは、軽い箱が「加速度1500 / 速さ350」になる値を、2つの箱に同じ値で渡す(`Force`・`Impulse` は質量をかけた値)。箱は Y・Z を固定し、重力を切る(3D版も。力の違いだけを見せるため)。空気抵抗 0.6 で、押すのをやめるとだんだん止まる。`AddTorque` は、軽い箱が約6rad/sで回る大きさを、2つに同じ値で渡す(重い箱は約0.4)。
- **P3**: 設計書の「1〜5: 設定を切替」は、「Bに変える設定を選ぶ」にした(Aはいつも既定)。2D は重力が無い世界なので、重力の比べ方は「Bだけ重力を入れる」、3D は「Bだけ重力を切る」。蹴る速さは、2D=400、3D=800(3D は地面の摩擦で止まるため)。位置固定は、2D は X・Y、3D は X・Y・Z(もう一度5で切り替え)。
- **P4**: 球の速さを 180 にした。速く(約200以上)押し付けると、PhysX の跳ね返り(`bounceThreshold` 約196)で `OnCollisionEnter` が何回も呼ばれるため。2D・3D とも `OnTriggerEnter2D` のように **Engine の名前は `○○2D`**。球は Dynamic(Kinematic にすると、静止した箱との当たりが検出されない。下の「Engineに無いもの」)。Collision の `Stay` は、動いている間だけ数える(止まって眠ると呼ばれない)。
- **トリガーの見た目**: 半透明の黄色(濃さ0.6。最初は0.4・0.35で、暗い背景では見えなかったため上げた。球が中にいる間は0.95)。2D は box.png の黄色い丸の1コマ、3D は黄色い半透明の球。箱は青(2D は button.png、3D は青い箱)で、トリガー=黄色・すり抜ける、箱=青・ぶつかるが2D/3Dで揃う。
- **P5**: 3D の線・範囲は、物の真ん中の高さ(y=0)の水平な面を、マウスの下に見つけて使う。3D でタグを指定しないと、地面(`DemoGround`)も見つかる(教材として残した)。2D の Overlap の箱は、奥行きを100にした(物の厚みより大きく)。
- **画面の配置(1280x720で確認)**: 状態の板は説明の板の下(左上)。2D のデモは、板と重ならないように、壁の高さ・物の高さを決めた(例: P1 は壁で囲んだ範囲を画面の右側 x=40〜610 にした)。3D は、見る点を高くして、物が板の下に映るようにした。
- **ファイルの追加**: 全デモが使う `DemoStatus`(状態の板)・`DemoBodyReset`(動く剛体を元の位置へ戻す)を `Components/Common/` に、`DemoBody`・`DemoWall` を `Objects/Common/` に追加した。`DemoMarker` の `DrawDemoLine` に、点の数の引数(省略すると今まで通り)を足した。
- **Engine に無いもの / 制約**: (1) 2D 用の塗りつぶした円・四角の素材が無い(上のとおり)。(2) Kinematic の物は、静止した箱とは当たりが検出されない(スクラッチのテストで、Kinematic の球は箱を通り抜けて、当たりのコールバックが0回だった。PhysX の `eENABLE_KINEMATIC_STATIC_PAIRS` を使っていないためと思われる)。P4 の球を Kinematic にできない理由。(3) 線を引く API が無いので、Query の線・範囲は点を並べて描いた(Batch 1 と同じ)。(4) 2D の `Transform.rotate.z` の向きと、画面の見た目の向きが同じかは、画面で確認できていない(2Dの物は、P2・P3 で回るのは四角と円だけなので、見た目の違いは目立たない)。

### Batch 5 で決めたこと・作った物

Behavior の全6機能(B1 Move・B2 Homing・B3 Vision・B4 Path・B5 MovingWall・B6 TerrainPath)を作りました(**Engine は変えていません**。新しい素材も足していません)。Behavior カテゴリの機能は、番号の順で6件並びます(Move・Homing・Vision・Path・MovingWall・TerrainPath)。カテゴリ一覧は8行(Core / Render / UI / Render3D / Physics / Behavior / Audio / Data)のままで、Behavior が `[6]` と出て押せるようになった(Core `[8]` Render `[8]` UI `[4]` Render3D `[10]` Physics `[6]` Behavior `[6]`。Audio・Data は準備中)。機能一覧の行の間隔は 440÷6=73、ボタンの高さは63(Physics と同じ)で、いちばん下の行も Back・Mode ボタンと重ならない。

**方針**: Batch 0〜4 の型を踏襲(2D/3D 両対応は引数 `DemoMode` で分ける。3D の舞台は `CreateDemoStage3D`(`CreateDemoGuide` の後)。日本語は `MakeDemoTextStyle`)。ガイドは、設定を変えるオブジェクトより先に作る。共通の道具を2つ足した:
- `Components/Common/DemoPlane.h/.cpp`: 動く平面の道具。位置を「平面の点」(2Dは x,y、3Dは x,z)で扱い、`ConvertPlaneToWorld` / `ConvertWorldToPlane` / `GetDemoMousePoint`(2Dはマウスのワールド位置、3Dはマウスの線が地面の上の水平な面に当たる場所)/ `GetDemoPlane`(`MovePlane`)/ `GetDemoField`(標準の競技場。2D は x -600〜600・y -250〜50、3D は x -750〜750・z -300〜350)/ `ClampToDemoField` がある。`DemoMarker` には、円と四角の枠を点で描く `DrawDemoRing` / `DrawDemoRect` を足した。
- `Objects/Behavior/DemoAgent.h/.cpp`: 動く物(`CreateDemoAgent`。円・球。`BodyType` を選ぶ)・目印(`CreateDemoGoal`・`PlaceDemoGoal`)・壁(`CreateDemoObstacle`。**タグ "Wall"**)・競技場(`CreateDemoField`)・経路探索の格子(`CreateDemoNavigation`)を、2Dと3Dで同じ呼び方で作る。2Dの壁は、これまでの `CreateDemoWall2D`(奥行き200の厚い板)、3Dの壁は `CreateDemoWall3D` を使い、タグだけ足している(`CreateDemoWall2D` / `3D` は変えていない)。

**作ったデモ**:

| デモ | 内容 | 後片付け・全体設定 |
|---|---|---|
| B1 Move(2D/3D) | `ChaseComponent` を付けた動く物2つ。青=`SetTarget` でマウスの目印を追う、橙=`SetTargetPosition` でクリックした場所へ向かう。↑↓で速さ(`SetSpeed`)、←→で止まる距離(`SetStopDistance`。目印の周りの円)。途中の壁にぶつかると、まっすぐ進む動きは止まる。`HasArrived` と目印までの距離を表示。動く物は Dynamic(R は `ResetDemoBody`) | なし |
| B2 Homing(2D/3D) | 同じ目標(マウスの黄色い目印)を、青 `ChaseComponent` / 橙 `AccelerateChaseComponent`(加速・最高速度・空気抵抗)/ 緑 `HomingComponent`(曲がる速さ。最初に上へ向いて飛び出す)で追う。通った道(点)と進んでいる向き(白い線)を表示。1・2・3で入り切り、↑↓で緑の曲がる速さ、←→で橙の加速、R で最初の位置へ。3つは Kinematic(お互いにすり抜ける) | なし |
| B3 Vision(2D/3D) | 見張り役(動かない)の `VisionComponent` の視野(扇形)の中に、矢印キーで動かすターゲットがいて、壁にさえぎられていなければ見える(見張り役が赤くなる)。`IsInCone` と `CanSee` を別々に表示。A・Dで向き(`SetFacing`)、W・Sで視野の角度、1・2で距離、SPACEで自動で回る、5で壁のタグを外す、3Dは4で目の高さ(`SetEyeOffset`。壁の高さ150より上へ)、F1で扇形の線 | `Uninit` で `VisionComponent::SetDebugDrawEnabled(false)`(ガイドの `ResetSettings` でも戻る) |
| B4 Path(2D/3D) | 壁の通り道を、緑は `PathFollowComponent` で壁を避けて歩き、赤は `ChaseComponent` でまっすぐ向かって壁に止まる。クリックで目的地。←→で探し直す間隔(`SetRepathInterval`)、↑↓で速さ、F1で格子(通れないマス=赤い線)と経路(緑の線)。格子の大きさ・焼いた時間・経路の点の数を表示 | `Uninit` で `NavGridComponent` と `PathFollowComponent` の `SetDebugDrawEnabled(false)`(`ResetSettings` でも戻る) |
| B5 MovingWall(2D/3D) | 真ん中の壁に、ドア(Kinematic の動く壁)と遠回りの広いすき間。1でドアを開け閉めすると、格子が `SetAutoRebakeInterval`(0.25秒)で焼き直されて、経路が変わる。3で自動の焼き直しを切ると、4(`RequestRebake`)まで格子が古いまま。2でクリックの意味を「目的地 / 柵」に切り替え、柵は `NavGrid::SetBlockedBox`(最大6個。Cで消す)。version・焼いた時間・経路の有無を表示 | `Uninit` で格子と経路の線を消す(`ResetSettings` でも戻る) |
| B6 TerrainPath(3D) | 丘の上を、`NavGridComponent::SetTerrain` で高さ・坂を取り込んだ格子で歩く。1・2・3で登れる坂の角度(15・20・30度)を変えると、通れないマス(急な坂)の数と、歩く道が変わる。クリックの位置は、タグ "Terrain" の丘へ `Physics::Raycast`。通れないマスの数を表示 | `Uninit` で格子と経路の線を消す(`ResetSettings` でも戻る) |

**設計書に案が無く、安全側に決めたこと**:
- **B3 の操作**: 設計書の「矢印: 目標 / ←→: 見る向き / ↑↓: 視野角」は矢印が重なるので、矢印=ターゲット、A・D=向き、W・S=視野の角度にした。`IsBlocked` は Engine では private なので、「扇形の中なのに見えない」を、`IsInCone` と `CanSee` の違いで見せた。
- **B5 の「クリック: 柵を置く」**: 「目的地を動かす」クリックとぶつかるので、2キーでクリックの意味を切り替える形にした。ドアは「ドアの通り道に緑がいる間は閉められない」(挟まって動けなくなるのを防ぐ。ガードが効くと、画面に知らせが出る)。
- **B6 の坂の角度**: この丘の一番急な所は約32度なので、設計書どおりの 20・40・60度では 40度以上が何も変わらない。15・20・30度にした(通れないマス: 15度で約3500、20度で約900、30度で約65。歩く道は 15度=大回り、20度=少し避ける、30度=ほぼまっすぐ)。`NavGridSettings` は作る時に決まる(後から変える関数は無い)ので、角度を変える時は格子を作り直す(古い方を `Destroy`、`SetNavigation` で渡し直す。Debug では焼くのに0.2秒くらいかかる)。
- **壁のすき間**: 歩く物は、`agentRadius` と、マスの半分ずつの分だけ壁から離れないと通れない。すき間が `(agentRadius + cellSize/2) x 2 + cellSize` より狭いと、経路が見つからない(スクラッチのテストで、MovingWall の2Dはすき間90・マス25・半径26で経路が無かった)。2Dの Path はすき間110・マス25・半径26、2Dの MovingWall は、動く物を44・マス20・半径22にして、すき間を90と100にした。3Dは、動く物80・マス50・半径40で、すき間200以上。
- **動く物の `BodyType`**: 壁にぶつけたい物は Dynamic(`MoveComponent::ApplyVelocity` は、Dynamic なら `SetVelocity`、Kinematic ・剛体無しなら `Transform` を直接動かす)。Kinematic の物は動かない箱をすり抜けるので(Batch 4 の気付き)、B2 の3つはその性質を使って「お互いにすり抜けて動きだけを比べる」ために Kinematic にした。
- **B2 の止まる距離**: `HomingComponent` は速さが一定で、最後の1歩を調整しないので、止まる距離が小さい(既定 1)と目標の周りを回り続けて止まらない。3つとも止まる距離を30(3Dは45)にした。
- **画面の配置(1280x720の座標を計算して確認)**: 2Dの競技場は、左上の板・左下右下のボタンと重ならない範囲(x -600〜600、y -250〜50。壁の外側は x ±630、y -280〜80)。状態の板は説明の板の下で幅740、行数は4〜5(6行になるのは B5 の知らせが出る間だけ)。3Dは、カメラ(見る点 (0,0,60)・向き -58度・距離 1250)で、競技場の手前の角が (±448, -205)、奥の角が (±333, 104) に映る。奥の壁は左上の板の後ろに隠れるが、動く物は板の下に映る。
- **Engine に無いもの / 制約**: (1) `NavGridSettings` を後から変える関数が無い(B6 は作り直し)。(2) `NavGrid::SetBlockedBox` で塞いだ範囲は、重なった柵の一方を外すと、もう一方のマスまで通れるようになる(手で登録した印が1種類のため)。B5 は、柵を外す時に、残りの柵を登録し直している。(3) 線を引く API が無いので、柵・円・線は点を並べて描いた(Batch 1 と同じ)。
- **確認の方法**: ゲームの操作は、していない。動きは、Engine.lib にリンクした画面なしのテスト(Physics を初期化して、数十〜数百ステップ)で確認した: B1 壁で止まる・止まる距離・速さ(2D/3D)、B2 青が着く・橙が行き過ぎる・緑が曲がる(2D/3D)、B3 壁の向こうは見えない・向きや角度や距離で外れる・3Dは目を上げると見える、B4 緑が壁を避けて着く・赤が止まる・目的地を動かすと追う、B5 ドアで経路が変わる・自動の焼き直しを切ると変わらない・柵で道が塞がると経路が無くなる、B6 角度で通れないマスが変わる・丘を歩いて着く・丘への Raycast。

### Batch 6 で決めたこと・作った物

Audio の A1 Sound・A3 Sound3D と、Data の T1 Csv・T2 Save を作りました(**Engine は変えていません**。`Sample2D` / `Sample3D` も変えていません)。**A2 Voice は作りません**。Audio カテゴリの機能は Sound・Sound3D の2件、Data カテゴリは Csv・Save の2件です。カテゴリ一覧は8行(Core / Render / UI / Render3D / Physics / Behavior / Audio / Data)のままで、各カテゴリの件数は Core `[8]` Render `[8]` UI `[4]` Render3D `[10]` Physics `[6]` Behavior `[6]` Audio `[2]` Data `[2]`(合計 46)になり、「準備中」のカテゴリは無くなった。Audio・Data の機能一覧は2行で、行の間隔は最大の84、いちばん下の行も Back・Mode ボタンと重ならない。

**方針**: Batch 0〜5 の型を踏襲(2D/3D 両対応は引数 `DemoMode`。3D の舞台は `CreateDemoStage3D`(`CreateDemoGuide` の後)。日本語は `MakeDemoTextStyle`。共通の `DemoPlane`・`DemoMarker`・`DemoStatus`・`DemoAgent` の `CreateDemoGoal` / `CreateDemoField` を再利用)。

**作ったデモ**:

| デモ | 内容 | 後片付け・全体設定 |
|---|---|---|
| A1 Sound(共通) | BGM(`TitleBGM.wav`。`AudioSourceComponent`・グループ BGM・ループ・位置を使わない)と SE(`shot.wav` は `Audio::PlayOneShot`、`hit.wav` は `AudioSourceComponent`)。BGM/SE の音量グループをスライダー(`SliderComponent`)で変える。**フェード**は `TweenValue` で BGM 自体の `SetVolume` を2秒で動かして作る(フェードアウトは終わったら `Stop`)。BGM 自体の音量の棒と、グループの音量・再生中かを表示。SE のスライダーは離すと SE が鳴る。1〜5 とボタン | `Uninit` で BGM と SE を止める(借りた音のデータは `AudioSourceComponent` が返す)。音量グループは `ResetSettings` が戻す |
| A3 Sound3D(2D/3D) | 橙の音源(`AudioSourceComponent`・ループ・`SetDistance`)が楕円を回り、青の聞く側(`AudioListenerComponent`)を WASD で動かす。緑の輪=最大音量の距離、黄の輪=聞こえなくなる距離。1で鳴/止、2で `SetSpatial` のオン/オフ、↑↓で聞こえなくなる距離、←→で回る速さ、SPACE で `Audio::PlayOneShotAt`(その場所に1回)。左右の目安の棒と、距離・距離による音量・向きを表示。3D は `SetUse3DCamera(true)` で3Dカメラの向きで聞く | `AudioSourceComponent` / `AudioListenerComponent` が `Uninit` で後片付けする |
| T1 Csv(共通) | デモ専用の `Assets/Demo/DemoTable.csv` を `Csv::Load` で読み、表で出す(`GetString` / `GetInt` / `GetFloat` / `HasColumn` / `FindRow`)。0.5秒ごと(`Timer`)に `IsFileUpdated` を調べ、変わっていたら `Reload`(ホットリロード)。読み直すと枠が緑に光る。R で今すぐ読み直す。読めない時は赤い文字。編集するファイルの場所を画面に出す | 読むだけで書かない。後片付けは不要 |
| T2 Save(共通) | `SaveData` に整数 `Demo_Count`・小数 `Demo_Ratio`・日本語の文字 `Demo_Name` を `Set`、`Save` / `Load` / `DeleteKey` / `HasKey`。スロット1〜3。未保存の印、保存先の表示 | `Uninit` は不要(`SaveData` の終了時の自動保存の決まりに任せる。README §38 に書いた) |

**設計書に案が無く、安全側に決めたこと**:
- **A2 Voice を作らない**: `LoadWave` / `StartVoice` / `UpdateVoice` / `StopVoice` は `Audio.h` に「Engine内部用(ゲーム側からは呼ばない)」と書かれていて、ゲームが使う入口は `AudioSourceComponent` と `PlayOneShot` / `PlayOneShotAt`(A1・A3 で見せた)。`Resources::LoadSound` の共有は C8 Resources で見せている。同時に鳴らせる数(32個)は `Audio.h` の説明で足りる。設計書も「優先度は低い・Engine を触る人向け」としていたので、省いた。必要になったら、`Audio::VoiceParams` を直接使う小さなデモとして足せる。
- **未確定 #9(2D 版の空間音)**: 実際のコードで確認 → `AudioListenerComponent` は2Dでも使える(位置は付けたオブジェクト、向きは画面の上が正面・右が右で固定。2Dカメラに付けなくても、聞く側の物に付ければよい)。そこで A3 は2D/3Dの**両方**にした。3D は `SetUse3DCamera(true)`。
- **未確定 #16(デモ専用の素材)**: 外のサイトから音・画像・フォントは取っていない。BGM と SE は既存の `TitleBGM.wav` `shot.wav` `hit.wav`。**A3 だけ、`Assets/Demo/DemoTone.wav` を自動で作った**(2秒・44100Hz・16bit・**モノラル**・220Hz の音を約0.4秒ごとに強弱。最初と最後が0でつながりループしても鳴りやすい。位置による左右はモノラルの音でないと出ないため、既存のステレオの音は使えなかった)。CSV は `Assets/Demo/DemoTable.csv`(Name・Hp・Speed・Memo の3行)を自作。どちらも `Engine.vcxproj` の `CopyRuntimeFiles`(`Assets` を `xcopy /e`)で出力先の `Assets\Demo\` に届く(確認済み)。
- **CSV の編集のしかた**: 画面に出る出力先のコピー(`x64\Debug\Assets\Demo\DemoTable.csv`)を編集して保存すると反映される。元の `3D_FreamWork\Assets\Demo\DemoTable.csv` を編集した時は、次のビルド(F5)でコピーされる(`xcopy /d` は元の方が新しい時だけ)。
- **SaveData のキーとスロット**: `SaveData` のキーは英字・数字・`_` だけなので、`demo.` ではなく `Demo_` で始まるキー(`Demo_Count` `Demo_Ratio` `Demo_Name`)にした。保存先は `%LOCALAPPDATA%\3D_FreamWork\save{slot}.txt` で、サンプルと同じ場所にある。**サンプルのセーブ(スロット0。ハイスコアなど)を壊さないよう、デモはスロット1〜3だけを使い、スロット0は読み書きしない**。仕様(`SaveData.h/.cpp` を読んで確認): 起動時に Engine が `Load(0)`、終了時に「書き換えていれば、最後に Load/Save に成功したスロットへ」`Save`。デモがスロット1を読んでから値を変えると、終了時の自動保存はスロット1へ行き、`save0.txt` は変わらない。デモを開かない・開いても書き換えない時は、何も書かれない。スロットを読めなかった時は、そのスロットを書き換えない(サンプルのデータを別のスロットに写さないため)。
- **A1 の音量**: 実際に鳴る音量は「その音自体の `SetVolume` × グループの音量」。フェードは自体の音量(BGM の棒)を動かし、スライダーはグループの音量を動かす。2つは別の値で、画面に両方出す。
- **A3 の PlayOneShotAt**: SPACE の音は、Engine の既定の距離(最大音量 300・聞こえなくなる 2500)で鳴る。このデモの `SetDistance` の距離は `AudioSourceComponent`(橙の玉の音)だけに効く。
- **画面の配置(1280x720 の座標で確認)**: 状態の板は説明の板の下(y 150〜、行の高さ25)。Sound は4+1行で幅820、Sound3D は4行で幅1100、Csv は4行で幅1230、Save は5行で幅1230。Sound のスライダー(x -530〜-130・y -10 と -100)・ボタン(x 25〜475・y 20〜-100)・BGM の棒(y -190)は、板の下(y 65 より下)で、左下の Back(x -610〜-450・y -338〜-282)と重ならない。Csv の表は x -600〜400・y 70〜(最大10行分)で、下の説明文は Back を避けて x -410 から。Save のボタン3列(x -400・-100・200、幅270)は y -10〜-166 で板の下。Sound3D の動ける範囲は Behavior と同じ競技場(2D: x ±600・y -250〜50、左右の目安の棒は y -300)。
- **Engine に無いもの / 制約**: (1) `Csv` に列の名前の一覧を取る関数が無いので、Csv デモは見せる列(Name・Hp・Speed・Memo)を決め打ちにして、`HasColumn` で有無を見せた。(2) `Csv` の `IsFileUpdated` / `Reload` は読み取り専用で、ファイルを書き換える関数は無い。(3) `SaveData` はスロットを切り替えると、今の値をすべて捨てる(Load の仕様)ので、スロット切り替え = 読み込み。(4) 音の再生状態は耳でしか確かめられない。
- **確認の方法**: ゲームの操作・耳での確認は、していない。`Csv` と `SaveData` は、Engine.lib にリンクした画面なしのテストで確認した: デモ用 CSV の読み込み(3行・4列・値・日本語・`FindRow`)、ファイルを書き換えて `IsFileUpdated` → `Reload`(行・列の増減)、ファイルが消えた時に今の中身を残すこと、無かったファイルが後からできた時に読めること、`SaveData` の保存→終了→再起動→読み込み(整数・小数・日本語の文字)、スロット90(サンプル役)のキーが、スロット91(デモ役)の保存・終了の自動保存の後も残ること、実際の `save0.txt` のハッシュが変わらないこと。音は、`AudioSourceComponent` と `Audio` の `Play` / `StartVoice` が、音声デバイスが無い・音が読めない時に何もしないで戻る作りであることをコードで確認した。
- 合計: 8 + 11 + 6 + 6 + 5 + 6 + 4 = **46 機能**(6章の47件のうち、A2 Voice を作らないので1件減った)。Batch 0 の「8」は、既存6デモのうち Terrain を3つに分けた数です。
- **順番を入れ替えてよい所**: Batch 2〜6 は互いにほぼ独立です(Batch 5 は Batch 4 の物理の使い方に頼る)。担当者が分かれているなら並行して進められます。
- **各段階の終わり**に、(1) Debug/Release のビルド (2) reviewer のレビュー (3) ユーザーが 2D/3D 切り替えを含めて遊んで確認 (4) コミット文と README 更新。

---

## 9. 影響・リスク・未確定事項

### 9.1 両サンプル・Engine への影響
- **Engine: 変更しません。** 新しい機能の追加も、既存ファイルの変更もありません(この設計書の範囲では)。
- **Sample2D / Sample3D**: Batch 0 の最後に、7.2 の物を**消すだけ**です(ゲームの動きは変わらない)。タイトル画面から DEMO ボタンが無くなります。
- **ソリューション**: `3D_FreamWork.sln` に Demo が増え、ビルドする物が3つ(Sample2D / Sample3D / Demo)+ Engine になります。
- **`.claude/scripts/`**: `check-files.ps1` / `build.ps1` が Demo を正しく扱うかを Batch 0 で確認します。扱わない場合の変更は、`.claude/` の設定の変更になるので、**ユーザー本人の了承が必要**です(未確定事項 13番)。
- **README / CLAUDE.md**: README は更新します(10章)。`CLAUDE.md` の「構成」の節(「3プロジェクト」)が古くなりますが、`CLAUDE.md` はユーザー本人の了承が無ければ変えません(未確定事項 14番)。

### 9.2 リスク
| リスク | 内容 | 対策 |
|---|---|---|
| **全体設定のリセット漏れ** | 光・影・LOD・倍率・枠線・揺れ・音量・カーソルなどが、シーンをまたいで残る。2D から3Dへ(逆も)切り替えた時に、前のデモの設定が見た目や動きを狂わせる | `ResetSettings` を **1か所に集約**し、`Init` と `Uninit` の両方で呼ぶ。デモが新しい全体設定を触ったら、`ResetSettings` に足す決まりにする(新しいデモの追加手順に書く)。reviewer のチェック項目に入れる |
| **1つのプロジェクトでの 2D/3D 同居** | 障害は見つからなかった(5章)。ただし、2Dのデモで `SetUseGravity(false)` を付け忘れると、物が落ちていく | 2D版の剛体を作る共通の工場関数(例: `CreateDemoBody2D`)を作り、重力OFF・Z固定を1か所で付ける |
| **ビルド時間** | 約47機能 x ファイル数(Scene + Object + Controller)で、`.cpp` が 150〜200 個になる。Release の全体最適化(`WholeProgramOptimization`)でリンクが遅くなる | Demo は Release の `WholeProgramOptimization` を切ることを検討する(サンプルと設定が変わる。ユーザーに相談)。複数プロジェクトの並行ビルドは `/FS` で済んでいる |
| **アセットのコピー** | Engine の `CopyRuntimeFiles` が、共有 `$(OutDir)` に `Assets/` をコピーする。**Demo だけをビルドした時**に、Engine が先にビルドされなければ古い `Assets/` のまま | `ProjectReference` で Engine に依存しているので、通常は Engine が先にビルドされる。デモ用の新しい素材(音・画像・CSV)は `3D_FreamWork/Assets/` に置く(サンプルと共有になることを、素材の追加時に意識する) |
| **ファイル名の衝突** | 同じプロジェクト内で同名の `.cpp` があると `.obj` が衝突 | 3章のとおり、ファイル名をプロジェクト内で一意にする。`check-files.ps1` の結果で気付けるようにする |
| **Resources の読み込みの量** | 47機能 x 画像・モデル・音が増えると、読み込みが遅くなる | `Resources::UnloadUnused()` が切り替えで解放する。大きな素材(FBX・音)は、そのデモに入った時にだけ読む |
| **移行の取りこぼし** | サンプルから消す時に、ゲーム本体も使っているファイルを消してしまう | 7.2 の「消してはいけない物」を守る。消した後に必ず Debug/Release の両方のビルドと、ゲームの確認(ユーザー) |
| **日本語の文字化け** | `ButtonComponent` は Arial 固定 | ボタンは英語、説明は `TextRenderer`(Noto Sans JP)。新しい部品でも同じにする |
| **キーの衝突** | 3D のデモはカメラが Q / E / 右ドラッグ、デモ側が数字キー・矢印を使う。Tab・Esc は共通部品が使う | 操作の表(6章)を作る時に、予約キー(Tab=2D/3D、Esc=戻る、Q・E=ズーム)を使わないと決める |

### 9.3 未確定事項(ユーザーに決めてほしいこと)
番号はこの文書の他の所からも参照しています。

1. **Demo の入口**: 起動して、すぐ「カテゴリ一覧」を出してよいか(タイトル画面を作らない案)。それとも、簡単なタイトル(「Engine Demo」と Start ボタン)を作るか。
2. **カテゴリの分け方**: 8カテゴリ(Core / Render / UI / Render3D / Physics / Behavior / Audio / Data)でよいか。分け方・名前の希望は。
3. **2D/3D の選択を覚える単位**: **全体で1つ**(最初の案)か、**機能ごと**に覚えるか。
4. **起動時の 2D/3D の初期値**: 2D と 3D のどちらから始めるか(おすすめ: 3D。印象が強いため)。
5. **前回の選択を保存するか**: `SaveData` に今の側を保存して、次回の起動で復元するか(おすすめ: しない。シンプルなため)。
6. **前/次のデモのボタン**: 最初から付けるか、後回しにするか(キーは PageUp / PageDown の案)。
7. **「共通」の扱い**: 2D と 3D の区別が無い機能(Input・Time・UI・Audio など)で、切り替えボタンは**押せない状態で表示**する案(今の案)でよいか。それとも非表示にするか。
8. **Esc の動き**: 一番上のカテゴリ一覧で Esc を押したら、アプリを閉じる案(今の案)でよいか。閉じる前に確認を出すか。
9. **2D 版の空間音(A3)**: `AudioListenerComponent` を2Dカメラで使えるか着手時に確認し、できれば「両方」にしてよいか。
10. **デモの優先順位・数**: 47機能で足りるか。多すぎる所(例: A2 Voice のような高度なデモ)を減らす/後回しにするか。他に見せたい機能は。
11. **バッチの順番**: Batch 0 → 1 → 2 → 3 → 4 → 5 → 6 でよいか。先にやりたいカテゴリは。
12. **ソリューションの起動プロジェクト**: Demo を起動プロジェクトにするか、今のまま(Sample のまま)か。
13. **`.claude/scripts/` の変更**: `check-files.ps1` / `build.ps1` が Demo を扱えない場合、スクリプトの変更を了承してよいか(`.claude/` はユーザー本人の了承が必要)。
14. **`CLAUDE.md` の更新**: Demo プロジェクトができたら、`CLAUDE.md` の「構成」(3プロジェクト → Demo を足す)と「作業の進め方」の説明を更新してよいか(ユーザー本人の了承が必要)。
15. **Release の最適化**: Demo の Release で `WholeProgramOptimization` を切ってビルドを速くしてよいか(サンプルと設定が違ってよいか)。
16. **デモ専用の素材**: デモのために新しい画像・音・FBX・CSV を `Assets/` に足してよいか(足す場合は、ライセンスの確認が必要。今ある素材だけで作ることもできる)。

(Engine の変更が必要になる可能性がある点は、今のところありません。もし、デモを作る途中で Engine の不足が見つかったら——例えば「音のフェードに専用の関数が欲しい」「2Dの重力を世界ごと切り替えたい」など——その1つずつを、①何を変えるか ②なぜ必要か ③両サンプルと Demo への影響、を説明して、ユーザーの了承をもらいます。)

---

## 10. 完成後の README・解説との関係

- **README**: Demo プロジェクトが動いたら、README の次の所を直します。
  - §0「プロジェクト構成」: 3プロジェクト → 4プロジェクト(Engine / Sample2D / Sample3D / Demo)。
  - §22「サンプル構成」: サンプルから Demo が無くなったことを反映する。
  - **§38「デモ」を、Demo プロジェクトの説明に書き直す**(開き方・カテゴリ・2D/3D の切り替え・デモ一覧・新しいデモの追加方法・全体設定のリセット)。節の番号は変えない方がリンクが壊れない。
  - 機能ごとの節(§12 など)の「Sampleでの使われ方」に、「Demo の ○○ で見られます」を足す。
- **エンジン解説 HTML(別の依頼)**: 解説の各機能の説明から、対応するデモ(6.12 の表)へリンクする・「Demo の ○○ を動かしてみよう」と書く、という関係になります。**解説はこの設計書の6.12の対応表を使えば作れます**。デモの名前(C1〜T2 の番号と英語名)は、解説でも同じ名前を使います。
- **この文書(`docs/DemoPlan.md`)**: 実装が進んだら、各デモの「状態」(未着手 / 実装済み)を表に足して更新します(`EngineRoadmap3.md` と同じ)。
