# 3D_FreamWork

DirectX11製の、Unity風のコンポーネント指向ゲームフレームワーク。2D(スプライト)と3D(メッシュ)の両方に対応しています。

このドキュメントは、これから機能を追加していくチームメンバー向けの使い方ガイドです。

今後のEngine機能追加の計画(関数の設計案・作る順番)は [`docs/EngineRoadmap.md`](docs/EngineRoadmap.md) を参照してください。

Engineの各ファイル・関数・使い方と、設計図(継承図・描画の順番など)の解説(HTML。ブラウザで開く)は [`docs/EngineGuide/index.html`](docs/EngineGuide/index.html) を参照してください。

---

## 0. プロジェクト構成

このソリューション(`3D_FreamWork.sln`)には4つのプロジェクトがあります。

```
3D_FreamWork\
├── 3D_FreamWork.sln
├── PhysX.props        共有MSBuildプロパティシート(PhysXのinclude/lib設定を1箇所にまとめたもの)
├── Assets\             全プロジェクト共有の画像・音声・モデル・フォント(Fonts\。§19)・表(Data\。§25)・地形の高さの画像(Terrain\。§34)
├── Shader.hlsl / Shader3D.hlsl / Shader3DSkin.hlsl / Shader3DCommon.hlsli / Shader3DWater.hlsl   全プロジェクト共有のシェーダー(Shader3DSkinは骨で曲げるFBX用。§27。Shader3DCommon.hlsliは3Dの2つのシェーダーに共通の見た目の計算で、マテリアルもライトも縁の光・枠線もここ。§30・§31・§33。Shader3DWaterは水面(WaterRenderer)専用。§35)
├── Engine\             フレームワーク本体(静的ライブラリ、Engine.vcxproj)
├── Sample2D\           2Dのサンプルゲーム(実行ファイル、Sample2D.vcxproj)
├── Sample3D\           3Dのサンプルゲーム(実行ファイル、Sample3D.vcxproj)
└── Demo\               Engineの機能のデモ(実行ファイル、Demo.vcxproj。§38)
```

- **Engine**: `GameObject`/`Component`/`RigidbodyComponent`/描画/音声などフレームワーク本体。それ単体では実行できない静的ライブラリ(`Engine.lib`)で、基本的にみんなで触る場所ではありません。
- **Sample2D** / **Sample3D**: それぞれ独立した実行ファイル。どちらも同じ`Engine.lib`をリンクしているので、**Engineを直した内容は、両方のサンプルを再ビルドするだけで反映されます**。中身の`Game\Objects\`/`Game\Components\`/`Game\Scenes\`は、後述の「2. オブジェクトの作り方」と同じパターンです。
- **Demo**: Engineの機能を、見て・触って分かるようにしたデモ集の実行ファイル(§38)。ゲームではなく、機能ごとの小さなシーンをカテゴリ別に並べ、2D版と3D版をその場で切り替えて見られる。同じ`Engine.lib`をリンクしているので、Engineの機能を足したら、ここにデモを足して紹介する。
- 外部ライブラリは、リポジトリ直下(`3D_FreamWork\`の1つ上)の`ThirdParty\`にあります: `ThirdParty/PhysX/`(物理演算。§23)と `ThirdParty/ufbx/`(FBXの読み込み。§27。MITかパブリックドメインのどちらかを選べるライセンスで、条文は同じフォルダの`LICENSE`)。
- ビルド後、`Engine.lib`/`Sample2D.exe`/`Sample3D.exe`/`Demo.exe`は全て同じ`x64\Debug\`(`Release\`)フォルダに出力されます。`Assets\`とシェーダーは、Engineのビルドのたびに(`Engine.vcxproj`の`CopyRuntimeFiles`)、新しいファイルだけがそこへ自動コピーされます。Engineのコードが変わっていなくても動き、`Assets/`とシェーダーを高速最新チェックの入力に登録してあるので(`UpToDateCheckInput`)、`Assets/`だけ編集して実行(F5)しても反映されます。ビルド済みの`.exe`をダブルクリックするだけでも動きます。
- ソリューションを開いてF5で実行する時は、実行したいもの(Sample2D・Sample3D・Demoのどれか)をソリューションエクスプローラーで右クリック→「スタートアッププロジェクトに設定」してから実行してください(複数のスタートアッププロジェクトを同時に起動する設定も可能です)。

### Engineのフォルダ構成

`Engine\`の中は、機能ごとに11個のフォルダに分けてあります(`.h`と`.cpp`は同じフォルダに入れます)。

| フォルダ | 役割 |
|---|---|
| `Core\` | フレームワークの土台。ゲームの起動・シーン・GameObject/Component・時間・入力・イベント・プーリングなど |
| `Render\` | 2Dの描画と画面まわり。スプライト・画像・カメラ・画面演出・パーティクルなど |
| `Render3D\` | 3Dの描画。メッシュ・FBXモデル・地形・水面・ライト・影・マテリアル・距離で省く設定(LOD)と描画カウンタ(§37)など |
| `Physics\` | 物理演算(PhysX)。剛体Component・コライダー・衝突の通知 |
| `Audio\` | 音の再生。BGM/SE・音を聞く側(Listener)と鳴らす側(Source) |
| `UI\` | 文字・ゲージ・スライダー・ボタンなど、画面に出す部品 |
| `Animation\` | 動きの時間変化。Tween・イージング・スプライトやモデルのアニメーション |
| `Transition\` | シーン切り替えの演出(フェード・スプライトを使った切り替え) |
| `Behavior\` | 動きの部品。移動・追いかけ・ホーミング・視界判定・経路探索(壁を避けて追いかける。§36)など、そのまま付けて使うComponent |
| `Data\` | データの読み書き。CSVの表・セーブデータ |
| `ThirdParty\` | 外部のコード(WICTextureLoader11)をそのまま置く場所 |

新しいファイルを作る時は、一番近い役割のフォルダに置きます。迷ったら、同じ仲間のファイル(例: 新しいComponentが「追いかける動き」なら`Behavior\`)と同じ場所にしてください。`.vcxproj`と`.vcxproj.filters`の登録(フィルターは同じフォルダ名)も忘れずに。

#include の書き方: 同じフォルダ内は`#include "Foo.h"`、別のフォルダは`#include "../Core/GameObject.h"`、Sampleからは`#include "../../../Engine/Core/GameObject.h"`のように、フォルダ名を挟んだ相対パスで書きます。

新しい3つ目のサンプルを作りたい時は、`Sample2D`か`Sample3D`のフォルダ構成をコピーして名前を変え、`.sln`に登録し、`Engine.vcxproj`への`ProjectReference`と`PhysX.props`のImportを忘れずに設定してください。

---

## 1. 基本構成: GameObject と Component

`GameObject`はそれ自体では何もしない「入れ物」です。見た目や動きは、`AddComponent<T>()`で後から部品(`Component`)を付けて作ります。継承ではなく組み合わせで機能を作る、という考え方です。

```cpp
GameObject* enemy = scene.CreateObject("Enemy", "Enemy");  // 名前, タグ
enemy->transform.position = { 100.0f, 0.0f, 0.0f };
enemy->transform.scale    = { 100.0f, 100.0f, 1.0f };

enemy->AddComponent<SpriteRenderer>(L"Assets/enemy.png");
enemy->AddComponent<BoxRigidbodyComponent>(BodyType::Kinematic, enemy->transform.scale, 1.0f, false, true);
```

- `GetComponent<T>()`: 同じGameObjectに付いている別のComponentを取得する
- `SetActive(false)`: 非表示・Update/Draw停止(**破棄はされない**。オブジェクトプーリングで使う)
- `Destroy()`: 本当に破棄する(次のフレームで実際に取り除かれる)

新しいComponentを作る時は`Component`(`Engine/Core/Component.h`)を継承し、必要な関数だけoverrideします:

```cpp
class MyComponent : public Component {
public:
    void Init() override { /* AddComponentされた直後に1回だけ */ }
    void Update(float dt) override { /* 毎フレーム */ }
    void Draw() override { /* 毎フレーム、描画したい時だけ */ }
    void Uninit() override { /* 破棄される時 */ }
};
```

---

## 2. オブジェクトの作り方: 工場関数パターン

`Player`や`Enemy`のようなクラスは作りません。代わりに、各サンプル(`Sample2D/`または`Sample3D/`)の`Game/Objects/`に「GameObjectを組み立てて返す関数」を1つ作ります(`Sample2D/Game/Objects/Player.cpp`などを参照)。

```cpp
// Game/Objects/Enemy.h
GameObject* CreateEnemy(Scene& scene);

// Game/Objects/Enemy.cpp
GameObject* CreateEnemy(Scene& scene) {
    GameObject* enemy = scene.CreateObject("Enemy", "Enemy");
    enemy->transform.position = { 0.0f, 0.0f, 0.0f };
    enemy->transform.scale    = { 100.0f, 100.0f, 0.0f };

    auto* sprite = enemy->AddComponent<SpriteRenderer>(L"Assets/enemy.png");
    enemy->AddComponent<BoxRigidbodyComponent>(BodyType::Kinematic, enemy->transform.scale, 1.0f, false, true);

    return enemy;
}
```

呼び出す側(`GameScene::Init()`など)は `CreateEnemy(*this);` と呼ぶだけです。新しい種類のオブジェクトを追加する時は、この形式(`Game/Objects/○○.h/.cpp` + `Game/Components/○○Controller.h/.cpp`)に合わせてください。**Sample2Dだけで使うオブジェクトはSample2D側に、Sample3Dだけで使うオブジェクトはSample3D側に**追加してください(両方に同じようなオブジェクトが必要になった場合、今のところは両方に同じファイルをコピーする形になっています。共通化は将来の課題です)。

---

## 3. Transform(位置・大きさ・回転、親子階層)

```cpp
XMFLOAT3 position; // 位置
XMFLOAT3 scale;    // 大きさ(初期値は等倍 {1,1,1})
XMFLOAT3 rotate;   // 回転(度数)
```

親子関係:

```cpp
weapon->transform.SetParent(&player->transform);  // playerの子にする
weapon->transform.position = { 20.0f, 0.0f, 0.0f }; // 親からのローカルオフセット
```

子は親が動く・回転する・拡縮するのに合わせて一緒に動きます。当たり判定(Collider)も含めて、親子階層はちゃんと反映されます。

**注意**: `GetWorldTransform()`(2D用)と`GetWorldMatrix()`(3D用)は別物です。自分でTransformの値を直接読み書きする場合は`GetWorldTransform()`(親を辿ったワールド座標のposition/scale/rotateが返る)を使ってください。3Dメッシュの描画は内部で`GetWorldMatrix()`を使っています。

---

## 4. 描画: SpriteRenderer(2D) と MeshRenderer(3D)

### SpriteRenderer(2D)

```cpp
auto* sprite = obj->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3); // ファイルパス, 横分割数, 縦分割数
sprite->SetSpriteIndex(1);           // スプライトシートの何コマ目か
sprite->SetColor(1.0f, 0.0f, 0.0f, 1.0f); // r, g, b, a
```

画像そのものは、同じ画像を使うオブジェクトみんなで共有しています(§11)。コマ番号(`SetSpriteIndex`)と色(`SetColor`)は**1体ごと**なので、同じ`player.png`を使うPlayerと敵で、色やコマが連動することはありません。コマを時間に合わせて切り替える(アニメーションさせる)時は、`SpriteAnimationComponent`(§17)を一緒に付けます。

**描き方(`SpriteMode`)は3種類**あります。`SetMode`で選びます(既定はUI):

```cpp
sprite->SetMode(SpriteMode::UI);        // UI(既定)
sprite->SetMode(SpriteMode::World2D);   // 2D
sprite->SetMode(SpriteMode::World3D);   // 3D
```

| `SpriteMode` | 呼び方 | 大きさ(z) | カメラ | 前後の決まり方 | 使う場面 |
|---|---|---|---|---|---|
| `UI` | UI | zで変わらない | 2Dカメラの位置でスクロール(`DemoScreenFixController`のように画面に固定したい時は、カメラの分を足す) | **必ず最前面**。深度は使わない。UIどうしは`drawPriority` → **作った順**(後に作った物が手前)。**zは使わない** | HUD・ボタン・Sample2Dの爆発 |
| `World2D` | 2D | zで変わらない(遠近なし) | 2Dカメラの位置でスクロール | **zが小さい(手前)ほど手前**(深度バッファ)。UIには隠される | 2Dのゲームで、zで前後を決めたい物(Sample2Dの背景・壁・敵・Player・弾) |
| `World3D` | 3D | zで変わる(奥ほど小さい) | 3Dカメラの透視。常にカメラの方を向く板(ビルボード) | 深度バッファ。3Dメッシュと同じ深度で、出たり隠れたりする | 3D空間に置く看板・敵の画像 |

- **UI**: 今までのUIモードと同じです。深度を使わず、`position.z`で大きさも変わりません。
- **2D**: 3Dメッシュなど3Dの物は無いのが普通です。深度バッファは、2D・3Dの物とメッシュだけで使い、**UIは2Dの深度に影響しません**(UIが後から描かれて、2Dの物を上書きするだけ)。2Dと3Dを同じシーンで混ぜることは想定していません(混ぜると、深度の値で前後が決まります)。
- **3D**: 今までのWorldモード(3D)と同じです。3Dメッシュ(Mesh)と全く同じcamera3DのView/Projection行列を使います。`CameraController`でカメラを動かした時もMeshと同じように正しく追従します。
- **今までの`SetWorldSpace(bool)`**は残してあります(互換)。`SetWorldSpace(true)` = `SpriteMode::World3D`、`SetWorldSpace(false)` = `SpriteMode::UI`です。`GetMode()`で今の描き方を読めます。
- `Image::Draw`は、今までの`bool worldSpace`の版と、`SpriteMode`を渡す版があります(`Image::Draw(transform, texID, color, sheet, SpriteMode::World2D)`)。

**2Dのzの注意**: 2Dモードの深度は3Dカメラからの距離(`viewZ`)から作るので、zは「3Dカメラのz(既定-500)より+5以上奥」にしてください(`viewZ <= 0`の物は深度が0=最前面になります)。

**描画の順番**: `Scene::Draw`は3つのパスで描きます。まず`drawPriority`の小さい順(同じなら`z`の小さい順)に、UI**以外**(3Dメッシュ・2D/3Dのスプライト・デバッグの線など)を描き、次にUIのスプライトを**`drawPriority` → 作った順**にまとめて描きます。そのため、UIのスプライトは、`z`が奥でも、3Dメッシュより先に作っても、必ず手前に出ます。2Dのパーティクル(`use3D = false`)もUIと同じ扱いです(2Dモードにする設定はありません)。文字(`Text::Draw`)・`Image::DrawRect`(ゲージ・ボタンの文字・スライダーの帯も、これらを呼んでいます)は、`Scene::Draw`の間は**その場で描かず**、UIスプライトを描いた後に、**呼んだ順**にまとめて描きます(オーバーレイキュー。Direct2Dは命令がたまると途中で画面に出すため、その場で描くと、後から描くUIスプライトに隠れてしまうからです)。四角を描いてから文字を描けば、四角の上に文字が出ます。画面エフェクト・シーン切り替えの演出は、`Scene::Draw`の外で描くので、これらよりさらに手前に重なります(今まで通り)。つまり描く順は、①UI以外(3Dメッシュ・2D/3Dスプライト・線) → ②UIスプライト → ③文字・四角 → ④画面エフェクト・遷移 です。

テクスチャの透明な部分は、2D・3Dなら後ろのオブジェクトが透けて見えます(`clip()`で深度を書き込まないようにしているため)。2D・3Dはカリングも無効になっているので、`SetBillboardLock`で軸を固定した板をカメラが裏側から見ても、消えずにちゃんと描画されます(UIは今まで通り裏面カリングあり)。

**注意**: 3Dの`scale`は(UI・2Dのピクセル単位とは違い)`MeshRenderer`と同じ**ワールド単位**として扱われます。`rotate.z`は、ビルボードの板の中での見た目の回転(画面に対する回転)として機能します。
**ビルボードの回転を軸ごとに止める**: `CameraController`でカメラの高さや角度を変えると、ビルボードは常にカメラの方を完全に向くため傾いたり回転して見えます。特定の軸を固定したい場合は`SetBillboardLock`を使います。

```cpp
// 水平方向(Y軸)だけカメラに向く、昔ながらの「立て看板」ビルボード(X/Zの傾きは固定)
sprite->SetBillboardLock(true, false, true);
```

- 引数は`(lockX, lockY, lockZ)`。trueにした軸はカメラに合わせて回転せず、固定されたままになる
- 3軸すべてtrueにすると、ビルボード自体が実質的に無効になり常に一定の向きで表示される
- `Player`/`Enemy`は既定でX/Zをロック(水平方向のみカメラに追従)しています。実例として参考にしてください
- カメラにロール(Z軸回転)が無いため、`lockZ`は今のところ見た目に影響しません(将来カメラがロールを持った時のために用意してあります)

### MeshRenderer(3D)

```cpp
auto* meshRenderer = cube->AddComponent<MeshRenderer>(Mesh::CreateCube());
meshRenderer->SetTexture(L"Assets/box.png");
meshRenderer->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
XMFLOAT4 color = meshRenderer->GetColor();   // 今の色(SpriteRendererと同じ)
```

用意されている形状:
- `Mesh::CreateCube()` — 単位立方体(-0.5〜0.5)
- `Mesh::CreateSphere(rings, segments)` — 半径0.5のUV球(緯度・経度の分割数を指定可能)
- `Mesh::LoadOBJ(filepath)` — `.obj`ファイル(Wavefront OBJ)を読み込む

```cpp
auto* meshRenderer = obj->AddComponent<MeshRenderer>(Mesh::LoadOBJ(L"Assets/model.obj"));
```

`LoadOBJ`は`v`(頂点座標)/`vt`(UV座標)/`vn`(法線)/`f`(面)に対応しています。四角形以上の面は自動で三角形に分割されます。`vn`が無いファイルは、三角形ごとに2辺の外積から面法線を自動計算するので、法線無しのモデルでもとりあえず陰影は付きます。

これら以外の形状を追加したい場合は、同じ形式(`std::vector<MeshVertex>`を返す静的関数)で頂点データを作る関数を`Engine/Render3D/Mesh.h/.cpp`に足してください(`Game/Objects/Cube.cpp`や`Sphere.cpp`が実例)。

**スカイボックス**: `Game/Objects/Skybox.cpp`が実例です。カメラを中心に追従する(回転はしない)巨大なメッシュを使った、遠景の作り方のパターンとして参考にしてください。

「必ず一番奥に描画される背景」を作る時は、深度テストで他のオブジェクトと競合させるのではなく、次の2つを組み合わせてください(スカイボックスの実装と同じ):

```cpp
skybox->SetDrawPriority(INT_MIN);      // 必ず一番最初に描画する(小さいほど先)
meshRenderer->SetDepthWrite(false);    // 深度バッファに書き込まない(後の描画を一切邪魔しない)
```

これにより、深度バッファの精度やカメラの位置・向きに一切依存しない、Z-fightingが原理的に起こらない背景になります(以前は深度テストに任せていたため、特定のカメラ角度でスカイボックスに隙間ができる不具合がありました)。

一つのGameObjectには、基本的に`SpriteRenderer`か`MeshRenderer`の**どちらか片方**を付けます。

**見た目の設定(マテリアル)**: 色・画像・光沢・自発光・半透明などは `Material` にまとめて持っています(`meshRenderer->GetMaterial()`で手直しできます。`SetColor`/`SetTexture`/`SetUnlit`/`SetDepthWrite` はその項目を書き換える短い書き方です)。詳しくは §30 を見てください。

**内部実装メモ**: シェーダー・サンプラー・ブレンド/深度/ラスタライザステート・定数バッファは、`Mesh`クラス全体で1つだけ作成して全インスタンスで共有しています(Cube/Sphere/Skyboxを何体作っても、シェーダーのコンパイルなどは初回の1回だけ)。頂点バッファ(形状データ)は作る時に1回だけ送る変更不可のバッファで、インスタンスごとに別です(描く度の頂点のコピーはしません。§30)(テクスチャは`Resources`で同じ画像どうし共有しています。§11)。特に意識する必要はありませんが、`Engine/Render3D/Mesh.h/.cpp`を触る時は「インスタンスごとの値」と「共有する値(static)」を混同しないよう注意してください。

### ライティング(3Dメッシュの陰影)

`MeshRenderer`は、`Image::GetLight()`(シーン共通の平行光源)を使って自動的に陰影を計算します。特に何もしなくても、法線が正しく設定されたメッシュ(`CreateCube`/`CreateSphere`/`LoadOBJ`)は陰影が付いた見た目になります。

```cpp
// 光の向き・色・環境光を変えたい場合
Light& light = Image::GetLight();
light.direction = { 0.5f, -1.0f, 0.5f }; // 光が進んでいく方向
light.color     = { 1.0f, 1.0f, 1.0f };  // 光の色・強さ
light.ambient   = { 0.25f, 0.25f, 0.25f }; // 陰になっている面の最低限の明るさ
```

スカイボックスなど「カメラの内側から見る巨大な球」のように、光源の影響を受けると不自然になるオブジェクトは、`SetUnlit(true)`で常に一定の明るさにできます(`Skybox.cpp`が実例)。

```cpp
meshRenderer->SetUnlit(true);   // 中身は meshRenderer->GetMaterial().lit = false と同じ
```

光る点(ハイライト)や自分で光る色は、マテリアルで付けられます(§30)。ランタンのように周りを照らす光(ポイントライト)や、懐中電灯のように円すい形に照らす光(スポットライト)は、`LightComponent` で足せます(§31)。物が地面などに落とす影は、`Shadow` で付けられます(§32)。

---

## 5. Camera

2D用と3D用で**別インスタンス**です(`Image::GetCamera()` / `Image::GetCamera3D()`)。2D用はプレイヤー追従などで動かすことが多く、3D用は`Image::BeginFrame()`内で2Dカメラの動きに自動で追従するようになっています(手動で動かす必要はありません)。

```cpp
Image::GetCamera().position.x = player->transform.position.x;
Image::GetCamera().position.y = player->transform.position.y;
```

オブジェクトを追いかけさせる時は、上のように毎フレーム書く代わりに`CameraFollowComponent`を付けます。画面を揺らす`CameraShake`、ワールド座標と画面座標の変換(`Image::WorldToScreen2D`など)も含めて、§14にまとめてあります。

`position.y`は2D・3D共通で**上方向が+**です(標準的な数学と同じ、Mesh/Sprite/当たり判定すべて統一)。マウス座標は2種類あります:

| 関数 | 中身 | 使いどころ |
|---|---|---|
| `Input::GetMouseWorldPosition()` | ワールド座標(2D用カメラの位置を考慮済み、Y+=上方向) | クリック判定・マウスの方を向くなど、2Dオブジェクトと比較する時(Sample2Dの`PlayerController`/`ButtonComponent`が実例) |
| `Input::GetMousePosition()` | 画面中央からのピクセル数(カメラ無関係、Y+=下方向のまま) | ドラッグ量など画面上での動きを見る時 |

`GetMousePosition()`は、ウィンドウの中(クライアント領域)での位置を、ゲーム画面の解像度(`Graphics::width/height`)の単位に直して返します。ウィンドウ枠のぶん、実際のウィンドウの中はゲーム画面の解像度より少し小さく、画面はそこへ縮めて表示されているためです(以前は枠を含む幅の半分を中心にしていたので、数ピクセル〜十数ピクセルずれていました)。

マウスの移動量とカーソルの固定(FPSのような視点操作用):

| 関数 | 中身 |
|---|---|
| `Input::GetMouseDelta()` | 前のフレームからのマウスの移動量(`GetMousePosition()`と同じ単位、Y+=下方向)。固定中でもそうでなくても使える。固定した直後・ウィンドウがアクティブに戻った直後のフレームは0 |
| `Input::SetCursorLocked(bool)` / `Input::IsCursorLocked()` | カーソルの固定(Unityの`Cursor.lockState = CursorLockMode.Locked`に相当)。固定中はカーソルを隠し、毎フレーム画面の中心へ戻す。別のウィンドウを選んだ・最小化した時は自動で解除される |

「ESCで解除・クリックで再固定」のようなルールはEngineには無いので、使う側(Sample3Dの`CameraController`が実例)で決めます。固定したComponentが破棄される時は、`Uninit()`で`SetCursorLocked(false)`に戻してください。

`GetMouseWorldPosition()`は、マウス座標(Yを反転して上方向に合わせたもの)を`Image::ScreenToWorld2D`(§14)でワールド座標に直しています。結果は「カメラ位置 + マウス座標」で、2Dオブジェクトの`z`には関係なく、そのオブジェクトとぴったり一致します。カメラを動かす処理(`CameraFollowComponent`など)がある場合は、カメラを動かした**後**で呼ぶと、そのフレームのカメラ位置で計算されます。画面の揺れ(`CameraShake`)は入りません。

**2D/3Dのスクロール速度について**: 2D(UI・2Dモード)は正射影(zに関係なく同じ大きさ・一定速度でスクロール)、3D(Mesh・3Dモードのスプライト)は本物の透視投影(近いものほど速く、遠いものほどゆっくり動く)なので、根本的に仕組みが違います。`Camera.focalLength`(既定500)の距離にある3Dオブジェクトだけは、2Dと同じ速度で動くように`fovY`を自動調整していますが、それ以外の距離にあるオブジェクトは2Dとズレます(これは正しい遠近感なので、バグではありません)。

### CameraController(三人称/一人称カメラ)

`Game/Components/CameraController.h/.cpp`は、Ownerを追いかける3D専用のカメラです。`Player.cpp`で実例としてアタッチしています。

```cpp
player->AddComponent<CameraController>();
```

- `TAB`キー: 三人称 ⇔ 一人称を切り替え
- マウスを動かす: 視点を回転(カーソルは隠れて、画面中心に固定される。`Input::SetCursorLocked`/`GetMouseDelta`を使用)
- `ESC`: カーソルの固定を解除(カーソルが表示され、視点は回らなくなる)。ゲーム画面を左クリックすると再び固定(そのクリックでは弾は出ない)
- 別のウィンドウを選ぶと固定は解除される(戻ってきたら左クリックで再固定)。タイトルに戻る時(Component破棄時)にカーソルは元に戻る
- `IsCursorLocked()` / `IsLockedThisFrame()`: カーソルを固定中か / このフレームの左クリックで固定したか。Sample3Dの`PlayerController`が、弾を撃ってよいかの判定に使っている
- `GetYaw()`: 今の水平方向の視点角度(ラジアン。0で+Z方向、増えると右へ回る)。Sample3Dの`PlayerController`が、向きとWASD移動の基準に使っている(`CreatePlayer()`の中で`PlayerController::Setup()`にポインタを渡している)

`Image::GetCamera3D()`を直接操作するため、有効な間は`Image::BeginFrame()`による2Dカメラへの自動追従(前述)を自動で止めます(`Image::SetCamera3DAutoSync(false)`。Component破棄時に自動でtrueへ戻ります)。MeshRendererだけでなく、3Dモード(`SpriteMode::World3D`)の`SpriteRenderer`(ビルボード)もこのcamera3Dを使うので、両方とも同じカメラの動きに正しく追従します。**UI・2Dモードの`SpriteRenderer`は2Dカメラで動き、3Dカメラの操作の影響を受けません。**

---

## 6. Collider(当たり判定)

**当たり判定は、2D/3D問わず全てPhysX(`RigidbodyComponent`、§23参照)で計算しています。**2Dのゲームでは、2D向けに使いやすくした`SquareRigidbodyComponent`/`CircleRigidbodyComponent`を使い、`SetFreezePositionZ(true)`で奥行き方向に動かないようにします(Sample2Dが実例)。

```cpp
// Playerの例(すり抜けない円形・半径50。Dynamic+速度指定で動かし、衝突応答はPhysXに任せる)
auto* rigidbody = player->AddComponent<CircleRigidbodyComponent>(BodyType::Dynamic, 50.0f, 100.0f);
rigidbody->SetUseGravity(false);
rigidbody->SetFreezeRotation(true);
rigidbody->SetFreezePositionZ(true);

// Bulletの例(すり抜ける円形。Enemyに当たったことだけ分かればよい)
bullet->AddComponent<CircleRigidbodyComponent>(BodyType::Kinematic, 8.0f, 1.0f, /*isTrigger*/ true);
```

反応する側は、Componentに以下をoverrideします:

```cpp
// isTrigger = true の時
void OnTriggerEnter(Collider2D* other) override;  // 入った瞬間
void OnTriggerStay(Collider2D* other) override;   // 入ったフレームの次から、出るまで毎フレーム
void OnTriggerExit(Collider2D* other) override;   // 出た瞬間

// isTrigger = false の時(押し返しも自動で行われる)
void OnCollisionEnter(CollisionInfo info) override;  // info.otherで相手のColliderが取れる
void OnCollisionStay(CollisionInfo info) override;
void OnCollisionExit(CollisionInfo info) override;
```

相手のタグは `other->GetTag()` (Trigger) / `info.other->GetTag()` (Collision) で判定します(`Collider2D`は当たり判定の計算自体は持たない、コールバック用の軽いハンドルです)。

**`OnTriggerStay` が呼ばれるタイミング**
- 入ったフレームは `OnTriggerEnter` だけで、**次のフレームから**、出る(`OnTriggerExit`)まで毎フレーム呼ばれます(Unityと同じ順番)。トリガー側と入ってきた側の両方に呼ばれます
- ポーズ中(`Time::SetTimeScale(0.0f)` で、PhysXの計算が止まっている間)は呼ばれません
- どちらかが非アクティブ(`ObjectPool` に戻った)・`Destroy()` 済みの間は呼ばれません。プールに戻した物をまた使った時は、重なっていれば `OnTriggerEnter` からやり直しになります
- `OnTriggerStay` の中で、相手をプールに戻す・`Destroy()` する・`AddForce` するのは大丈夫です(`AddForce` はその場で効きます)
- PhysXはトリガーの「重なっている間」の知らせを出さないので、`Engine/Physics/Physics.cpp` が Enter で組み合わせを覚えて、Exit で忘れ、`Physics::Update()` の最後に毎回呼んでいます

**注意: トリガー同士が重なっても、トリガーのコールバックは呼ばれません**
- 両方が `isTrigger = true` の物同士が重なっても、`OnTriggerEnter` / `OnTriggerStay` / `OnTriggerExit` は呼ばれません。これはPhysX 4.1の仕様です(`PxShape.h` / `PxSimulationEventCallback.h` に書かれています)。Unityでは、どちらかにRigidbodyがあればトリガー同士でも呼ばれるので、ここはUnityと違います
- トリガーと普通の当たり判定(`isTrigger = false`)の組み合わせは呼ばれます(今のサンプルの「弾(トリガー)と敵」や、ゴールをトリガーにしてPlayerが触れる場合など)
- トリガー同士で重なりを知りたい時の対策:
  1. 片方をトリガーにしない(ただし普通の当たり判定は、ぶつかって止まる/押し返す動きになります)
  2. すり抜けたまま調べたい時は、範囲の側が自分から探す。例: `Physics::OverlapBox(t.position, t.scale, "Bullet", QueryTriggers::Include)`(→「物理の問い合わせと力」の節)

**内部実装メモ**: `Engine/Physics/Physics.cpp`のシミュレーションコールバック(`onContact`/`onTrigger`)が、PhysXの結果をこれらのコールバックに変換しています。Kinematicなオブジェクト同士がぶつかった時の「押し戻し」は、PhysXの自動計算に任せられない(キネマティックは力の影響を受けないため)ので、接触の法線・めり込み量から手動で`Transform.position`をずらしています(`isStaticForPush=true`側は動かさない)。Dynamicなオブジェクト(Sphereなど)が絡む場合は、PhysX自身が正しく押し返すので、この手動処理はスキップされます。

**薄い形状のすり抜けについて**: PhysXは、紙のように薄い形状同士が角(コーナー)でかすめるように当たると、押し返しが弱く貫通してしまうことがあります(以前の`SquareRigidbodyComponent`は奥行き1.0の薄い箱だったため、実際にこれが起きていました)。対策として2つ行っています:
- `SquareRigidbodyComponent`は奥行きも一辺と同じ長さにした(実質的に立方体)。2Dゲームでは`SetFreezePositionZ(true)`で奥行き方向に動かないようにしておけば、厚みがあっても見た目や動きには影響しない
- Dynamicなアクター全てでCCD(移動の軌跡をスイープして判定するモード)を有効にした(`Physics::Init()`でシーン全体のCCDを有効化、`RigidbodyComponent.cpp`の`CreateActor()`でDynamicアクターごとに`PxRigidBodyFlag::eENABLE_CCD`を設定)

**作成直後・再利用直後のテレポートについて**: `RigidbodyComponent`は、作成直後(`AddComponent`直後)と、`ObjectPool`で再利用された直後(`SetActive(true)`)の最初の`Update()`で、PhysX側を今のTransformの位置へテレポートさせ、速度も0に戻します。これにより、「`Rent()`の後で出現位置を入れる」という普通の書き方をしても、Dynamicなら作成時の位置や前回やられた位置に引き戻されず、Kinematicなら作成時の位置からスイープ扱いで移動して途中の物に誤って当たる、ということが起きません。

### Colliderの見た目(デバッグ表示)

実際にPhysXが使っている当たり判定の形・大きさを、枠線で表示できます(見た目のスプライト/メッシュのサイズとズレていないか確認する時に便利です)。

```cpp
RigidbodyComponent::SetDebugDrawEnabled(true);
```

どこかで1回呼ぶだけで(Sample2Dでは`GameManager::Init()`で呼んでいる)、以後その回のプレイ中は全ての`RigidbodyComponent`が、緑(通常の当たり判定)または黄(`isTrigger=true`のすり抜ける判定)の枠を、実際のPhysX形状のサイズで描画します。不要になったら`false`を渡すか、呼び出し自体を消してください。

| Component | 表示される枠 |
|---|---|
| `BoxRigidbodyComponent` | 立方体の12本の辺 |
| `SphereRigidbodyComponent` | 3方向(XY/XZ/YZ)の円 |
| `SquareRigidbodyComponent` | Z軸方向で切った断面の四角 |
| `CircleRigidbodyComponent` | Z軸方向で切った断面の丸 |
| `CapsuleRigidbodyComponent` | 両端の半円2組 + 上下の輪 + 側面の4本の線(§29) |
| `Capsule2DRigidbodyComponent` | XY平面で切った断面(角の丸い四角=スタジアム形。§29) |

2D向けのSquare/Circleは、PhysX上は奥行きのある立方体/球ですが(§23)、2Dゲームでは奥行き方向に動かないので、平面で見た時の形(断面)だけを表示しています。

**2Dの枠は、2Dスプライトと同じ描き方(2Dの正射影)で描きます**(`Mesh::DrawLines2D`)。2Dカメラでスクロールし、zで位置・大きさがずれないので、`z`が0でない物(Sample2Dの壁・敵・弾など)でも、スプライトの輪郭にぴったり重なります。深度テストはしないので、描いた物の上に出ます(UIモードのスプライトは最後に描くので、UIより手前には出ません)。Box/Sphere/Capsuleなど3Dの枠は、今まで通り3Dカメラの透視で描きます。同じく、見張り役の視野(`VisionComponent`)・経路探索の線(`NavGridComponent` / `PathFollowComponent`)も、XY平面(2D)の時は2Dの正射影、XZ平面(3D)の時は3Dカメラの透視です。

**内部実装メモ**: `RigidbodyComponent::Draw()`が、`PxShape`から実際のジオメトリ(`PxBoxGeometry`/`PxSphereGeometry`)を取得し、Ownerの`Transform`の`scale`だけを一時的に差し替えて描画しています(位置・回転は普段どおりOwnerのTransformをそのまま使う)。枠は三角形ではなく線(`D3D11_PRIMITIVE_TOPOLOGY_LINELIST`)で描いていて、形ごとの輪郭データは`Mesh::CreateCubeOutline()`などで作り、全`RigidbodyComponent`で1つずつ共有しています。

**Meshのテクスチャについて**: `Shader3D.hlsl`は必ずテクスチャを読むため、テクスチャを設定していない`Mesh`には、自動で1x1の白いテクスチャを使うようにしています。以前はこれが無く、直前に別のMeshが使ったテクスチャを読んでしまい、色が変わったり透明部分で線が途切れたりしていました。

---

## 7. EventBus(疎結合な通知)

お互いを直接知らないComponent同士で連絡を取り合いたい時に使います。

```cpp
// 聞く側(登録)
EventBus::Get().Subscribe("EnemyDefeated", []() { /* ... */ });
EventBus::Get().SubscribeObject("FireBullet", [](GameObject* shooter) { /* ... */ });

// 知らせる側
EventBus::Get().Publish("EnemyDefeated");
EventBus::Get().PublishObject("FireBullet", owner);
```

**重要**: ラムダの中で`this`(SceneやGameObjectなど)をキャプチャして使う場合、そのSceneが終わる時に必ず`EventBus::Get().Clear()`を呼んでください(`GameScene::Uninit()`を参照)。呼び忘れると、次のシーンで既に壊れたポインタを触りにいくバグになります。

---

## 8. ObjectPool(オブジェクトの使い回し)

弾やエフェクトなど、頻繁に生成・消滅するものに使います(Sample2Dの`Game/Components/BulletManager.cpp`が実例)。

```cpp
ObjectPool pool;

// 借りる(無ければ新規作成される)
GameObject* obj = pool.Rent(scene, [](GameObject* obj) {
    obj->AddComponent<SpriteRenderer>(L"Assets/bullet.png");
    // ここはAddComponentなど「初回だけ」の組み立て処理
});

// 返す(非表示になるだけで、破棄はされない)
pool.Return(obj);
```

**注意**: 使い回されるオブジェクトは、`Init()`が2回目以降は呼ばれません(最初に`AddComponent`された時の1回だけ)。位置や向き、寿命タイマーなどの状態は、`Init()`ではなく「借りた/発射した時に呼ぶ専用の関数」(`BulletController::Fire()`のような)でリセットしてください。

**ObjectPoolは、そのシーンのGameObjectにアタッチしたComponentのメンバとして持たせてください**(Sample2Dの`BulletManager`/`EnemyManager`を参照)。こうするとプールの寿命がシーンと同じになり、シーン終了時にプールの中身と一緒に片付けられます。ファイルのstatic変数のように、シーンをまたいで生かし続けると、シーンが破棄された後もプール内に「もう存在しないGameObjectへのポインタ」が残り、クラッシュします。

---

## 9. 計算系(Math)

方向・距離・角度・乱数の計算は、`Engine/Core/MathUtil.h` の `namespace Math` にまとめてあります。各Componentで `atan2f`/`sqrtf`/`sinf`/`cosf`/`rand()` を直接書かずに、こちらを使ってください(0で割る・角度の向きを間違える、といったミスを防ぐため)。

```cpp
#include "../../../Engine/Core/MathUtil.h"
```

`XMFLOAT2`/`XMFLOAT3` の `+` `-`(2つの和・差)と `* float`(何倍か)も使えるようになります。

| 種類 | 主な関数 |
|---|---|
| ベクトル | `Length` / `Distance` / `DistanceSq`(距離の2乗。大小を比べるだけならこちらが速い) / `Normalize`(長さ1に。長さ0なら0ベクトル) / `DirectionTo(from, to)`(向き。長さ1) / `Dot` / `Cross` / `ToXY`(XMFLOAT3→XMFLOAT2)。`Cross`以外はXMFLOAT2版もある |
| 補間・制限 | `Lerp(a, b, t)`(tは0〜1に制限しない) / `Clamp(value, min, max)` / `MoveTowards(current, target, maxDistance)`(行き過ぎない) |
| 角度(すべて度) | `DeltaAngle`(差。-180〜180) / `MoveTowardsAngle` / `AngleToTarget2D(from, to)` / `AngleToTargetY(from, to)` / `Forward2D(transform)` / `Forward3D(transform)` |
| 乱数 | `RandomRange(min, max)`(小数。整数を渡すとmaxも含む整数) / `RandomOnCircle(radius)`(円周上のランダムな点) |

```cpp
// 2D: マウスの方を向く(Sample2D PlayerController)
XMFLOAT2 playerPos = Math::ToXY(t.position);
t.rotate.z = Math::AngleToTarget2D(playerPos, Input::GetMouseWorldPosition());

// 目的地の方へ進む速度(Engine ChaseComponent。offset=自分から目的地までのずれ)
ApplyVelocity(Math::Normalize(offset) * moveSpeed, dt);

// 敵から離れる向き(Sample2D PlayerControllerのノックバック)
XMFLOAT2 away = Math::DirectionTo(
    Math::ToXY(info.other->owner->transform.position), Math::ToXY(GetOwner()->transform.position));

// 3D: 正面と右(Sample3D PlayerController)
XMFLOAT3 forward = Math::Forward3D(t);
XMFLOAT3 right = Math::Cross({ 0.f, 1.f, 0.f }, forward);

// Playerの周りのランダムな位置(Sample2D/3D EnemyManager)
XMFLOAT2 offset = Math::RandomOnCircle(spawnDistance);
```

**角度の向きの決まり**:
- **2D**(`rotate.z`): `rotate.z = 0` で上(Y+)を向き、増えると左回り。`Forward2D` が正面の向き、`AngleToTarget2D` は「その点の方を向く `rotate.z`」で、お互いに逆の関係になっている
- **3D**(`rotate.y`、水平の向き): `rotate.y = 0` で+Zを向き、増えると+X(右)側へ回る。正面は `(sin, 0, cos)`(`CameraController::GetYaw()` と同じ)。`Forward3D` は `rotate.y` だけを使う(上下の向き `rotate.x` は無視)。右は `Math::Cross({0,1,0}, 正面)`
- どちらも角度は**度**。ラジアンが必要な時は `XMConvertToRadians()` で変換する

---

## 10. 時間とタイマー(Time / Timer)

「1フレームの時間」「時間の倍率(スロー・ポーズ)」は `Engine/Core/Time.h` の `namespace Time`、「○秒たったら」を数えるのは `Engine/Core/Timer.h` の `Timer` でできます。各Componentで `timer -= dt; if (timer <= 0) ...` のような処理を手書きせずに、こちらを使ってください。

### Time(1フレームの時間・スロー・ポーズ)

```cpp
#include "../../../Engine/Core/Time.h"
```

| 関数 | 内容 |
|---|---|
| `Time::GetDeltaTime()` | 1フレームの時間(秒)。倍率の影響を受ける(ポーズ中は0)。各Componentの `Update(float dt)` の `dt` と同じ値 |
| `Time::GetUnscaledDeltaTime()` | 1フレームの本当の時間(秒)。倍率の影響を受けない。ポーズ中も動かしたいUIなどに使う |
| `Time::GetTime()` | 起動してからの時間(秒)。倍率の影響を受ける |
| `Time::SetTimeScale(scale)` | 時間の倍率。1=通常、0.3=スロー、0=ポーズ(負の値は0になる) |
| `Time::GetTimeScale()` | 今の倍率 |
| `Time::IsPaused()` | ポーズ中か(倍率が0の時true) |

`Time::Init()` / `Time::Update()` はEngine(`GameEngine`)だけが呼ぶ関数です。ゲーム側からは呼ばないでください。

```cpp
Time::SetTimeScale(0.3f);   // スローにする
Time::SetTimeScale(0.0f);   // ポーズ
Time::SetTimeScale(1.0f);   // 元に戻す
```

**決まり**:
- **1フレームの時間には上限(0.1秒)があります。** ウィンドウをドラッグしている間などはゲームが止まっていて、その分を1フレームでまとめて進めると、物がすり抜けたりワープしたりするためです。シーンを切り替えた直後(読み込みで1フレームが長くなった時)なども、`dt` は最大0.1秒になります。上限は倍率をかける前の時間にかかるので、`GetUnscaledDeltaTime()` も0.1秒を超えません。
- **時間の倍率はシーンをまたいで残ります。** ポーズ中(倍率0)のままシーンを切り替えると、次のシーンも止まったままになるので、切り替える前に `Time::SetTimeScale(1.0f)` で元に戻してください。なお、演出付きのシーン切り替え(§16)の間は時間が止まり、終わると切り替える前の倍率に戻ります。
- **ポーズ中も、Engineは各Componentの `Update` を止めません**(`dt` が0で呼ばれ続けます。PhysXの物理演算は止まります)。そのため、何もしないとポーズ中でもキー入力で弾が出てしまいます。**入力を止めたいControllerは、`Update` の先頭(入力を読む前)で次のように書いてください。**
  ```cpp
  void PlayerController::Update(float dt) {
      if (Time::IsPaused()) return;   // ポーズ中は操作を受け付けない
      ...
  }
  ```
  両サンプルの`PlayerController::Update`に書いてあります(今のサンプルにポーズ機能は無いので、効くのはシーン切り替えの演出中です。§16)。Sample3Dでは、一人称の時にPlayerを隠す処理(カメラはポーズ中もTABで切り替えられるため)より後に置いています。

### Timer(○秒たったら)

```cpp
#include "../../../Engine/Core/Timer.h"
```

Componentのメンバに持って、`Update` で毎フレーム `dt` を渡します。

| メンバ・関数 | 内容 |
|---|---|
| `Timer(duration, loop)` | 作る。`duration`=何秒で時間が来るか、`loop`=繰り返すか(省略すると1秒・繰り返さない) |
| `Update(dt)` | 時間を進める。**時間が来たフレームだけtrue**。`loop` なら余った時間を次の周へ持ち越す。`loop` でなければ、一度trueを返した後はfalseのまま(0秒のタイマーは次の `Update` で1回だけtrue) |
| `Reset()` | 最初から数え直す |
| `Complete()` | すぐに「終わった状態」にする(繰り返す(`loop`)タイマーで呼んだ場合は、終わった状態にはならず、次の `Update` で時間が来る(trueが返る)) |
| `GetProgress()` | 進み具合(0〜1) |
| `IsFinished()` | 終わったか(`loop` なら常にfalse) |
| `elapsed` | 経過時間(秒)。直接書き換えてもよい(最初の1回だけ早くしたい時など) |

```cpp
// 一定間隔でくり返す
Timer spawnTimer{ 2.5f, true };                      // 2.5秒ごと
spawnTimer.elapsed = 2.5f - 1.0f;                    // 最初の1回だけ1.0秒で来るようにする(Initで)
if (spawnTimer.Update(dt)) Spawn(...);               // 時間が来たフレームだけ出す
// ※Sample2D/3Dの敵の出現(EnemyManager)は、時間と数を表で調整できるように、Timerではなく表(§25)の時間で出している

// きっかけがあってから○秒の間(Sample2D/3D PlayerControllerの無敵時間)
Timer invincibleTimer{ 1.0f };
invincibleTimer.Complete();                          // Init: 最初は無敵ではない(終わった状態)
invincibleTimer.Update(dt);                          // Update: 毎フレーム進める
if (!invincibleTimer.IsFinished()) return;           // 無敵中はダメージを受けない
invincibleTimer.Reset();                             // ダメージを受けたら数え始める

// 寿命(Sample2D/3D BulletController)
if (lifeTimer.Update(dt)) pool->Return(GetOwner());  // 2秒たったらプールに返す
```

スプライトシートのコマ送り(歩きアニメーションなど)は、Timerで手書きせずに `SpriteAnimationComponent`(§17)を使ってください。

**注意**: `ObjectPool` で使い回すオブジェクトのタイマーは、借りた/発射した時に呼ぶ関数(`BulletController::Fire()` など)で `Reset()` してください(§8 と同じ理由。`Init()` は2回目以降は呼ばれません)。

---

## 11. リソース(Resources、画像の使い回し)

同じ画像を何度も読み込まないための仕組みです(`Engine/Core/Resources.h` の `namespace Resources`)。同じパスの画像は1回だけ読み込み、「何人が使っているか」(参照カウント)を数えて管理します。敵を100体出しても、`player.png` を読み込むのは1回だけです。

**`SpriteRenderer` / `MeshRenderer` は中で自動で使っているので、ゲーム側は普段意識しなくてかまいません**(`AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2)` の書き方はそのまま)。自分で画像を読んで描くComponent(Sample2D `BackgroundController`、Sample3D `GroundController` など)だけが、直接使います。

```cpp
#include "../../../Engine/Core/Resources.h"

// Init: 画像を借りる(読み込み済みなら読み直さずに同じIDが返る)
texID = Resources::LoadTexture(L"Assets/grid.png");

// Draw: 描く(2Dの場合。色とコマは自分で渡す。省略すると白・1枚絵)
Image::Draw(transform, texID);
Image::Draw(transform, texID, { 1.f, 0.f, 0.f, 1.f }, { 8, 2, 5 });  // 赤、横8×縦2に分けた5コマ目

// Uninit: 借りた画像を返す
Resources::ReleaseTexture(texID);
```

| 関数 | 内容 |
|---|---|
| `Resources::LoadTexture(path)` | 画像を借りる。同じパスは2回目以降読み込まずに同じIDを返す(`/` と `\`、大文字と小文字の違いは同じ画像として扱う)。失敗したら `UINT_MAX` |
| `Resources::ReleaseTexture(id)` | 借りた画像を返す(参照カウントを1減らす) |
| `Resources::UnloadUnused()` | 誰も使っていない画像をまとめて解放する。シーン切り替えの時に `SceneManager` が自動で呼ぶ |

**決まり**:
- **参照カウントが0になっても、すぐには解放されません。** 次のシーン切り替えまで残ります。弾や敵のように出たり消えたりするものが、その度に画像を読み直さないようにするためです。
- シーン切り替えでは、新しいシーンの `Init()` が終わった後に `UnloadUnused()` が呼ばれます。両方のシーンで使う画像(ボタンなど)は、読み直さずにそのまま使い回されます。
- `LoadTexture` で借りたら、必ず `Uninit()` などで `ReleaseTexture` を返してください。返し忘れると、その画像はゲームを終了するまで解放されません(終了時にはまとめて解放され、Visual Studioの出力ウィンドウに「返し忘れの画像があります」と表示されます)。
- 画像は共有されるので、**色やコマ番号は画像ではなく、描く側(`SpriteRenderer` など)が1体ごとに持ちます。** `Image::Draw` には、色とコマ(`SpriteSheet` = 横の分割数・縦の分割数・コマ番号)を毎回渡します。
- `Image::LoadTexture` / `Image::ReleaseTexture` はEngine内部用(`Resources` だけが使う)です。ゲーム側からは呼ばないでください(呼ぶ度に同じ画像を読み直してしまいます)。
- 音(WAV)も同じ仕組みで使い回せます(`Resources::LoadSound` / `ReleaseSound`)。ただし普段は `Audio::PlayOneShot` や `AudioSourceComponent` が中で使うので、直接呼ぶことはありません(§24)。FBXモデルも同じ考え方で使い回せます(`Resources::LoadModel`。`ModelRenderer` が中で使うので、直接呼ぶことはありません。§27)。

---

## 12. イージング(Ease / TweenComponent)

「最初は速く、最後はゆっくり止まる」「少し行き過ぎてから戻る」のような気持ちいい動きを、1行で付けられます。

- `Engine/Animation/Ease.h` の `namespace Ease`: 動きの「速さの変化」を決める関数(イージング関数)
- `Engine/Animation/TweenComponent.h`: オブジェクトの位置・大きさ・回転・色(と好きな値)を、イージングを付けて目標値まで動かすComponent

```cpp
#include "../../../Engine/Animation/TweenComponent.h"   // Ease.h も一緒に読み込まれる
```

### Ease(イージング関数)

すべて `float 関数名(float t)` の形で、進み具合 `t`(0〜1)を受け取って0〜1の値を返します(t=0で0、t=1で1)。`t` が0〜1の外で渡された時は、0〜1に収めてから計算します(t<0は0、t>1は1)。

| 関数 | 動き |
|---|---|
| `Ease::Linear` | 一定の速さ |
| `Ease::InQuad` | ゆっくり始まる |
| `Ease::OutQuad` | ゆっくり止まる(TweenComponentの既定) |
| `Ease::InOutQuad` | ゆっくり始まって、ゆっくり止まる |
| `Ease::OutCubic` | OutQuadより強くゆっくり止まる |
| `Ease::OutBack` | 少し行き過ぎてから戻る(途中で1を超える。最大1.1くらい) |
| `Ease::OutElastic` | ばねのように揺れて止まる(途中で0〜1をはみ出す) |
| `Ease::OutBounce` | ボールのように跳ねて止まる |

計算式は [easings.net](https://easings.net/ja) と同じなので、動きはそこで見比べられます。イージング関数を変数に入れる時の型は `EaseFunction` です。TweenComponentを使わずに、自分で補間する時にも使えます:

```cpp
float t = timer.GetProgress();                            // 0〜1
float y = Math::Lerp(startY, endY, Ease::OutBounce(t));   // 跳ねながら止まる
```

### TweenComponent(動きを付ける)

```cpp
auto* tween = obj->AddComponent<TweenComponent>();
tween->MoveTo({ 0.f, 100.f, 0.f }, 1.0f);                         // 1秒で(0,100)へ動く(既定はEase::OutQuad)
tween->ScaleTo({ 2.f, 2.f, 1.f }, 0.5f, Ease::OutBack);           // 0.5秒で2倍に(少し行き過ぎて戻る)

// SetDelay などは「直前に始めた動き」に効く。自分自身を返すので、つなげて書ける
tween->ColorTo({ 1.f, 0.f, 0.f, 1.f }, 0.3f)                      // 0.3秒で赤くする
     .SetDelay(0.5f)                                              // 0.5秒待ってから
     .SetOnComplete([this] { GetOwner()->Destroy(); });           // 終わったら消える

// 好きな値を動かす(毎フレーム今の値が渡される)
tween->TweenValue(0.f, 1.f, 2.0f, [this](float v) { alpha = v; }, Ease::Linear);
```

| 関数 | 内容 |
|---|---|
| `MoveTo(目標, 秒, イージング)` | 位置(`transform.position`)を動かす |
| `ScaleTo(目標, 秒, イージング)` | 大きさ(`transform.scale`)を変える |
| `RotateTo(目標, 秒, イージング)` | 回転(`transform.rotate`、度)を変える。数値をそのまま補間するので、350度→10度は逆回りに340度回る(近い方へ回したい時は `Math::DeltaAngle` で目標を計算する) |
| `ColorTo(目標, 秒, イージング)` | 色を変える。`SpriteRenderer` があればその色、無ければ `MeshRenderer` の色。どちらも無ければ何もしない(出力ウィンドウに表示される) |
| `TweenValue(開始, 目標, 秒, 関数, イージング)` | 好きな値を動かす。毎フレーム、今の値を関数に渡す |
| `SetDelay(秒)` | 動き出すまで待つ時間 |
| `SetLoop(loop, pingPong)` | 繰り返す。`pingPong=true` なら「行って戻る」を1周として繰り返す。`SetLoop(false, true)` なら1回だけ行って戻って終わる |
| `SetUnscaled(true)` | ポーズ中(`Time::SetTimeScale(0)`)・スロー中も、本当の時間(`Time::GetUnscaledDeltaTime()`)で動く。ポーズ画面のUIなどに使う。既定(false)は `Update` の `dt` と同じ時間で動く |
| `SetOnComplete(関数)` | 終わった時に呼ぶ関数 |
| `Stop()` | すべての動きを、今の値のまま止める(OnCompleteは呼ばない) |

**決まり**:
- **開始値は「動き出した時の今の値」です**(`SetDelay` を付けた時は、待ち時間が終わった時の値)。`TweenValue` だけは引数の「開始」から始まります。
- 位置・大きさ・回転・色・TweenValueは**同時に動かせます**。**同じ種類の動き(位置なら位置)を2つ始めると、前の動きは消えて後から始めた方だけが動きます**(前の動きのOnCompleteは呼ばれません)。`TweenValue` は毎回別の値を動かすものなので上書きせず、いくつでも同時に動かせます。
- 秒数が0以下の動きは、次のフレームですぐ1周終わった状態になります。普通の動きは目標値になって終わります(OnCompleteも呼ばれる)。`SetLoop(false, true)`(1回だけ行って戻る)は開始値に戻って終わります。`SetLoop(true, ...)` は終わりません。
- `SetLoop(true, ...)` の動きは終わらないので、**OnCompleteは呼ばれません**。止める時は `Stop()` を呼びます。
- 終わった動きは自動で消えます。
- 位置・大きさ・回転は `transform` の値(親がいれば親からの相対)をそのまま書き換えます。**PhysXのDynamicな剛体(`RigidbodyComponent`)が付いたオブジェクトには `MoveTo` / `RotateTo` を使わないでください**(PhysXの位置で上書きされます。Dynamicは `SetVelocity()` で動かす決まりです)。
- GameObjectが非アクティブ(`SetActive(false)`)の間は、動きも止まります。

**コールバックの安全性**:
- Componentなので、オブジェクトが消えると一緒に消えます。`SetOnComplete` のラムダが、**消えたオブジェクトを触る心配がありません**(`EventBus` と違い、自分で `Clear()` しなくてよい)。ただし、ラムダの中で**別の**オブジェクトのポインタを使う時は、そのオブジェクトが先に消えていないか気を付けてください。
- `SetOnComplete` や `TweenValue` の関数の中で、同じTweenComponentに新しい動きを始めても、`Stop()` しても、オブジェクトを `Destroy()` しても大丈夫です。新しく始めた動きは次のフレームから動きます。`Destroy()` された後は、残りの動きは進みません。

```cpp
// 動きを順番につなげる(右へ動いて、終わったら上へ)
tween->MoveTo({ 100.f, 0.f, 0.f }, 0.5f).SetOnComplete([tween] {
    tween->MoveTo({ 100.f, 100.f, 0.f }, 0.5f);
});

// ポーズ中もふわふわ上下し続ける(行って戻るを繰り返す)
tween->MoveTo({ 0.f, 20.f, 0.f }, 0.8f, Ease::InOutQuad).SetLoop(true, true).SetUnscaled(true);
```

**実例**: 両サンプルのタイトル画面のSTARTボタン(`Game/Objects/StartButton.cpp`)は、大きさ0から本来の大きさへ `Ease::OutBack` で0.4秒かけて大きくなって出ます。ボタンのクリック判定(`ButtonComponent`。§20)も `transform.scale` を使っているので、大きくなる途中でも見た目と判定が合っています(ボタンの文字の大きさも、ボタンの高さに合わせて一緒に大きくなります)。

---

## 13. 移動(MoveComponent ほか)

「目的地(相手・場所)へ向かって動く」処理をComponentにしたものです。敵・弾・ミサイルなどで使い回せます。

```cpp
#include "../../../Engine/Behavior/ChaseComponent.h"   // 使うComponentのヘッダーを読む(MoveComponent.h も一緒に読み込まれる)

auto* chase = enemy->AddComponent<ChaseComponent>();
chase->SetTarget(player);          // 追いかける相手
chase->SetSpeed(60.f);             // 速さ(1秒に進む距離)
chase->SetPlane(MovePlane::XZ);    // 3Dの地面の上を動く時(2Dなら不要。既定はXY)
```

| Component | 動き |
|---|---|
| `ChaseComponent` | 一定の速さでまっすぐ向かう |
| `PathFollowComponent` | 壁・障害物を避けて、一定の速さで向かう(経路探索つきの `ChaseComponent`。両サンプルの敵。§36) |
| `AccelerateChaseComponent` | 加速しながら向かう。勢いがあるので行き過ぎて戻ってくる |
| `HomingComponent` | 曲がれる角度に上限がある追尾(ミサイル・車・魚向け)。急に向きを変えない |

`ChaseComponent` / `PathFollowComponent` / `AccelerateChaseComponent` / `HomingComponent` のどれも、親クラス `MoveComponent` の次の関数が使えます(`MoveComponent` 単体ではAddComponentしません)。

| 関数 | 内容 |
|---|---|
| `SetTarget(相手)` | 追いかける相手(`nullptr` で外す) |
| `SetTargetPosition(位置)` | 決まった場所へ向かう(相手がいない時に使う) |
| `SetSpeed(速さ)` | 速さ(1秒に進む距離。既定100)。`AccelerateChaseComponent` では最高速度 |
| `SetStopDistance(距離)` | これより近づいたら止まる(既定1.0 = ほぼ重なったら止まる) |
| `SetPlane(MovePlane::XY / XZ)` | 動く平面(2D=XY、3Dの地面=XZ。既定XY) |
| `HasArrived()` | 着いたか(目的地が `SetStopDistance` より近いか) |

| Component | 自分だけの関数 |
|---|---|
| `AccelerateChaseComponent` | `SetAcceleration(強さ)`(1秒に速さがいくつ増えるか。既定200)、`SetMaxSpeed(最高速度)`(`SetSpeed` と同じ)、`SetDrag(空気抵抗)`(0で減速しない。大きいほどすぐ減速する。既定0) |
| `HomingComponent` | `SetTurnSpeed(度)`(1秒に何度まで曲がれるか。既定180)、`SetFaceMovement(true)`(見た目の向き `rotate` も進行方向に合わせる) |

**決まり**:
- 目的地は、`SetTarget` の相手がいればその位置、いなければ `SetTargetPosition` の位置です。**どちらも無ければ止まります**(速度0)。
- 相手が非アクティブ(`SetActive(false)`・`Destroy()` 済み)の間は、追わずに止まります。ただし、相手が `Destroy()` されて**本当に消えた後**は、ポインタが消えた物を指したままになり見分けられません。相手を消す時は、先に追いかけている側で `SetTarget(nullptr)` してください。
- 距離・向きは動く平面の上だけで考えます(XYならZの差、XZならYの差は無視)。
- 動かし方は、付いている `RigidbodyComponent` で変わります。
  - **Dynamic**: `SetVelocity()` で速度を渡す(Dynamicは `transform` を直接書き換えない決まりのため)。XZの時は、Y方向の速度(重力で落ちる速さ)を今の値のまま残します。XYの時はZ方向の速度を0にします
  - **Kinematic・Rigidbody無し**: `transform.position` を直接動かす
  - **Static**: 動かさない(Visual Studioの出力ウィンドウに1回だけ知らせる)
- `ObjectPool` で使い回された時(`SetActive(true)`)は、速度・進行方向などの状態をリセットします。相手・速さなどの設定は残るので、出す度に `SetTarget` などで入れ直してください(Sampleの `EnemyManager::Spawn` が実例)。
- Dynamicの剛体と一緒に使う時は、`RigidbodyComponent` の**後に** `AddComponent` してください(Componentは追加順に `Update` されるので、PhysXの結果が反映された位置で計算できる)。
- `HomingComponent` の進行方向は `rotate` と同じ向きです(XYは `rotate.z` = `Math::Forward2D`、XZは `rotate.y` = `Math::Forward3D`)。最初の進行方向は、出した時の `rotate` の向きです(出す時に向きを決めておけば、その向きへ飛び出してから曲がっていく)。`SetFaceMovement(true)` をDynamicの剛体で使う時は、`SetFreezeRotation(true)` にしておきます(PhysXの回転で上書きされないように)。
- `AccelerateChaseComponent` は、Dynamicなら `AddForce(..., ForceMode::Acceleration)`(§23)で加速します(ぶつかって跳ね返った勢いも残る)。1フレーム分の加速だけ、最高速度を少し超えることがあります。

**実例**: 両サンプルの `EnemyManager` が、敵に `PathFollowComponent`(`ChaseComponent` に経路探索を足したもの。§36)を付けて(`SetupEnemy`)、出す時に追いかける相手(Player)・速さ・地図を渡しています(`Spawn`)。Sample3Dは `SetPlane(MovePlane::XZ)` で、地面の上を重力で落ちながら追いかけます。`ChaseComponent` はそのまま残してあり、壁を避けなくてよい時(ごく近くの敵など)に使えます。

---

## 14. カメラ(CameraFollowComponent / CameraShake / 座標の変換)

カメラの追従・画面の揺れ・座標の変換を、毎回手書きしなくて済むようにしたものです。カメラそのもの(2D用`Image::GetCamera()`と3D用`Image::GetCamera3D()`の違い)は§5を参照してください。

```cpp
#include "../../../Engine/Render/CameraFollowComponent.h"   // 追従
#include "../../../Engine/Render/CameraShake.h"             // 揺れ
#include "../../../Engine/Render/Image.h"                   // 座標の変換
```

### CameraFollowComponent(2Dカメラで追いかける)

追いかけてほしいオブジェクト(Playerなど)に付けると、2D用カメラ(`Image::GetCamera()`)がそのオブジェクトを追いかけます。動かすのはカメラの`position`のx,yだけです。

```cpp
auto* follow = player->AddComponent<CameraFollowComponent>();
follow->SetSmoothTime(0.15f);                                  // 少し遅れてなめらかに追う(既定0=ピッタリ)
follow->SetBounds({ -2000.f, -2000.f }, { 2000.f, 2000.f });   // ステージの外を映さない
follow->SetDeadZone({ 200.f, 100.f });                         // 画面中央の200x100の中で動いてもカメラは動かない
```

| 関数 | 内容 |
|---|---|
| `SetOffset({x, y})` | 対象からどれだけずらして映すか(例: `{0, 100}` で対象より上を映す=対象は画面の少し下に見える) |
| `SetSmoothTime(秒)` | 追いつくまでの時間。0(既定)はピッタリ追従。この秒数で残りの距離が約5%になる。フレームレートが変わっても同じ動きになる(指数的に近づく式) |
| `SetBounds(左下, 右上)` | **画面に映してよい範囲**(Z=0の平面のワールド座標)。画面の端がこの外に出ないところでカメラが止まる(カメラの中心ではなく、画面の半分の大きさも考えて止める)。範囲が画面より狭い向きは、範囲の真ん中に固定する |
| `SetDeadZone({幅, 高さ})` | この大きさの四角(対象がいつも映る位置が中心)の中で対象が動いても、カメラは動かさない。四角から出た分だけ動く(画面酔い対策) |

**決まり**:
- **付ける順番に注意**: Componentは付けた順に`Update`されます。**DynamicのRigidbodyより後**(PhysXの結果が反映された位置で追う)、**`Input::GetMouseWorldPosition()`を使うComponentより前**(画面に見えているマウスの位置とずれない)に`AddComponent`してください(Sample2Dの`Objects/Player.cpp`が実例: 物理 → カメラ追従 → `PlayerController`)。
- 最初の`Update`と、`SetActive(true)`で戻った時は、なめらかに追わずに一気に合わせます(遠くから流れてこない)。
- 対象の位置は、親がいてもワールド座標(`GetWorldTransform()`)で追います。
- 対象が非アクティブの間はカメラはその場で止まります。ポーズ中(`Update`の`dt`が0)も止まります。
- 2つ以上のオブジェクトに付けると、後から`Update`された方が勝ちます(1つだけに付ける)。
- 3D用カメラは動かしません(3Dのカメラ操作は、今はSample3Dの`CameraController`が担当)。ただし`Image::SetCamera3DAutoSync`がtrue(既定)なら、3Dカメラは2Dカメラに自動で付いてきます(§5)。
- シーンを切り替えてもカメラの位置は残ります。Sample2Dでは`TitleController::Init()`が原点に戻しています。

### CameraShake(画面を揺らす)

```cpp
CameraShake::Shake(6.f, 0.2f);   // 強さ6で0.2秒揺らす
```

- 強さは「1方向に最大どれだけずれるか」(2Dはピクセル、3Dはワールドの単位)。揺れは時間とともに弱まって(`Ease::OutQuad`)、指定した秒数で0になります。
- **2D用・3D用の両方のカメラ**が揺れます。3Dカメラは、カメラから見た右・上の向きに、`position`と`target`を同じだけずらします(向きは変わらない)。
- **カメラの`position`は変えません。描画している間だけずらして、描き終わったら元の値に戻します**(`GameEngine::Run`の`Image::BeginFrame()`の後〜`Image::EndFrame()`の前)。そのため`Update`の中で読むカメラの値・`Input::GetMouseWorldPosition()`・`CameraFollowComponent`の計算には揺れが入らず、追従とぶつかったり、カメラが元の位置に戻らなくなったりしません。
- `Draw`の中で読むカメラの値(`Image::WorldToScreen2D`など)には揺れが入ります。オブジェクトに重ねて描く文字(ボタンの文字など)は、絵と一緒に揺れます。画面の決まった位置に描く文字(HUD)は揺れません。
- 揺れの時間はポーズ中も進みます(`Time::GetUnscaledDeltaTime()`)。
- **揺れている途中にまた`Shake`を呼んだ時**: 今の揺れの強さ(弱まった後の値)と比べて、新しい方が強いか同じなら、新しい揺れに置き換えます(最初から数え直す)。弱ければ無視します(強い揺れが、後から来た弱い揺れで消されない)。
- `CameraShake::IsShaking()` で揺れている途中か分かります。
- `CameraShake::Stop()` で揺れをすぐに止められます(描画の途中で呼ばれても、カメラはちゃんと元の位置に戻ります)。
- **シーンを切り替えると、揺れは止まります**(前のシーンの揺れが次のシーンに残らない)。止めるのは古いシーンの`Uninit`の後・新しいシーンの`Init`の前なので、新しいシーンの`Init`で`Shake`した揺れは消えません。

**実例**: Sample2Dの`PlayerController::OnCollisionStay()`で、敵に当たってダメージを受けた時に揺らしています(強さ・時間は`PlayerController.h`の`damageShakeStrength`/`damageShakeDuration`)。

### 座標の変換(ワールド ⇔ 画面)

**画面座標は「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」**(`Text::Draw`と同じ)なので、結果をそのまま`Text::Draw`に渡せます。

| 関数 | カメラ | 内容 |
|---|---|---|
| `Image::WorldToScreen2D(ワールド座標)` | 2D用 | ワールド座標 → 画面座標(`XMFLOAT2`)。UIモードの`SpriteRenderer`と同じ計算(「ワールド座標 - カメラ位置」。**zは大きさにも位置にも影響しない**) |
| `Image::ScreenToWorld2D(画面座標)` | 2D用 | 画面座標 → ワールド座標(`XMFLOAT3`、zは0)。`WorldToScreen2D`の逆。`Input::GetMouseWorldPosition()`の中身 |
| `Image::WorldToScreen3D(ワールド座標)` | 3D用 | ワールド座標 → 画面座標(`XMFLOAT3`)。`MeshRenderer`と同じ透視投影。x,yが画面座標、**zはカメラからの奥行き**。**zが0以下ならカメラの後ろ**(画面に映らない。x,yは0で使えない)なので、使う前に必ず`z > 0`を確かめる |
| `Image::ScreenPointToRay3D(画面座標, origin, direction)` | 3D用 | 画面の点から奥へ伸びる線(`origin`=手前の描画限界(nearZ)の面の上の点、`direction`=長さ1の向き)。`Physics::Raycast`と組み合わせると、画面のその点に映っている物を探せる |

```cpp
// 2D: 敵の頭の上に文字を出す(Drawの中で)
XMFLOAT2 p = Image::WorldToScreen2D(enemy->transform.position);
Text::Draw(L"!", p.x, p.y + 60.f, 30.f);

// 3D: 敵の頭の上に文字を出す(カメラの後ろにいる時は出さない)
XMFLOAT3 q = Image::WorldToScreen3D(enemy->transform.position);
if (q.z > 0.f) Text::Draw(L"HP", q.x, q.y + 40.f, 24.f);

// 3D: マウスの下にある物を探す(GetMousePositionはY+=下なので、Yの符号を反転して渡す)
XMFLOAT2 mouse = Input::GetMousePosition();
XMFLOAT3 origin, direction;
Image::ScreenPointToRay3D({ mouse.x, -mouse.y }, origin, direction);
RaycastHit hit;
if (Physics::Raycast(origin, direction, 5000.f, hit, "Enemy")) { /* hit.object がマウスの下の敵 */ }
```

- **`Input::GetMousePosition()`はY+=下**なので、これらの関数に渡す時はYの符号を反転してください(`GetMouseWorldPosition()`は反転済み)。
- `Draw`の中で呼ぶと`CameraShake`の揺れが入ったカメラで、`Update`の中で呼ぶと揺れの入らないカメラで計算します。
- 2D用カメラ(正射影。zで大きさが変わらない)と3D用カメラ(透視投影)は計算の仕方が違うので、関数の名前で`2D`/`3D`を分けています。

**実例**: 両サンプルの`ButtonComponent::Draw()`(Engine)が、ボタンの文字の位置を`Image::WorldToScreen2D`で求めています。

---

## 15. 画面エフェクト(ScreenEffect)

被弾・回復・シーン切り替えなどで、画面全体に色の演出を重ねます。

```cpp
#include "../../../Engine/Render/ScreenEffect.h"
```

| 関数 | 内容 |
|---|---|
| `ScreenEffect::Flash(色, 秒)` | 画面全体が一瞬光る。色の`w`の濃さから始まり、指定した秒数でだんだん0になる |
| `ScreenEffect::Vignette(色, 秒, 回数)` | 画面の**端だけ**が色付きで点滅する(中央は透明)。指定した秒数の間に、指定した回数だけ点滅して消える。色の`w`が一番濃い時の濃さ |
| `ScreenEffect::Fade(色, 目標の濃さ, 秒, 終わった時の処理)` | 暗転・明転。**今の濃さから**目標の濃さ(0〜1)へ、指定した秒数で変える。1で塗りつぶしたまま止まり、その後0にすると消える。色は`x,y,z`だけを使う(`w`は使わない)。最後の「終わった時の処理」は省略できる |
| `ScreenEffect::IsFading()` | `Fade`の濃さが変わっている途中なら`true`(暗転し終わって止まっている間は`false`) |
| `ScreenEffect::Stop()` | 今出ている`Flash`と`Vignette`をすぐに消す。`Fade`は消さない(今の濃さも「終わった時の処理」もそのまま) |

```cpp
// 敵に当たった時: 画面の端を赤く、0.5秒で2回点滅させる
ScreenEffect::Vignette({ 1.f, 0.f, 0.f, 0.6f }, 0.5f, 2);

// 回復した時: 画面全体を緑っぽく光らせる
ScreenEffect::Flash({ 0.3f, 1.f, 0.3f, 0.5f }, 0.3f);

// ポーズ画面を開いた時: 0.3秒で半分くらいまで暗くする(閉じる時は目標の濃さ0で戻す)
ScreenEffect::Fade({ 0.f, 0.f, 0.f, 1.f }, 0.5f, 0.3f);
```

**シーンを切り替える時の暗転・明転は、§16 の `FadeTransition` を使ってください**(1行で「暗転 → 切り替え → 明転」まで済み、二重押しも防げます)。STARTボタンも今はこちらを使っています:

```cpp
// STARTボタンを押した時: 0.5秒で暗転 → GameSceneへ切り替え → 0.5秒で明転
SceneManager::Get().ChangeScene<GameScene>(FadeTransition());
```

**決まり**:
- **文字(`Text::Draw`)も含めて、画面の一番上に描きます。** 暗転すると、HUDやボタンの文字も一緒に隠れます(`GameEngine::Run`で、シーンの描画が全部終わった後に描いている)。画像ファイルは使わず、Direct2D(文字と同じ仕組み)で描いています。
- 3つのエフェクトは別々に動くので、同時に出せます。重なる順番は下から **Vignette → Flash → Fade**(Fadeが一番上)。
- 同じエフェクトを途中でまた呼ぶと、新しい方に置き換わります(最初から数え直す)。
- **シーンを切り替えると、`Flash`と`Vignette`は消えます**(被弾の赤い点滅などが次のシーンに残らない)。**`Fade`の濃さと進み具合は残ります。** 暗転したまま次のシーンへ切り替えて、次のシーンで明転する、という使い方ができます。逆に、暗転したまま明転を忘れると、次のシーンが真っ暗のままになるので注意してください。
- 消すのは古いシーンの`Uninit`の後・新しいシーンの`Init`の前なので、新しいシーンの`Init`で始めた明転や`Flash`は消えません。
- **時間はポーズ中も進みます**(`Time::GetUnscaledDeltaTime()`)。ポーズ画面への暗転などにも使えます。
- 秒数が0以下の時は、すぐに目標の値になります(`Flash`/`Vignette`は何も出さない、`Fade`はすぐにその濃さになり、終わった時の処理もその場で呼ぶ)。
- `Fade`の「終わった時の処理」は1回だけ呼ばれます。呼ばれるのは毎フレームの最初の方(`ScreenEffect::Update`。シーンの`Update`より前)なので、中で`ChangeScene`してよく、同じフレームのうちに新しいシーンに切り替わります(切り替えた瞬間に前のシーンが一瞬見えることはない)。変わり終わる前に次の`Fade`を呼ぶと、前の`Fade`の「終わった時の処理」は呼ばれません。
- **`Fade`が変わり終わる前にシーンが切り替わると、「終わった時の処理」は呼ばれずに捨てられます**(濃さの変化は最後まで続く)。そのため、ラムダに`this`などシーンのオブジェクトのポインタを入れても、消えたシーンのオブジェクトを触ってしまうことはありません。ただし、これはシーンを切り替えた時だけの話です。**同じシーンの中で`Destroy()`されるオブジェクトの`this`は入れないでください**(`Fade`が終わる前にそのオブジェクトが消えると、終わった時の処理はそのまま呼ばれるので、消えたオブジェクトを触ってしまう)。逆に「シーンが変わっても必ず呼んでほしい処理」は、終わった時の処理には書けないので注意してください。
- 「終わった時の処理の中で`ChangeScene`する」使い方もできます(ただしシーン切り替えの演出には、§16の方が簡単です)。終わった時の処理は呼ばれた時点で取り出されているので、その後のシーンの切り替えで捨てられることも、二重に呼ばれることもありません。ただし、その中でもう一度「終わった時の処理」付きの`Fade`を始めると、すぐ後のシーンの切り替えで捨てられます(次のシーンで続けたい演出は、次のシーンの`Init`で始めてください)。

**実例**:
- 両サンプルの`PlayerController::OnCollisionStay()`: ダメージを受けた時に`Vignette`(色・時間・回数は`PlayerController.h`の`damageVignetteColor`/`damageVignetteDuration`/`damageVignetteBlinkCount`)
- STARTボタンの暗転・明転は、§16の`FadeTransition`に置き換えました(`ScreenEffect::Fade`はシーン切り替え以外の演出用に残しています)

---

## 16. シーン切り替え(SceneTransition)

シーンを切り替える時に、暗転や画像のアニメーションで画面を隠します。どの切り替え方も「**画面を覆う → シーンを切り替える → 開く**」の順に進みます。

```cpp
#include "../../../Engine/Core/SceneManager.h"   // これだけで3種類とも使える
```

| 書き方 | 内容 |
|---|---|
| `SceneManager::Get().ChangeScene<GameScene>()` | **瞬時**。今まで通り、演出なしですぐに切り替える |
| `SceneManager::Get().ChangeScene<GameScene>(FadeTransition())` | **フェード**。黒で0.5秒かけて暗転 → 切り替え → 0.5秒かけて明転 |
| `SceneManager::Get().ChangeScene<GameScene>(FadeTransition({ 1.f, 1.f, 1.f, 1.f }, 1.f))` | フェードの色と時間を変える(この例は白で、暗転・明転とも1秒)。色の`w`は使わない(必ず全部塗りつぶす) |
| `SceneManager::Get().ChangeScene<TitleScene>(SpriteTransition(L"Assets/transition_circle.png", 8, 2, 0.04f))` | **スプライト**。1枚の画像に並べたコマ(この例は8列×2行=16コマ)を、1コマ0.04秒で、画面全体に引き伸ばして再生する。覆う時はコマ0→最後のコマへ順に、開く時は最後のコマ→コマ0へ逆に再生する |
| `SpriteTransition(L"Assets/close.png", 8, 2, 0.04f).SetOpenSheet(L"Assets/open.png", 4, 2, 0.05f)` | 開く時だけ別の画像を使う(こちらも逆再生) |
| `SpriteTransition(L"Assets/transition_wipe.png", 8, 2, 0.04f).SetSwitchFrame(7)` | **切り替えるコマを決める**。逆再生はせず、コマ0→最後のコマを順に1回だけ再生して、途中のコマ7でシーンを切り替える(下の「SetSwitchFrame」) |

```cpp
// STARTボタンを押した時(両サンプルの Objects/StartButton.cpp)
SceneManager::Get().ChangeScene<GameScene>(FadeTransition());

// ゲームオーバー中にSPACEを押した時(Sample2D の GameManager::Update): 黒い丸が閉じて、また開く
SceneManager::Get().ChangeScene<TitleScene>(SpriteTransition(L"Assets/transition_circle.png", 8, 2, 0.04f));

// ゲームオーバー中にSPACEを押した時(Sample3D の GameManager::Update): 黒い帯が左から入って覆い、右へ抜けていく
SceneManager::Get().ChangeScene<TitleScene>(SpriteTransition(L"Assets/transition_wipe.png", 8, 2, 0.04f).SetSwitchFrame(7));
```

**SetSwitchFrame(コマ番号)**: 「左から入って右へ抜ける」のように、開く時が覆う時の逆再生にならない演出に使います。

| 段階 | 出すコマ | 時間 | 例: 16コマ・1コマ0.04秒・`SetSwitchFrame(7)` |
|---|---|---|---|
| 覆う | コマ0 → 指定したコマ | (指定したコマ+1)コマ分 | コマ0〜7、0.32秒 |
| シーンを切り替える | 指定したコマ(全部覆っている) | — | 切り替えた瞬間はコマ7が出ている |
| 開く | 指定したコマの次 → 最後のコマ | (最後のコマ−指定したコマ)コマ分 | コマ8〜15、0.32秒 |

- 最後のコマが終わると演出が終わります(最後のコマは全部透明にしておく)。
- 指定できるのは **0〜(コマ数−2)** です。範囲の外(マイナスや、最後のコマ以上)を指定すると、範囲の中に収めて、出力ウィンドウに`★ SpriteTransition::SetSwitchFrame: ...`と出ます。最後のコマで切り替えると開く段階が0コマになり、新しいシーンがいきなり見えてしまうためです。
- `SetOpenSheet`と一緒に使うと`SetSwitchFrame`が優先され、`SetOpenSheet`の画像は使われません(出力ウィンドウに1回知らせます)。
- `SetSwitchFrame`を使わなければ、今まで通り(覆う=順再生、開く=逆再生)です。

**決まり**:
- **切り替え中(覆っている間・開いている間)は、ゲームの時間が止まります**(`Time::GetTimeScale()`が0、`Time::IsPaused()`がtrue。§10)。演出そのものは`Time::GetUnscaledDeltaTime()`で進みます。開き終わると、切り替える前の倍率に戻ります(切り替える前がスロー0.3なら0.3に戻る)。
  - 切り替え中に`Time::SetTimeScale()`を呼ぶと、すぐには変わらず(止まったまま)、開き終わった時にその倍率になります(最後に呼んだ値)。新しいシーンの`Init`で`Time::SetTimeScale(1.0f)`と書いておけば、明転し終わってから通常の速さになります。
  - ポーズ中と同じく、各Componentの`Update`は`dt`が0で呼ばれ続けます。キー入力で弾が出ないようにしたいControllerは、§10の`if (Time::IsPaused()) return;`を書いてください(両サンプルの`PlayerController`は書いてあるので、切り替え中は移動・向き・発射を受け付けません)。
- **切り替え中に`ChangeScene`をもう一度呼んでも無視されます**(引数あり・なしの両方。出力ウィンドウに`★ SceneManager::ChangeScene: ...`と出ます)。ボタンを連打しても二重に切り替わりません。演出なしの`ChangeScene`も、呼んでから実際に切り替わるまで(次のフレームの最初まで)の間に呼んだ物は無視されます。新しいシーンの`Init`の中で呼んだ`ChangeScene`も無視されるので注意してください。
- **文字(`Text::Draw`)と画面エフェクト(§15)も含めて、一番上に描きます**(暗転するとHUDの文字も隠れる)。画面エフェクトと同じく Direct2D で描いています。
- シーンが切り替わるのは、**画面を全部覆い終わったフレーム**です。切り替えの処理は演出なしの時と同じです(古いシーンの`Uninit` → 画面エフェクト・揺れの後片付け → 新しいシーンの`Init` → 使わなくなった画像の解放)。切り替わる瞬間は覆われているので見えません。
- 時間を0以下にすると、一瞬で切り替わります。

**スプライトシートの作り方**:
- 1枚の画像に、コマを左上から右へ、行の終わりまで行ったら次の行へ、の順に並べます(`SpriteRenderer`のコマ番号と同じ)。
- **最後のコマで、画面全体が覆われるように描いてください**(最後のコマが出ている間にシーンを切り替えます)。コマ0は全部透明にすると、元の画面から自然につながります。透明を使うので、PNGで保存します。
- 開く時は同じ画像を逆に再生するので、「閉じていく絵」を1つ作れば「開いていく絵」も自動でできます。`SetOpenSheet`で別の画像にする時も、同じ決まり(最後のコマで全部覆う)で作ります。
- `SetSwitchFrame`を使う画像は決まりが違います。**指定するコマで画面全体を覆い、最後のコマは全部透明に**します(コマ0も全部透明にすると、元の画面から自然につながります)。
- 各コマは画面全体に引き伸ばすので、1コマの縦横比を画面と同じ(16:9。例: 320×180)にするとゆがみません。
- 画像は`Assets/`に置けば、ビルドの時に自動で出力先にコピーされます。一度読み込んだ画像は使い回し、ゲームの終了時に解放されます(`Resources`の画像とは別の仕組み)。読み込めなかった時は出力ウィンドウに知らせて、何も描かずに時間どおりに切り替えます。

**見本**:
- `Assets/transition_circle.png`(8列×2行、1コマ320×180)。コマ0は全部透明で、コマが進むほど黒が外側から広がり、真ん中の透明な丸が小さくなっていき、最後のコマで全部黒になります。
- `Assets/transition_wipe.png`(8列×2行、1コマ320×180。`SetSwitchFrame(7)`用)。コマ0は全部透明で、黒い帯が左から入ってきてコマ7で全部黒になり、その後は黒が右へ抜けていき、最後のコマ(15)で全部透明になります。

**実例**:
- 両サンプルの`Objects/StartButton.cpp`: 押したら`FadeTransition`で`GameScene`へ
- Sample2Dの`GameManager::Update()`: ゲームオーバー中にSPACEで、`SpriteTransition`(丸く閉じて開く)で`TitleScene`へ
- Sample3Dの`GameManager::Update()`: ゲームオーバー中にSPACEで、`SpriteTransition(...).SetSwitchFrame(7)`(左から覆って右へ抜ける)で`TitleScene`へ

---

## 17. 2Dアニメーション(SpriteAnimationComponent)

スプライトシートのコマを、時間に合わせて切り替える(パラパラ漫画にする)Componentです。タイマーを手書きせずに、歩き・待機・攻撃などのアニメーションを作れます。同じGameObjectの`SpriteRenderer`のコマ番号(`SetSpriteIndex`)を、毎フレーム書き換えます。

```cpp
#include "../../../Engine/Animation/SpriteAnimationComponent.h"

// 組み立て時(工場関数・SetupEnemyなど)。SpriteRendererを先に付けておく
auto* anim = obj->AddComponent<SpriteAnimationComponent>();
anim->AddClip("Walk", { 4, 5 }, 0.3f, true);    // 名前, コマ番号の並び, 1コマの秒数, 繰り返すか
anim->AddClip("Idle", { 4 }, 1.0f, true);       // 1コマだけなら止め絵
anim->AddClip("Attack", { 8, 9, 10 }, 0.1f, false);   // 1回だけ
anim->Play("Walk");
```

| 関数 | 内容 |
|---|---|
| `AddClip(名前, コマの並び, 1コマの秒数, 繰り返すか)` | アニメーション(クリップ)を登録する。いくつでも登録できる。同じ名前で登録し直すと上書き |
| `Play(名前)` | 再生する。**今と同じ名前なら何もしない**。別の名前なら、そのクリップの最初のコマから |
| `Stop()` | 止める(コマは今のまま残る)。次の `Play` は、同じ名前でも最初から |
| `IsFinished()` | 繰り返さないクリップが、最後のコマまで行って止まったか(繰り返すクリップは常にfalse) |
| `SetSpeed(速さ)` | 再生の速さ。1=通常、2=倍速、0.5=半分、0=一時停止(負の値は0になる。逆再生はしない) |
| `SetRandomStart(true)` | 始まるコマと経過時間をランダムにする(何体も並んだ敵の足踏みがそろわないように)。繰り返すクリップだけに効く。**`Play` の前に呼ぶ** |
| `GetCurrentClip()` | 再生中のクリップの名前(止まっていれば空の文字列) |

**役割分担**: このComponentは「コマを送る」だけです。**どのアニメーションを再生するかは、Controllerが `if` で決めます**(Unityの「Animator」のような状態遷移図は作りません。`if` の方が読みやすいため)。

```cpp
// Controller::Update(毎フレーム呼んでよい。同じ名前なら最初に戻らない)
if (moving) anim->Play("Walk");
else        anim->Play("Idle");

// 1回だけのアニメーションが終わったら、次のアニメーションへ
if (anim->GetCurrentClip() == "Attack" && anim->IsFinished()) anim->Play("Idle");
```

**決まり**:
- **`Play` に今と同じ名前を渡した時は何もしません。** Controllerは毎フレーム `Play("Walk")` を呼ぶので、その度に最初のコマに戻ると、アニメーションが止まって見えるためです。繰り返さないクリップを同じ名前でもう一度最初から再生したい時は、`Stop()` してから `Play` します(別の名前のクリップを挟んだ後なら、そのまま `Play` で最初から)。
- 登録されていない名前の `Play` は何もしません(今のクリップのまま)。Visual Studioの出力ウィンドウに、名前ごとに1回だけ `★ SpriteAnimationComponent::Play: ...` と出ます。
- コマの並びが空のクリップと、1コマの秒数が0以下のクリップは登録されません(出力ウィンドウに知らせます)。止め絵にしたい時は、コマを1つだけにします。
- 繰り返さないクリップは、**最後のコマを1コマ分の時間見せ終わったら**止まり(最後のコマのまま)、`IsFinished()` がtrueになります。
- 1フレームの時間(`dt`)が大きくて何コマも進む時も、進んだコマ数だけ送ります(余った時間は次のコマへ持ち越すので、速さがずれません)。繰り返さないクリップは最後のコマで止まります。
- 時間は `Update` の `dt` で進みます。ポーズ中(§10)・シーン切り替え中(§16)は止まります。GameObjectが非アクティブの間も止まります。
- **`ObjectPool` で使い回された時(`SetActive(true)`)は、今のクリップを最初のコマからやり直します**(前回の続きから始まらないように)。`SetRandomStart(true)` なら、ランダムなコマからやり直します。借りる側で何かを呼ぶ必要はありません。
- `SetRandomStart(true)` は、繰り返すクリップだけに効きます。繰り返さないクリップ(攻撃・やられなど)は、必ず最初のコマから始まります。
- `SetRandomStart(true)` は **`Play` の前に**呼んでください。ランダムにするのは `Play` した時(と再利用された時)なので、`Play` の後に呼ぶと、最初の再生はランダムになりません(次の `Play` や再利用からランダムになります)。
- `SpriteRenderer` の後に `AddComponent` してください(`Play` した時に、すぐそのコマにするため)。同じGameObjectに `SpriteRenderer` が無ければ何もしません(出力ウィンドウに1回知らせます)。

**実例**: Sample3Dの `EnemyManager` の `SetupEnemy` で、敵に `AddClip("Walk", { 4, 5 }, 0.3f, true)`・`SetRandomStart(true)`・`Play("Walk")` をしています(コマ4と5を0.3秒ごとに切り替える歩きアニメーション)。プールで再利用された敵も、自動でアニメーションをやり直します。

---

## 18. セーブ/ロード(SaveData)

ハイスコア・音量などを保存して、次に起動した時も残るようにする仕組みです。使い方はUnityの `PlayerPrefs` と同じで、`Set○○` / `Get○○` で読み書きして、`Save` でファイルに書きます。

```cpp
#include "../../../Engine/Data/SaveData.h"

// 書く・読む
SaveData::SetInt("HighScore", 1200);
int highScore = SaveData::GetInt("HighScore");          // 無ければ0
float volume  = SaveData::GetFloat("MusicVolume", 0.8f); // 無ければ0.8
SaveData::SetString("PlayerName", L"ゆうしゃ");

// すぐにファイルへ書きたい時(ハイスコアを更新した時など)
SaveData::Save();
```

| 関数 | 内容 |
|---|---|
| `SetInt(キー, 値)` / `GetInt(キー, 無い時の値 = 0)` | 整数を書く / 読む |
| `SetFloat(キー, 値)` / `GetFloat(キー, 無い時の値 = 0)` | 小数を書く / 読む(無限大・NaNは書かない) |
| `SetString(キー, 値)` / `GetString(キー, 無い時の値 = L"")` | 文字を書く / 読む(日本語・改行・`=` もそのまま使える) |
| `HasKey(キー)` | そのキーがあるか |
| `DeleteKey(キー)` | そのキーを消す(無ければ何もしない) |
| `Save(スロット = 0)` | ファイルに書き出す。成功したらtrue(中身が16MBより大きい時は書かずにfalse) |
| `Load(スロット = 0)` | ファイルから読み込む(今の値はすべてファイルの中身に置き換わる)。読めなかったらfalse(今の値はそのまま) |

**自動で読み込み・保存します**:
- 起動した時(`GameEngine::Init`)に、Engineが自動で `Load(0)` します。最初のシーンの `Init` から、もう `Get○○` で読めます
- 終了した時(`GameEngine::Uninit`)に、**書き換えていれば**自動で `Save` します(ゲーム側で `Save` を呼び忘れても消えません)。保存先は、最後に `Load` / `Save` に成功したスロットです(普段は0。`Load(1)` した後なら `save1.txt`)
- ただし、起動時の `Load(0)` が読めなかった時(ほかのアプリが `save0.txt` を開いている・16MBより大きいなど。出力ウィンドウに `★ SaveData: 読み込めませんでした...` と出ます)は、そのファイルを中途半端な中身で上書きしないように、**終了時の自動保存はしません**(`Load` / `Save` に1回でも成功すれば、自動保存するようになります)
- **`Load` がfalseだった後に、ゲーム側から `Save` を呼ぶと、読めなかったファイルを今の値で上書きします**(読めなかったファイルにあった他のキーは消えます)
- ただし、ゲームが落ちた時・Visual Studioでデバッグを止めた時は、終了時の保存は行われません。**大事な値(ハイスコアなど)は、変えた時に `Save()` を呼んでください**

**保存先**: `%LOCALAPPDATA%\3D_FreamWork\save0.txt`(スロット1なら `save1.txt`)。エクスプローラーのアドレス欄に `%LOCALAPPDATA%\3D_FreamWork` と打つと開けます。exeの隣(`x64\Debug\`)にしないのは、ビルドし直したりフォルダを消したりすると消えることがあるためです。**Sample2DとSample3Dは同じファイルを使います**(ハイスコアは `"Sample2DHighScore"` / `"Sample3DHighScore"` のように**サンプルごとに別のキー**にして、別々に記録しています。同じファイルを使う別のゲームとキーの名前がぶつからないように気を付けてください)。中身を消して最初からやり直したい時は、ゲームを閉じてから `save0.txt` を消します。

**ファイルの形式**(メモ帳で開いて確認・書き換えできます):
```
HighScore=1200
MusicVolume=0.8
PlayerName=ゆうしゃ
```
- UTF-8(BOM付きで書きます。手で書き換えた時はBOMが無くても読めます)。1行に「キー=値」で、キーの順に並びます
- 値は、最初の `=` の後ろから行の終わりまでです(値の中の `=`、先頭・末尾の空白もそのまま残ります)。値の中の `\`・改行は、`\\`・`\n` と書かれます(CRは `\r`)
- 空の行と `#` で始まる行は飛ばします(`#` はメモ用ですが、`Save` し直すと消えます)。「=」が無い行・キーに使えない文字がある行などの**壊れた行は飛ばして**、残りの行を読みます(Visual Studioの出力ウィンドウに、何行目かが出ます)
- 保存する時は、いったん一時ファイル(`save0.txt.tmp`)に書いてから本物と差し替えます。保存に失敗しても・保存中に落ちても、前のセーブデータは壊れません(失敗した時は出力ウィンドウに `★ SaveData: 保存できませんでした...` と出て、`Save` がfalseを返します)
- ファイルの大きさの上限は16MBです。16MBより大きいファイルは読まず(`Load` がfalse)、書く中身が16MBより大きくなる時は書きません(`Save` がfalse。次の起動で読めないファイルを作らないため)

**キーの名前の決まり**:
- EventBusのイベント名と同じく、**PascalCaseの英語**にします(`"HighScore"`, `"MusicVolume"`)
- 使える文字は英字・数字・`_` だけです。それ以外(空白・日本語・`=` など)を含むキーは保存されず、`Get○○` はいつも「無い時の値」を返します(出力ウィンドウに知らせます)
- 同じキーをいろいろな場所で書くと打ち間違いに気付きにくいので、キーの文字は1つのファイルにまとめます(Sampleの `GameManager.cpp` の `highScoreKey` のように)

**型の決まり**: 値は型を覚えずに文字で持ち、`Get○○` の時に変換します。
- 同じキーに別の型で書くと、後に書いた値で上書きされます
- `GetInt` は、整数として読めない値(`"1.5"`、`"abc"`、intに入らない大きな数)なら「無い時の値」を返します(小数を切り捨てたりしません。出力ウィンドウに1回知らせます)
- `GetFloat` は、数として読めない値なら「無い時の値」を返します(`SetInt` で書いた `100` は 100.0 として読めます)
- `GetString` は、どの型で書いた値も文字のまま返します(`SetInt("A", 5)` なら `L"5"`)

**実例**: 両サンプルの `GameManager`。ゲームオーバーになった時(`"PlayerDied"` を受けた時)に、スコアがハイスコアより大きければ `SetInt(highScoreKey, score)` して `Save()` します(キーはSample2Dが `"Sample2DHighScore"`、Sample3Dが `"Sample3DHighScore"`)。HUDは `GameManager::GetHighScore()`(中で `SaveData::GetInt(highScoreKey)` を読むだけ)で、スコアの下にハイスコアを出します(値の持ち主は `SaveData` で、HUDは読むだけ。§22)。

---

## 19. テキスト(Text / TextStyle / TextRenderer)

画面に文字を描く仕組みです(`Engine/UI/Text.h`、`Engine/UI/TextRenderer.h`)。座標は **「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」** です(§14の座標の変換と同じ)。`Text::Draw` は Component の `Draw()` の中で呼びます。

**今までの書き方も、そのまま使えます**(見た目も今までと同じ。文字の中心が (x, y) に来ます):
```cpp
#include "../../../Engine/UI/Text.h"

Text::Draw(L"GAME OVER", 0.f, 40.f, 64.f, { 1.0f, 0.2f, 0.2f, 1.0f });   // 文字, x, y, 大きさ, 色, フォント名(省略するとArial)
```

左寄せ・縁取り・太字・自分で用意したフォントを使いたい時は、`TextStyle`(文字の見た目のまとめ)を渡します:
```cpp
TextStyle style;
style.size = 28.f;
style.fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");   // L"Noto Sans JP" が返る
style.align = TextAlign::Left;                     // x を文字の左端にする
style.outlineColor = { 0.f, 0.f, 0.f, 1.f };       // 黒い縁取り
Text::Draw(L"Score: 100", -560.f, 320.f, style);
```

| 関数 | 内容 |
|---|---|
| `Text::Draw(文字, x, y, style)` | スタイルを指定して描く |
| `Text::Draw(文字, x, y, 大きさ = 32, 色 = 白, フォント名 = L"Arial")` | 今までの書き方(中心に合わせる。縁取り・太字なし) |
| `Text::Measure(文字, style)` | 描いた時の幅と高さ(`XMFLOAT2`。縁取りの分は入らない)。寄せ方(align)では変わらない。`Update` の中で呼んでもよい |
| `Text::LoadFont(ファイルのパス)` | フォントファイル(.ttf/.otf)を読み込み、`style.fontName` に入れる名前を返す。読めなかったら空の文字列(出力ウィンドウに `★ Text::LoadFont: ...` と出る) |

**TextStyle の中身**(書かなかった項目は右の値になります):

| 項目 | 既定 | 内容 |
|---|---|---|
| `size` | `32` | 文字の大きさ(ピクセル) |
| `color` | 白 `{1,1,1,1}` | 文字の色。`w` は濃さで、下げると縁取りも一緒に薄くなる |
| `fontName` | `L"Arial"` | フォントの名前(`LoadFont` が返した名前か、PCにインストール済みのフォント名)。空なら Arial |
| `align` | `TextAlign::Center` | 横の寄せ方。`Left`: x が文字の**左端**、`Center`: x が**中心**、`Right`: x が**右端**。縦はいつも中心 |
| `bold` | `false` | 太字 |
| `outlineColor` | `{0,0,0,0}` | 縁取りの色。**`w`(濃さ)が0なら縁取り無し** |
| `outlineWidth` | `2` | 縁取りの太さ(ピクセル) |

- 縁取りは、同じ文字を8方向に `outlineWidth` ずつずらして縁の色で描いてから、上に本体を描いています(1つの文字を9回描くので、たくさん出す時は少し重くなります)
- 右寄せ(`TextAlign::Right`)は、スコアを画面の右上にそろえる時などに使います。数字の桁が増えても右端がそろいます

### フォント(LoadFont と Assets/Fonts)

`fontName` にPCのフォント名(`L"Meiryo"` など)を書くこともできますが、**そのフォントをインストールしていないPCでは、別のフォントで表示されてしまいます**。`Text::LoadFont` で `Assets/Fonts/` のフォントファイルを読み込むと、チーム全員のPCで同じ見た目になります。

- 今は **Noto Sans JP**(日本語と英数字が入ったフォント。太さを変えられる1つのファイル)を `Assets/Fonts/NotoSansJP[wght].ttf` に置いています。`style.bold = true` で太字になります
- 同じファイルは2回読まないので、`LoadFont` は何回呼んでも大丈夫です(Sampleでは、使うComponentの `Init()` で呼んで、返った名前をメンバに取っておいています)
- フォントの読み込みには Windows 10 以降の仕組み(DirectWriteのフォントセット)を使っています。古いWindowsでは読み込みに失敗し、Arialで表示されます
- **フォントにはライセンスがあります。** Noto Sans JP は **SIL Open Font License(OFL)** で、ゲームに入れて配ってよいですが、**ライセンスの文章(`Assets/Fonts/OFL.txt`)を必ず一緒に置きます**。新しいフォントを足す時も、ライセンスを確かめて、ライセンスのファイルを一緒に置いてください
- `Assets/` は、ビルドの時にフォルダごと(`Fonts/` も)出力先へコピーされます(§0)

### TextRenderer(オブジェクトに付いて動く文字)

ダメージの数字・キャラクターの名前など、**GameObjectと一緒に動く文字**を出すComponentです。オブジェクトの位置(ワールド)を画面座標に直して(§14の `Image::WorldToScreen2D`)、`Text::Draw` で描きます。

```cpp
#include "../../../Engine/UI/TextRenderer.h"

auto* text = obj->AddComponent<TextRenderer>();
text->SetText(L"+10");                 // 表示する文字
text->SetStyle(style);                 // 見た目(TextStyle)
text->SetOffset({ 0.f, 60.f, 0.f });   // オブジェクトの位置から、ワールドでずらす量(頭の上など)
```

| 関数 | 内容 |
|---|---|
| `SetText(文字)` / `GetText()` | 表示する文字 |
| `SetStyle(style)` / `GetStyle()` | 見た目 |
| `SetColor(色)` | 文字の色だけ変える(`w` を下げると縁取りも一緒に薄くなる。消えていく演出に使う) |
| `SetOffset(ずらす量)` | オブジェクトの位置からずらす量(ワールド) |
| `SetUse3DCamera(true)` | 3Dのオブジェクトに付ける時に使う。3D用カメラで画面座標に直し(`Image::WorldToScreen3D`)、カメラの後ろにある時は描かない |

- 2Dでは、`SpriteRenderer`(UIモード)と同じく2Dカメラと一緒にスクロールします
- 文字の大きさは、カメラからの距離では変わりません(いつも `size` のピクセル)

**実例**: Sample2Dで敵を倒すと、その位置に「+10」が出て、上に少し動きながら消えます。`Objects/ScorePopup.cpp` の `CreateScorePopup(scene, 位置)` が、`TextRenderer` と `TweenComponent`(§12。`MoveTo` で上へ動かし、`TweenValue` で文字の濃さを1→0にして、`SetOnComplete` で自分を `Destroy`)を付けたオブジェクトを作ります。呼んでいるのは `EnemyController::OnTriggerEnter2D`(倒された時の処理)です。

### 注意
- **文字はいつも画面の一番上に出ます**(Direct2Dで、画像や3Dの描画が全部終わった後に描かれるため)。画像の後ろに文字を隠すことはできません。画面エフェクト(§15)とシーン切り替え(§16)は、文字よりさらに上に出ます
- フォントの設定(フォント名・大きさ・太字の組み合わせ)は、1回作ったら取っておいて使い回します(64種類を超えたら全部作り直します)。毎フレーム大きさを変えるアニメーションをしても大丈夫です

---

## 20. UI(Anchor / DrawRect / GaugeRenderer / SliderComponent / ButtonComponent)

HPバー・音量スライダー・ボタンを、どのゲームでも使える部品にしたものです(`Engine/UI/Anchor.h`、`Engine/UI/GaugeRenderer.h`、`Engine/UI/SliderComponent.h`、`Engine/UI/ButtonComponent.h`、`Engine/Render/Image.h` の `DrawRect`)。画面サイズが変わっても、隅や中央からの距離でずれずに置けます。

### 画面座標の決まり

- 座標は **「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」** です(§14の座標の変換、§19の `Text::Draw` と同じ)。下へずらす時はYをマイナスにします。
- **`Input::GetMousePosition()` はY+=下**なので、マウスと比べる時はYの符号を反転します(`SliderComponent` は中でこれをやっています)。
- UIの四角(`DrawRect`、ゲージ、スライダー)は、`Text::Draw` と同じ Direct2D で描きます。だから**カメラの影響を受けず(スクロールしない)、画像や3Dの描画が全部終わった後に重なります**。`Text::Draw` とは**呼んだ順**に重なります(ゲージを描いてから数字を描けば、ゲージの上に数字が出ます)。
- 画面エフェクト(§15)とシーン切り替え(§16)は、UIよりさらに上に出ます。

### Anchor(画面のどこを基準にするか)

```cpp
#include "../../../Engine/UI/Anchor.h"

XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, { 40.f, -40.f });   // 左上の隅から、右に40・下に40
Text::Draw(L"Score", pos.x, pos.y, style);
```

`Anchor` は9か所です: `TopLeft` `Top` `TopRight` / `Left` `Center` `Right` / `BottomLeft` `Bottom` `BottomRight`。

| 関数 | 内容 |
|---|---|
| `GetAnchoredPosition(anchor, offset)` | 基準の位置(画面の隅・辺の真ん中・中央)に `offset`(ずらす量。画面座標なので、下へはマイナス)を足した画面座標を返す。画面の大きさは `Graphics::width` / `height` から毎回計算するので、画面サイズが変わってもずれない |
| `GetAnchoredCenter(anchor, offset, size)` | 大きさ `size` の四角を、四角の `anchor` 側(左上なら左上の角、下なら下の辺の真ん中)を `GetAnchoredPosition` に合わせて置いた時の、四角の**中心**の画面座標。ゲージやスライダーを「左上から右へ40・下へ40」に置いても、画面からはみ出さない |

### Image::DrawRect(画面に四角を塗る)

```cpp
Image::DrawRect({ 0.f, 0.f }, { 200.f, 20.f }, { 1.f, 0.f, 0.f, 1.f });   // 画面の中央に、幅200・高さ20の赤い帯
```

- 引数は **中心の画面座標**・**大きさ(幅, 高さ)**・**色**(`w` は濃さ。0なら描かない)です。
- Component の `Draw()` の中で呼びます(`Text::Draw` と同じ)。
- ブラシは使い回していて、`GameEngine::Uninit` が解放します(ゲーム側で解放する必要はありません)。

### GaugeRenderer(HPバーなどの帯のゲージ)

**値の持ち主にはならず、読んだ値を表示するだけ**のComponentです(§22の「値の持ち主は1か所」と同じ。HPは `PlayerController` が持ち、ゲージは読むだけ)。

```cpp
#include "../../../Engine/UI/GaugeRenderer.h"

// 画面の左上に固定して表示(HUD)
auto* gauge = hud->AddComponent<GaugeRenderer>();
gauge->SetAnchor(Anchor::TopLeft, { 80.f, -135.f });     // 左上の角が、画面の左上から右に80・下に135の所に来る
gauge->SetSize(240.f, 16.f);                             // 幅と高さ
gauge->SetDelayedBar(true, { 1.f, 0.9f, 0.4f, 1.f });    // 減った分を少し遅れて縮める(ダメージ量が分かる)
gauge->SetValueSource(                                   // 毎フレーム、持ち主の値を読んで表示する
    [player] { return (float)player->GetHp(); },
    [player] { return (float)player->GetMaxHp(); });

// オブジェクトの頭上に表示(Playerの上70の位置)
auto* bar = player->AddComponent<GaugeRenderer>();
bar->SetFollowTarget(player, { 0.f, 70.f });
bar->SetSize(80.f, 10.f);
// 3Dのオブジェクトの時は、3D用カメラで画面座標に直す
// bar->SetUse3DCamera(true);
```

| 関数 | 内容 |
|---|---|
| `SetValue(current, max)` | 今の値と最大値(中身の割合 = current / max。0〜1に収める。`max` が0以下なら空) |
| `SetValueSource(今の値を返す関数, 最大値を返す関数)` | 毎フレーム自動で読む関数を決める(持ち主のゲッターを渡す)。決めたら `SetValue` は呼ばなくてよい |
| `SetColors(中身の色, 背景の色)` | 色(`w` は濃さ) |
| `SetDelayedBar(有効か, 色)` | 減った分を遅れて縮める帯。減ってから0.4秒は止まっていて、その後、全体の半分を1秒の速さで縮む(`GaugeRenderer.h` の `delayedHoldTime` / `delayedShrinkSpeed`)。増えた時はすぐ追いつく |
| `SetSize(幅, 高さ)` | ゲージの大きさ(ピクセル) |
| `SetAnchor(anchor, offset)` | 画面に固定して表示。ゲージの `anchor` 側の角(辺)が `GetAnchoredPosition(anchor, offset)` に合う |
| `SetFollowTarget(オブジェクト, offset)` | オブジェクトの上に表示。ゲージの**中心**が、オブジェクトの位置+`offset`(ワールドでずらす量)の画面座標に来る |
| `SetUse3DCamera(true)` | `SetFollowTarget` で3Dのオブジェクトを追う時に使う。`Image::WorldToScreen3D`(§14)で画面座標に直し、カメラの後ろにある時は描かない。2Dは `Image::WorldToScreen2D` で、2Dカメラと一緒にスクロールする |
| `GetRatio()` / `GetDelayedRatio()` | 中身・遅れて縮む帯の割合(0〜1) |

- 時間は `Update` の `dt` で進むので、ポーズ中は帯も止まります(§10)。
- 追いかける相手が非アクティブ(`SetActive(false)`)の間や、`Destroy()` された後は描きません。
- ★ **追いかける相手(`target`)は、ゲージより長く生きるオブジェクトにしてください。** ゲージを付けたオブジェクト自身(上の `Player` の例)なら安全です。別のオブジェクトを追いかけて、そのオブジェクトが消える場合は、先に `SetFollowTarget(nullptr, ...)` で外すか、ゲージも一緒に `Destroy` してください(ポインタだけでは、相手が消えたかどうか調べられないためです)。`nullptr` の間は描きません。

### SliderComponent(ドラッグで値を変えるスライダー)

```cpp
#include "../../../Engine/UI/SliderComponent.h"

auto* slider = obj->AddComponent<SliderComponent>();
slider->SetRange(0.f, 1.f);                              // 値の範囲
slider->SetValue(0.8f);                                  // 最初の値
slider->SetAnchor(Anchor::Bottom, { 0.f, 90.f });        // 画面の下の真ん中から、上へ90(下の辺の真ん中がそこに来る)
slider->SetSize(300.f, 12.f);                            // 溝の幅と高さ
slider->SetLabel(L"Volume", labelStyle);                 // スライダーの上に出す文字(TextStyle)
slider->SetOnChanged([](float v) { /* 値が変わった時 */ });
slider->SetOnDragEnd([](float v) { /* つまみを離した時 */ });
```

| 関数 | 内容 |
|---|---|
| `SetRange(最小, 最大)` / `SetValue(値)` / `GetValue()` | 範囲・値。値は範囲に収まる。コードから変えた時は `SetOnChanged` は呼ばれない(最初の値を決める時に、保存などが動かないように) |
| `SetStep(刻み幅)` | 最小値から刻み幅ごとの値だけになる(0=刻まない) |
| `SetOnChanged(関数)` | つまみを動かして値が**実際に変わった時だけ**呼ばれる(同じ値のままなら呼ばれない) |
| `SetOnDragEnd(関数)` | つまみを離した時に1回呼ばれる(値が変わっていなくても)。保存などに使う |
| `SetAnchor(anchor, offset)` / `SetSize(幅, 高さ)` | 画面のどこに置くか(溝の `anchor` 側の角が合う)・溝の大きさ |
| `SetColors(溝, 左側, つまみ)` / `SetLabel(文字, style)` | 色・スライダーの上に出す文字 |
| `IsDragging()` / `SliderComponent::IsAnyDragging()` | つまみをつかんでいるか(自分 / どれか) |

- 溝のどこか(つまみのまわりも含む)を押すと、そこへつまみが動いて、押している間はマウスに付いてきます。**マウスが溝や画面の外に出ても、離すまで付いてきます**(値は範囲に収まる)。離したら終わりです。
- **ポーズ中・シーン切り替え中(`Time::IsPaused()`)は、スライダーは操作できません**(つかんでいたら離したことになります。ボタンは押せます)。
- **ウィンドウの外でマウスボタンを離した時も、離したことになります。** `Input` はウィンドウの外で離したことに気付けない場合があるので、スライダーは物理的なボタンの状態(`GetAsyncKeyState`)も見ています。他のアプリをクリックした時にドラッグが終わるのは仕様です。
- つかんでいる間に、スライダーのComponentだけを `SetEnabled(false)` しないでください(`Update` が呼ばれなくなり、「つかんでいる」状態が残ってボタンが反応しなくなります)。止めたい時はオブジェクトごと `SetActive(false)` にします(離したことになります)。
- つかんでいる間は、ボタン(`ButtonComponent`)は反応しません(マウスが乗っても、離しても押されない)。

### ButtonComponent(クリックできるボタン)

見た目の画像は、同じオブジェクトの `SpriteRenderer` を使います(先に付けておく)。以前は Sample2D・Sample3D に同じ内容の `ButtonController` がコピーされていましたが、Engineの `ButtonComponent` に移しました。

```cpp
#include "../../../Engine/UI/ButtonComponent.h"

GameObject* button = scene.CreateObject("Button");
button->transform.position = { 0.f, -150.f, 0.f };
button->transform.scale = { 320.f, 96.f, 1.f };                  // ボタンの大きさ(クリック判定もこの大きさ)
button->AddComponent<SpriteRenderer>(L"Assets/button.png");
button->AddComponent<ButtonComponent>(L"START", 48.f, [] { /* 押された時の処理 */ });   // 文字・文字の大きさ・押された時の処理

// あとから変える時
auto* component = button->GetComponent<ButtonComponent>();
component->SetOnClick([] { /* ... */ });
component->SetInteractable(false);    // 押せなくして、灰色にする
```

- 押す→離すで `onClick` が呼ばれます。**ボタンの上で押して、ボタンの上で離した時だけ**です(途中で外に出たらキャンセル)。
- 見た目は、押している間は暗く、マウスが乗っている間は明るく、それ以外は少し暗めです。`SetInteractable(false)` の間は、クリックを受け付けず、画像も文字も灰色になります。
- クリック判定は**ワールド座標**で行います(`Input::GetMouseWorldPosition` と `transform`)。文字は画面座標に直して重ねます(§14)。
- 文字の大きさは、作った時(`AddComponent` した時)のボタンの高さに対する、今の高さの割合で変わります。`TweenComponent`(§12)でボタンが大きくなる途中も、文字が画像と一緒に大きくなります。だから、**ボタンの大きさ(`transform.scale`)は `AddComponent` の前に決めてください**。

### 実例

- **HUD(両サンプル)**: `HUDController` の文字の位置は `GetAnchoredPosition` で決めています(Score / High Score / HP は `Anchor::TopLeft`、GAME OVER などは `Anchor::Center`)。HPの文字の下のHPゲージは、`Objects/HUD.cpp` で `GaugeRenderer` を付けて、`SetValueSource` で `PlayerController::GetHp()` / `GetMaxHp()` を読んで描いています。
- **頭上のHPバー**: Sample2Dの `Objects/Player.cpp` で `SetFollowTarget(player, { 0, 70 })`。Sample3Dは `SetUse3DCamera(true)` も付けています(3Dカメラで画面座標に直す)。一人称の間はカメラがPlayerの位置にあって意味が無いので、`PlayerController` がPlayerの見た目と一緒に隠しています。
- **タイトルの音量スライダー(両サンプル)**: `Objects/VolumeSlider.cpp` の `CreateVolumeSlider(scene, titleController)`。範囲は0〜1で、最初の値は `TitleController::GetVolume()`(= `SaveData` の `"MusicVolume"`、無ければ1)。動かすと `TitleController::SetVolume()` がBGMグループの音量(`Audio::SetGroupVolume(AudioGroup::BGM, 音量)`。§24)を変えて `SaveData::SetFloat` で書き、つまみを離した時(`SetOnDragEnd`)に `SaveData::Save()` でファイルに保存します(§18)。次に起動した時も、その音量でBGMが鳴ります。音量の値の持ち主は `TitleController` で、スライダーはそれを変えてもらうだけです。キーはSample2DとSample3Dで共通です。
- **ボタン(両サンプル)**: `Objects/Button.cpp` の `CreateButton()` が `ButtonComponent` を付けています。

### 注意

- UIの四角と文字は、画像や3Dの後ろには隠れません(Direct2Dで、描画が全部終わった後に描かれるため)。
- ゲージやスライダーは、`DrawRect` の位置が整数のピクセルなら輪郭がくっきり出ます。割合で伸びる帯の端は、ピクセルの途中で止まることがあります(少しぼやけて見えます)。

---

## 21. 開発時の注意点

### 文字コード(重要)

日本語コメントを含む`.h`/`.cpp`ファイルは、**UTF-8 BOM付き**で保存してください。BOM無しだと、日本語ロケールの環境でコンパイラが文字化けを起こし、意味不明な構文エラーが大量に出ます。Visual Studioで保存する分には自動的にBOM付きになりますが、他のエディタを使う場合は明示的に選んでください。

### コメントの書き方

Sample2Dの`PlayerController.cpp`に合わせて、短く書きます。

- `.cpp`: 処理のまとまりごとに、何をしているかを1行で(`// 無敵時間`、`// 移動処理: ...`)。理由が分かりにくい所だけ一言足す(`// マウスがほぼ真上にある時は向きが定まらないので変えない`)
- `.h`: クラスの役割を1行、メンバ変数には行末コメント(`float speed = 200.0f;  // 移動速度`)
- 工場関数(`Game/Objects/*.h`): 何を作るかを1行

### 名前の付け方・チームのルール(CLAUDE.md)

クラス・関数・変数の名前の付け方(PascalCase/camelCase、`Get○○`/`Is○○`、Componentの語尾`Controller`/`Manager`/`Renderer`/`Component`など)と、設計・Engine変更のルールは、リポジトリ直下の`CLAUDE.md`にまとめてあります。新しいコードを書く前に一度読んでください。

**Engine(`Engine/`・`Shader.hlsl`・`Shader3D.hlsl`・`PhysX.props`)を変える時は、必ず先にチームに相談してください。** 両方のサンプルに影響するためです。

ただし今は、`docs/EngineRoadmap.md` の機能を順番に作っている間の**例外**として、ロードマップの機能を作るためのEngine変更は、了承を待たずに進めてよいことにしています(着手する時に「何を作るか・どのEngineファイルを変えるか・両サンプルへの影響」を説明し、終わったら変更したファイルを報告して、レビューは必ず受けます)。ロードマップに無いEngineの変更(バグ修正など)は、今まで通り先に相談してください。ロードマップがすべて終わったら、この例外は元に戻します(詳しくは`CLAUDE.md`の「docs/EngineRoadmap.md の機能を作る間の例外」)。

Claude Code で作業する場合は、`.claude/`の設定で、メインの会話が相談役(navigator)になり、実装は作業役(implementer)、確認はレビュー役(reviewer)が分担します。オブジェクトの追加(`add-object`)・ビルド確認(`build`)・コミット文の作成(`commit-message`)は手順書(スキル)になっています。`CLAUDE.md`・`.claude/`を変更しようとすると、毎回確認のダイアログが出ます(普段はEngineの変更でも出ますが、ロードマップの間は外しています)。詳しくは`CLAUDE.md`の「役割分担」「スキル」を参照してください。

### 新しいファイルを追加したら

追加したファイルが属するプロジェクト(Engineに追加するなら`Engine.vcxproj`、Sample2D用のGameコードなら`Sample2D.vcxproj`、Sample3D用なら`Sample3D.vcxproj`)の、`.vcxproj`と`.vcxproj.filters`の両方に新規ファイルを追加してください(Visual Studioでソリューションエクスプローラーから該当プロジェクトを右クリック→「追加」すれば自動でやってくれます)。

### Update中にオブジェクトを生成してよい

`Scene::CreateObject()`はUpdate処理中に呼んでも安全です(内部で一時リストに貯めてから、ループの外でまとめて追加しています)。

### 音が鳴らない時は(WAVファイルの中身を疑う)

`Audio::Load()` / `Audio::PlayOneShot()` / `AudioSourceComponent`(`Engine/Audio/Audio.cpp`)は`.wav`ファイルの`fmt `/`data`チャンクを名前で探して読みます。以前は「`fmt `の直後は必ず`data`」という決め打ちの44バイト固定ヘッダーで読んでいたため、ffmpeg/Lavfなど一部のツールで書き出したWAV(`fmt `と`data`の間に`LIST`メタデータチャンクが挟まる)を読み込むと、メタデータの文字列を音声データと誤認識し、音が鳴らない(鳴ってもごく短いノイズだけ)という不具合がありました。現在は汎用的なチャンク読み取りに直してあるので、`LIST`/`fact`など余計なチャンクが挟まっていても正しく読めます。**それでも音が鳴らない場合は、まず`.wav`ファイル自体が壊れていないか(別プレイヤーで再生できるか)を確認してください**。

---

## 22. サンプル構成(現状)

プロジェクト構成自体は`0. プロジェクト構成`を参照してください。ここでは各サンプルの中身(`Game/`)を説明します。

```
Sample2D/Game/Objects/     Title, Button, StartButton, VolumeSlider, Background, Player, GameSystem, HUD,
                            EnemySpawner, BulletSpawner, ScorePopup, Explosion, Guard, Wall, Navigation
Sample2D/Game/Components/  TitleController, BackgroundController, PlayerController,
                            BulletController, EnemyController, GameManager, HUDController,
                            EnemyManager, BulletManager, GuardController
Sample2D/Game/Scenes/      TitleScene, GameScene

Sample3D/Game/Objects/     Title, Button, StartButton, VolumeSlider, Skybox, Ground, Player, GameSystem, HUD,
                            EnemySpawner, BulletSpawner, Explosion, ModelDisplay, SkinDisplay, Guard, Wall, WallField, GlassPanel, Lantern, Sun, Hills, Water, Navigation, Cube(未使用), Sphere(未使用)
Sample3D/Game/Components/  TitleController, GroundController, PlayerController,
                            CameraController, BulletController, EnemyController, GameManager,
                            HUDController, EnemyManager, BulletManager, SkyboxFollowComponent,
                            ModelDisplayController, HighlightController, GuardController, LanternController, ShadowManager, SpinComponent(未使用)
Sample3D/Game/Scenes/      TitleScene, GameScene

Demo/Game/Scenes|Objects|Components/   デモ(§38)。カテゴリ別のフォルダ(Common / Core / Render / UI / Render3D / Physics / Behavior / Audio / Data)に分かれている
```

- **Sample2D**: トップダウン視点のアリーナシューター。タイトル画面のSTARTボタン(出る時に`TweenComponent`でぴょこっと大きくなる)をクリックすると、画面が暗転してからゲーム開始(ゲーム画面に入ると明転する。`FadeTransition`。§16)(ボタンは`CreateButton(scene, 文字, 位置, クリックされた時の処理)`で作れる汎用部品で、動きはEngineの`ButtonComponent`。他の画面のボタンにも使い回せる。§20)。WASDで移動、Playerは常にマウスの方を向き、左クリックでその方向へ弾を発射。背景はタイル状に敷き詰めた床(`BackgroundController`)で、移動するとスクロールする(カメラはPlayerに付けた`CameraFollowComponent`が追いかける。§14)。敵(`EnemyManager`が一定間隔でPlayerの周囲にランダムスポーンさせる)が壁を避けながらゆっくり追いかけてきて(`PathFollowComponent`。§36)、弾を当てると倒せる(`EventBus`で`"EnemyDefeated"`イベントを発行しスコア加算。倒した位置に「+10」が出て、上に動きながら消える=`CreateScorePopup`。§19。爆発のパーティクルも出る=`CreateExplosion`。§26)。敵に触れるとHPが減り(1秒間の無敵時間つき)、敵から離れる向きに弾かれる(ノックバック。`AddForce`で衝撃を加え、0.2秒間は移動キーが効かない。§23)、画面が少し揺れて(`CameraShake`。§14)、画面の端が赤く点滅する(`ScreenEffect::Vignette`。§15)。HPが0になるとゲームオーバー画面が出てSPACEでタイトルに戻る(黒い丸が閉じて開く`SpriteTransition`。§16)。スコアがハイスコアを超えていたら、ゲームオーバーの時に保存され(`SaveData`。キーは`"Sample2DHighScore"`で、Sample3Dの記録とは別。§18)、画面左上のスコアの下に出る(次に起動しても残る)。タイトル画面の下にはBGMの音量スライダー(`CreateVolumeSlider`。つまみをドラッグして0〜1で変えられ、離すと保存されて次に起動した時も残る。§20)がある。画面左上のHPの文字の下にはHPゲージ(減った分が少し遅れて縮む)、Playerの頭上にも小さなHPバーが出る(`GaugeRenderer`。§20)。当たり判定・物理はPhysXを使い(Player=`CircleRigidbodyComponent`、敵=`SquareRigidbodyComponent`、弾=`CircleRigidbodyComponent`)、Player・敵はDynamic(`SetVelocity()`で速度を渡して動かす。重力なし・回転なし・`SetFreezePositionZ(true)`で奥行き方向に固定)、弾はKinematicのトリガー。3Dカメラは使わない(`CameraController`なし)。画面の左上のHPゲージの下には、画面の外なので描かずに省いた数「Culled」が出る(敵の `SpriteRenderer` は `SetFrustumCulling(true)`。§28)。マップには見張り役の `Guard`(青い `Player` 画像がゆっくり回り、扇形の視野に `Player` が入って壁にさえぎられていなければ赤くなる。視野は線で表示)と、視線をさえぎる壁 `Wall` が2枚置いてある(`VisionComponent`。§28)。敵が壁を避けるための地図として、原点の周りの6000四方の格子 `Navigation`(`CreateNavigation`。`NavGridComponent`。壁は自動で読まれる)が置いてある(§36)。
- **Sample3D**: Sample2Dと同じアリーナシューターを3D空間で遊べるようにしたもの。タイトル画面のボタン(Sample2Dと同じく、出る時に`TweenComponent`でぴょこっと大きくなる)をクリックすると、Sample2Dと同じく暗転してからゲーム開始(Sample3Dの`CreateButton()`は、位置と大きさをまとめて`Transform`で受け取る形になっている: `CreateButton(scene, 文字, transform, クリックされた時の処理)`)。Player(立方体)は三人称カメラ(`CameraController`。マウス移動で視点回転。カーソルは画面中心に固定され、ESCで解除・左クリックで再固定。TABで一人称に切り替え)と同じ向きを向き、WASDもカメラ基準(W=カメラの前、D=カメラの右)で移動する。左クリックでPlayerの向いている方向へ、地面と水平に弾(球)を発射。敵を倒すと、その位置に爆発のパーティクルが出る(`CreateExplosion`。§26)。敵は2Dの画像(Worldモードの`SpriteRenderer`。Y軸だけカメラの方を向く立て看板)で、地面の上をPlayerに向かって歩いてくる(壁と急な坂を避けて追いかける`PathFollowComponent`を`SetPlane(MovePlane::XZ)`で使う。§36)。歩く間は、敵に付けた`SpriteAnimationComponent`がスプライトシートのコマ(4番と5番)を0.3秒ごとに切り替えて、歩きアニメーションにしている(始まるコマはランダムなので、何体いても足踏みがそろわない。§17)。スコア・ハイスコア(保存先のファイルはSample2Dと同じだが、キーは`"Sample3DHighScore"`で、記録はSample2Dとは別。§18)・HP・無敵時間・被弾時に画面の端が赤く点滅すること・ゲームオーバー(SPACEでタイトルへ。Sample3Dは、黒い帯が左から覆って右へ抜けていく`SpriteTransition(...).SetSwitchFrame(7)`。§16)・`EventBus`のイベント名はSample2Dと同じ。タイトル画面の下のBGMの音量スライダーと、画面左上のHPゲージ(Sample2Dと同じ)、三人称の間だけ出るPlayerの頭上のHPバー(`SetUse3DCamera(true)`の`GaugeRenderer`)もある(§20)。Playerは立方体の見た目とDynamicの`BoxRigidbodyComponent`、敵はDynamicの`CapsuleRigidbodyComponent`(半径40・高さ100の立ったカプセル。§29)(どちらも重力あり・回転なし。Y速度は`GetVelocity().y`を維持したまま`SetVelocity()`で動かす。敵は`PathFollowComponent`がこれをしている)、弾はKinematicのトリガー(`SphereRigidbodyComponent`)。地面は、1000四方のタイルを3Dカメラの描画距離(farZ)の範囲だけ敷き詰めて描く`GroundController`(見た目)と、見えない大きなStaticの板(当たり判定)の組み合わせ。遠景はスカイボックス。地面の端(Playerから斜めに離れた所)には、FBXモデルの見本(`CreateModelDisplay`。`Assets/Models/TestAnim.fbx` の立方体2つ=親と子のパーツ。子の「Top」が、「Spin」(回る)と「Bob」(上下)のアニメーションを3秒ごとに切り替えて再生する。§27)が置いてある(FBXが無い時は何も置かない)。そのとなり(位置 `(1000, -50, 800)`)には、骨2本で曲がる細長い箱(`CreateSkinDisplay`。`Assets/Models/TestSkin.fbx` のアニメーション「Bend」を再生。§27)も置いてある。地面のタイルは、カメラに映る物だけ描く(`Visibility::IsSphereInView3D`)。敵の画像は `SetFrustumCulling(true)` で、画面の外にいる間は描かない。画面の左上のHPゲージの下には、描かずに省いた数「Culled」が出る。`Player` の近く(位置 `(-800, 0, 600)`)には見張り役 `Guard`(青い立方体がゆっくり回り、扇形の視野に `Player` が入って壁にさえぎられていなければ赤くなる。視野は線で表示)と、視線をさえぎる壁 `Wall` が2枚置いてある(`VisionComponent`。§28)。そのとなり(位置 `(400, 50, 300)`)には、半透明で光沢のある水色の板 `GlassPanel`(マテリアルの使用例。当たり判定は無いので通り抜けられる。§30)も置いてある。原点の周りでは、オレンジ色の光(ポイントライト)を持った小さな光る球 `Lantern` が、ふわふわ上下しながらゆっくり回っていて、近くの地面や壁を照らす。`Guard` は、視線の向きに懐中電灯(スポットライト)を持っていて、回ると地面の光の輪が一緒に動く(`LightComponent`。§31)。さらに、太陽の影が有効で(`CreateSun`。`ShadowManager`)、地面に `Player`・壁・FBXモデルの影が落ち、`Guard` の懐中電灯の光は壁や `Player` でさえぎられる(§32)。Playerの東の遠く(位置 `(1500, -50, -2000)` から4000四方)には、高さの画像(`Assets/Terrain/heightmap.png`)から作った、なだらかな丘と谷の地形 `Hills`(`CreateHills`)が置いてある(`TerrainRenderer`。影を受ける。当たり判定の`TerrainRigidbodyComponent`も付いていて、丘の起伏の上を歩ける。Playerの歩く速さ(200)で、丘の最大の傾き(約37度)も越えられる。丘の端はなだらかなスロープなので、地面から歩いて丘に登れる。§34)。その丘の北西のくぼみ(位置 `(2075, -16, 1350)`、650 x 700)には、波打つ半透明の池 `Water`(`CreateWater`)が張ってあります(`WaterRenderer`。青緑の水が波でうねり、日が当たる所がきらっと光り、水の下の地面が透けて見えます。当たり判定は無いので通り抜けられます。§35)。敵が壁と急な坂を避けるための地図として、9000四方の格子 `Navigation`(`CreateNavigation`。丘の高さと傾きも取り込む)が置いてある(池は通れるまま。§36)。Playerの南の遠く(位置 `(0, 0, -6000)` の周り)には、細い柱を15 x 15本並べた `WallField`(`CreateWallField`)が置いてあり、遠い柱を描かない・影を落とさない・枠線を描かない設定(LOD)の見本になっている。画面の左上には、描画命令の数と三角形の数(`Draw` / `Tri`)も出る(§37)。

**デモ**: 機能ごとのデモは、サンプルの中ではなく、別のプロジェクト `Demo`(`Demo.exe`)に集めてある。ゲーム本体(START → `GameScene`)には影響しない。画面の流れ・各デモの内容・新しいデモの足し方は §38。

**Sample3Dの実装メモ**: 向きはすべて`transform.rotate.y`(度)で表し、`rotate.y`の時の正面(ローカルの+Z)は`(sin, 0, cos)`の方向になる(`CameraController::GetYaw()`の式と同じ。`Math::Forward3D()`で求められる)。Worldモードの`SpriteRenderer`は`rotate.y`を使わない(ビルボードの向きとZ軸回転だけを使う)ので、敵の画像の向きは`SetBillboardLock(true, false, true)`でカメラの方へ向けている。Playerは`SetFreezeRotation(true)`で、`RigidbodyComponent`は位置しか同期しないため、`PlayerController`が`rotate.y`が変わった時に`GetActor()`の`setGlobalPose`で当たり判定の箱の回転も合わせている(デバッグ表示の枠と実際の判定が同じ向きになる)。一人称の間は、カメラが立方体の内側に入るのでPlayerの`MeshRenderer`と頭上のHPバー(`GaugeRenderer`)を非表示にしている。

**Sample2Dの実装メモ**: `EnemyManager`/`BulletManager`はどちらも`ObjectPool`を使った使い回し方式(実例は`Engine/Core/ObjectPool.h`)。プールで使い回すオブジェクトは`Scene::CreateObject("Pooled")`で名前だけ付けて作られるため、`GameObject::SetTag()`で組み立て時にタグ("Enemy"/"Bullet")を後付けしている。Sample2Dのスプライトは、背景・壁・見張り役・敵・Player・弾が**2Dモード**(`SpriteMode::World2D`)で、**前後はz座標で決まります**(zが小さい=手前)。割り当ては 背景+100 → 壁+20 → 見張り役+10 → 敵+5 → Player 0 → 弾-5 です(背景のタイルは`BackgroundController`が`Image::Draw`の2Dモードで描く)。爆発(2Dの粒)・ボタン(`SpriteRenderer`のUI)は**UI**で、必ず最前面です(HUDの文字・ゲージは`Direct2D`でさらに手前)。zの差は小さい(最大100)ので、物理(Zを固定した`RigidbodyComponent`。厚み100の壁・敵と重なる)・経路探索(`NavGrid`はXY。壁を探す箱はz=0の厚み10で、壁の厚み100に届く)・視野(`VisionComponent`はXY)には影響しません。Sample2Dのコライダーの枠線・見張り役の視野の線(`Mesh`)は、2Dモードのスプライトと同じ2Dの正射影(`Mesh::DrawLines2D`)・深度テストなしで描かれるので、zが違う物でもスプライトの輪郭に重なり、描いた物の上に見えます(UIより手前には出ません)。敵を倒す/Playerがダメージを受ける判定は、それぞれ`EnemyController::OnTriggerEnter`(相手のタグが"Bullet")と`PlayerController::OnCollisionStay`(相手のタグが"Enemy")が担当する。

役割分担は「値の持ち主は1か所、他は読むだけ」にしている:

| Component | 持っている値・担当 |
|---|---|
| `PlayerController` | HP・無敵時間・ノックバック(HPが0になったら`EventBus`で`"PlayerDied"`を発行し、以後は移動・向き・発射を受け付けない。ポーズ中・シーン切り替え中も`Time::IsPaused()`で受け付けない) |
| `GameManager` | スコア(`"EnemyDefeated"`で加算)・ゲームオーバー判定(`"PlayerDied"`を受けて)・ハイスコアの更新と保存(ゲームオーバーの時。ハイスコアの値そのものは`SaveData`が持ち、`GetHighScore()`はそれを読むだけ。§18)・タイトルへの遷移。表示は一切しない |
| `EnemyManager` | 敵のプールと出現の予定。`Init()`で`Assets/Data/Enemies.csv`(敵の速さ)と`Assets/Data/Waves.csv`(出現ウェーブ)を読み(§25)、表の時間にPlayerの周囲に敵を出し(追いかける動きは敵に付けた`PathFollowComponent`に、相手・速さ・格子を渡す。格子は`CreateEnemySpawner`の引数で受け取る)、`"PlayerDied"`を受けたら出現を止める |
| `BulletManager` | 弾のプール。`"FireBullet"`を受けて、撃った本人の向きへ弾を出す(出す時に発射音`shot.wav`を`Audio::PlayOneShotAt`で鳴らす。§24) |
| `HUDController` | 画面表示(Score/High Score/HP/GAME OVER)だけ。値は自分で持たず、`GameManager::GetScore()`/`GameManager::GetHighScore()`/`PlayerController::GetHp()`などで毎フレーム読む。文字は`Init()`で`Text::LoadFont`したNoto Sans JPで、左上の文字は左寄せ+黒い縁取り(`TextStyle`。§19)。位置は`GetAnchoredPosition`で画面の隅・中央から決める(§20)。HUDオブジェクトに付けた`GaugeRenderer`のHPゲージも、`SetValueSource`で`PlayerController`のHPを読んで描くだけ(§20) |
| `TitleController` | タイトルの表示とBGMの再生。起動時に、BGMグループの音量(`"MusicVolume"`)と効果音(SE)グループの音量(`"SoundEffectVolume"`。変えるUIはまだ無い)を`SaveData`から読んで適用する(§24)。BGMの音量の持ち主(`GetVolume()`/`SetVolume()`。`SaveData`の`"MusicVolume"`に書く)。音量スライダー(`SliderComponent`)は、`CreateVolumeSlider`でここへつないで、動かしてもらうだけ(§20) |

表示用に値のコピーを別に持つと、本物の値とズレる原因になるため、HUDは読むだけにしている(HPゲージも`SetValueSource`で読むだけ)。新しいUI要素を足す時は`HUDController::Draw()`(ゲージなら`GaugeRenderer`。§20)に追加し、表示したい値は持ち主側にゲッターを生やす形にすること。

GameManager/HUD/EnemyManager/BulletManagerのような「見た目の無い管理役」も、Player/Enemyと同じく`Game/Objects/`の工場関数(`CreateGameSystem()`/`CreateHUD()`/`CreateEnemySpawner()`/`CreateBulletSpawner()`)で、それぞれ専用のGameObjectにアタッチして組み立てている(Unityで空のGameObjectにスクリプトを付けるのと同じ)。依存する相手(GameManagerやPlayerControllerのポインタ)は工場関数の引数で受け取るので、`GameScene::Init()`を見れば「誰が誰に依存しているか」が一目で分かる。なお、Component名とファイル名がぶつからないよう、工場関数側は別名にしてある(`GameManager` ↔ `Objects/GameSystem`、`EnemyManager` ↔ `Objects/EnemySpawner`、`BulletManager` ↔ `Objects/BulletSpawner`。TitleController ↔ `Objects/Title.cpp`と同じ関係)。

管理役同士はお互いのポインタを持たず、`EventBus`だけでつながっている。たとえば「Playerが倒れた」は`PlayerController`が`"PlayerDied"`を1回発行するだけで、`GameManager`(ゲームオーバーにする)と`EnemyManager`(出現を止める)がそれぞれ勝手に反応する。新しい管理役を足す時も、既存の管理役を書き換えずに済む。

**Sceneには「何を置くか」だけを書く**(Unityのシーンと同じ考え方)。`TitleScene::Init()`/`GameScene::Init()`は`CreateTitle()`/`CreateStartButton()`/`CreatePlayer()`のような工場関数を並べるだけにして、動き・ルールは各オブジェクトのComponentに書く:

| 処理 | 置き場所 |
|---|---|
| タイトルに戻った時にカメラを原点へ戻す | `TitleController::Init()` |
| STARTを押したら暗転してGameSceneへ(入ったら明転) | `Objects/StartButton.cpp`(汎用の`CreateButton()`に「押された時の処理」を渡す。`ChangeScene<GameScene>(FadeTransition())`。§16) |
| ゲームオーバー中にSPACEでタイトルへ(Sample2Dは丸く閉じて開く、Sample3Dは左から覆って右へ抜ける) | `GameManager::Update()`(`ChangeScene<TitleScene>(SpriteTransition(...))`。Sample3Dは`.SetSwitchFrame(7)`付き。§16) |
| STARTボタンをぴょこっと出す | `Objects/StartButton.cpp`(`TweenComponent`を付けて`ScaleTo`。§12) |
| `"FireBullet"`を受けて弾を出す | `BulletManager::Init()` |
| 表(`Waves.csv`)の時間に敵を出す | `EnemyManager::Update()`(表は`Init()`で読む。§25) |
| 敵を倒した位置に「+10」を出す(Sample2D) | `EnemyController::OnTriggerEnter()`から`CreateScorePopup()`を呼ぶ(動き・消え方は`Objects/ScorePopup.cpp`で付けた`TweenComponent`。§19) |
| 敵を倒した位置に爆発を出す | `EnemyController::OnTriggerEnter()`から`CreateExplosion()`を呼ぶ(両サンプル。粒の設定は各サンプルの`Objects/Explosion.cpp`、粒の動き・描画はEngineの`ParticleRenderer`。§26) |
| FBXモデルの見本を置く、Topパーツのアニメーションを切り替える(Sample3D) | `Objects/ModelDisplay.cpp`(`ModelRenderer`と`ModelAnimationComponent`を付ける)と `ModelDisplayController::Update()`(3秒ごとに`CrossFade`。§27) |
| 骨で曲げるFBXモデルの見本を置いて "Bend" を再生する(Sample3D) | `Objects/SkinDisplay.cpp`(`ModelRenderer`と`ModelAnimationComponent`を付け、`Play("Bend")`。§27) |
| 敵がPlayerを追いかける(壁を避ける) | 敵に付けた`PathFollowComponent`(Engine。§36)。相手・速さ・格子は`EnemyManager::Spawn()`が渡す |
| 敵が壁を避けるための地図(格子)を置く | `Objects/Navigation.cpp`(`NavGridComponent`を付ける。壁は最初のフレームの後に自動で読まれる。Sample3Dは丘の`TerrainRenderer`も渡す。§36)。`GameScene`で壁・丘の後に置く |
| 敵の歩きアニメーション(Sample3D) | 敵に付けた`SpriteAnimationComponent`(Engine。§17)。クリップの登録と再生は`EnemyManager`の`SetupEnemy` |
| 敵に当たったら弾かれる(Sample2D) | `PlayerController::OnCollisionStay()`(`AddForce`。§23) |
| 敵に当たったら画面が揺れる(Sample2D) | `PlayerController::OnCollisionStay()`(`CameraShake::Shake`。§14) |
| 敵に当たったら画面の端が赤く点滅する | `PlayerController::OnCollisionStay()`(`ScreenEffect::Vignette`。§15) |
| タイトルのBGM音量スライダー | `Objects/VolumeSlider.cpp`(`SliderComponent`を付け、動かしたら`TitleController::SetVolume()`、離したら`SaveData::Save()`。§20) |
| 発射音・敵を倒した音 | `BulletManager::Fire()`(`shot.wav`)・`EnemyController::OnTriggerEnter()`(`hit.wav`)から`Audio::PlayOneShotAt`(その場所で鳴る。連射しても重なる。§24) |
| 音を聞く位置 | `Objects/Player.cpp`で付けた`AudioListenerComponent`(Engine。Sample3Dは`SetUse3DCamera(true)`も。§24) |
| 画面左上のHPゲージ | `Objects/HUD.cpp`(`GaugeRenderer`を付け、`SetValueSource`で`PlayerController`のHPを読む。§20) |
| Playerの頭上のHPバー | `Objects/Player.cpp`(`GaugeRenderer`の`SetFollowTarget`。Sample3Dは`SetUse3DCamera(true)`も。一人称の間は`PlayerController`が隠す。§20) |
| カメラがPlayerを追いかける(Sample2D) | `Objects/Player.cpp`で付けた`CameraFollowComponent`(Engine。§14) |
| コライダー枠・見張り役の視野(扇形)の表示ON | `GameManager::Init()`(`RigidbodyComponent::SetDebugDrawEnabled` と `VisionComponent::SetDebugDrawEnabled`) |
| 見張り役(Guard)を置く、ゆっくり回して視野に入ったら赤くする | `Objects/Guard.cpp`(`VisionComponent` を付ける)と `GuardController::Update()`(`CanSee(player)`。§28) |
| 敵の当たり判定(立ったカプセル。Sample3D) | 敵の `SetupEnemy`(`CapsuleRigidbodyComponent`+`SetFreezeRotation(true)`。§29) |
| 壁(Wall)を置く(視線をさえぎる) | `Objects/Wall.cpp`(タグ `"Wall"` + Staticの `BoxRigidbodyComponent`。§28) |
| 半透明で光沢のある板を置く(Sample3D) | `Objects/GlassPanel.cpp`(`MeshRenderer` の `GetMaterial()` で色のα・光沢・自発光・`depthWrite = false` を設定し、`SetDrawPriority` で後に描く。§30) |
| 周りを照らしながら回るランタンを置く(Sample3D) | `Objects/Lantern.cpp`(光る球の見た目に `LightComponent`(ポイントライト)を付ける)と `LanternController::Update()`(中心の周りを回って上下に揺れる。§31) |
| Guardの懐中電灯(スポットライト)を視線の向きに向ける(Sample3D) | `Objects/Guard.cpp`(`LightComponent`(スポット)を付ける)と `GuardController::Update()`(`SetDirection` に、正面を少し下へ傾けた向きを毎フレーム渡す。§31) |
| 縁が青く光るPlayer(リム発光。Sample3D) | `Objects/Player.cpp`(`MeshRenderer::SetRimLight`。§33) |
| 見張り役(Guard)が相手を見つけたら赤い枠線を光らせる(Sample3D) | `GuardController::Update()`(見えた時は太さ4の赤、見えていない時は太さ2の青の枠線を、壁越し(`SetOutline` の3つ目の引数 `true`)で出す。§33) |
| Playerが近づいたら、モデルの枠線を光らせる(Sample3D) | `Components/HighlightController.cpp`(`Physics::OverlapSphere` で近くの "Player" を調べ、`ModelRenderer::SetOutline` の太さを切り替える)。`Objects/ModelDisplay.cpp`(黄色)と `Objects/SkinDisplay.cpp`(緑。骨で曲げたモデル)が付ける。§33 |
| 影を有効にする(Sample3D) | `Objects/Sun.cpp`(見た目の無い管理役)と `ShadowManager::Init()`(`Shadow::GetSettings().enabled = true`)、`Uninit()`(既定に戻す)。影を落とす・受ける物は、各オブジェクトの `Material` の `castShadow` / `receiveShadow`(Player・Wall・ModelDisplay・SkinDisplayは落とす+受ける、Guardは受けるだけ)と、`GroundController::Draw()`(地面は `receiveShadow = true` で描く)。Guardの懐中電灯は `SetCastShadow(true)`。§32 |
| 地面のタイルのうち、画面に映る物だけ描く(Sample3D) | `GroundController::Draw()`(`Visibility::IsSphereInView3D`。§28) |
| 起伏のある地形を置く(丘と谷。Sample3D) | `Objects/Hills.cpp`(`Terrain::LoadFromImage` で高さの画像から地形のデータを作り、`TerrainRenderer` で描く。影を受ける。同じデータから `TerrainRigidbodyComponent` で歩ける当たり判定も付ける。§34) |
| 丘の起伏の上を歩く・物が乗る(Sample3D) | `Objects/Hills.cpp` で付けた `TerrainRigidbodyComponent`(Engine。PhysXの高さフィールド。Playerは今までどおり `SetVelocity` で動くだけ。§34) |
| 丘のくぼみに、波打つ半透明の池を置く(Sample3D) | `Objects/Water.cpp`(`WaterRenderer` に色・波の大きさ・速さの設定 `WaterSettings` を渡し、`SetDrawPriority(100)` で不透明な物より後に描く。当たり判定は付けない=通り抜けられる。波は頂点シェーダーが時間から計算する。§35) |
| 画面の外の敵を描かない | 敵の `SetupEnemy`(`SpriteRenderer::SetFrustumCulling(true)`。両サンプル。§28) |
| 画面の外で省いた数の表示(Culled) | `HUDController::Draw()`(`Visibility::GetCulledCount()`。§28) |
| 描画命令・三角形・距離で省いた数の表示(Draw / Tri。Sample3D) | `HUDController::Draw()`(`RenderStats::GetDrawCallTotal()` など。§37) |
| 多数の柱を置いて、遠い物を省く設定(LOD)を試す(Sample3D) | `Objects/WallField.cpp`(`CreateWall` で柱を格子状に並べ、`MeshRenderer::SetLod` で `drawDistance` / `shadowDistance` / `outlineDistance` を渡す。§37)。`GameScene::Init()` に1行。LODのON/OFFを比べる時は `GameScene.cpp` の `lodEnabled` を書き換える |
| デモの機能を一覧に並べる、新しいデモを足す(Demo) | `Demo/Game/Scenes/Common/DemoRegistry.cpp` の `GetDemoList()` に1行(カテゴリ・名前・説明・2D版/3D版を開く関数。`DemoCategoryScene` が一覧を読んでボタンを並べる。§38) |
| デモの説明の板・Backボタン・Escで戻る・2D/3Dの切り替え(Tab)・全体の設定を既定に戻す(Demo) | `Demo/Game/Objects/Common/DemoGuide.cpp` と `Demo/Game/Components/Common/DemoGuideController.cpp`(§38) |
| ゲームオーバーの時にハイスコアを保存する | `GameManager::Init()`の`"PlayerDied"`を受けた所(`SaveData::SetInt` + `Save`。§18) |

Sceneに残してよいのは、オブジェクトの配置と、シーン全体の後片付け(`GameScene::Uninit()`の`EventBus::Get().Clear()`)くらい。新しい処理を足したくなったら、まず「どのオブジェクトの仕事か」を考えて、そのComponentに書くこと。

どちらも現時点では「動く土台」で、ゲームとしての作り込み(敵の挙動、演出、レベルデザインなど)はこれから追加していく想定です。新しいオブジェクトを追加する時は、それぞれのサンプルの`Game/Objects/`と`Game/Components/`に、既存のPlayer/Enemyと同じ形式でファイルを足していくのが基本の流れです。

---

## 23. PhysX(3D物理演算)

3D用の物理エンジンとして[NVIDIA PhysX 4.1](https://github.com/NVIDIA-Omniverse/PhysX)を導入しています。ビルド済みのライブラリ一式(ヘッダー・dll・lib)を`ThirdParty/PhysX/`にコミット済みなので、**チームメンバーは自分でビルドし直す必要はありません**(`git clone`してそのままビルドできます)。

### 現状

`Engine/Physics/Physics.h/.cpp`が、PhysXの初期化・毎フレームのシミュレーション更新・終了処理を行う最小限の窓口です(Image/Audio/Textと同じ、名前空間+staticなグローバル状態という流儀)。`GameEngine::Init/Run/Uninit`から自動的に呼ばれるので、現時点では特に何もしなくてもPhysXは起動・更新され続けています。

```cpp
namespace Physics {
    bool Init();      // PxFoundation/PxPhysics/PxSceneを作る(GameEngine::Initから呼ばれる)
    void Update(float dt); // シーンをシミュレーションする(GameEngine::Runから毎フレーム呼ばれる)
    void Uninit();    // 後片付け(GameEngine::Uninitから呼ばれる)
}
```

ゲーム側から使う「問い合わせ」(`Raycast`/`OverlapSphere`/`OverlapBox`)と、剛体に力を加える関数(`AddForce`など)は、下の「物理の問い合わせと力」を参照してください。

### RigidbodyComponent(3Dの物理演算でオブジェクトを動かす)

`Engine/Physics/RigidbodyComponent.h/.cpp`に、`BoxRigidbodyComponent`/`SphereRigidbodyComponent`(3D向け)と`SquareRigidbodyComponent`/`CircleRigidbodyComponent`(2D向け)があります。見た目(`MeshRenderer`/`SpriteRenderer`)とは別に、動き方だけを担当するComponentです。4つとも共通の基底クラス`RigidbodyComponent`を継承しているので、他のComponentから形状を意識せず`GetComponent<RigidbodyComponent>()`で探せます(`PlayerController`が実例。これにより、Playerの形状を変えても`Player.cpp`の1行を変えるだけで済みます)。

- 立った人・敵向けの`CapsuleRigidbodyComponent`(3D)/`Capsule2DRigidbodyComponent`(2D)もあります(§29)
- 地形(§34)の起伏に合わせた、動かない当たり判定の`TerrainRigidbodyComponent`もあります(§34)。`Physics::GetCooking()`(PhysXの「クッキング」。形のデータをPhysXが使える形に変える窓口。Engine内部用)を使います
- `Square`/`CircleRigidbodyComponent`は、中身は`Box`/`SphereRigidbodyComponent`と全く同じ(PhysXに2D専用の形状は無いため、奥行きの薄い箱・普通の球で代用している)。トップダウン2Dゲームで使う時に、`XMFLOAT3`のサイズではなく一辺の長さ/半径を1つ渡すだけで済む、より簡単なAPIになっている
- `SquareRigidbodyComponent`の奥行き(Z)は一辺と同じ長さです(薄すぎるとPhysXですり抜けが起きやすいため)。2Dゲームでは`SetFreezePositionZ(true)`を一緒に呼んで、奥行き方向に動かないようにしてください。厚みを自分で決めたい場合は`BoxRigidbodyComponent`を使ってください

```cpp
enum class BodyType {
    Dynamic,    // 重力や衝突で物理的に動く(PhysXの計算結果がTransformに反映される)
    Kinematic,  // 自分のコード(PlayerControllerなど)がTransformを動かす。
                // 他のDynamicな物体を押せるが、自分は重力や力の影響を受けない
    Static      // 完全に動かない(床・壁など)
};
```

```cpp
// 重力や衝突で物理的に動くSphere
sphere->AddComponent<SphereRigidbodyComponent>(BodyType::Dynamic);

// 動かない床
ground->AddComponent<BoxRigidbodyComponent>(BodyType::Static);

// 自分のコードで速度を渡して動かすキャラクター(下記Player参照)
player->AddComponent<SphereRigidbodyComponent>(BodyType::Dynamic, 50.0f, 1.0f);

// キネマティック(Enemyのように、自分は動かないが他の物体を押せる/押し返せる壁として振る舞う)
enemy->AddComponent<BoxRigidbodyComponent>(BodyType::Kinematic, size);
```

完全なコンストラクタは次の形です(`SphereRigidbodyComponent`も同じ並び):

```cpp
BoxRigidbodyComponent(BodyType bodyType, XMFLOAT3 size, float density,
    bool isTrigger, bool isStaticForPush);
```

- `bodyType`: 上記3種類
- サイズ・半径を省略すると、Ownerの`transform.scale`から自動で決まる
- `isTrigger`: `true`ですり抜ける当たり判定(`OnTriggerEnter`)になる。既定`false`(押し返される、`OnCollisionEnter`)
- `isStaticForPush`: `true`だと、Kinematic同士がぶつかった時に自分は押し戻されない側になる(Enemyのように、自分は動かず相手だけ押し返したい時に使う。`BodyType::Static`とは別の概念で、Kinematic同士の手動押し戻しにだけ関係する)
- 実例: Sample3Dの`Game/Objects/Player.cpp`(Dynamic)・`Game/Objects/Ground.cpp`(Static)、Sample2Dの`Game/Components/BulletManager.cpp`(弾=Kinematic)・`Game/Components/EnemyManager.cpp`(敵=Dynamic)・`Game/Objects/Player.cpp`(Dynamic)

**内部実装メモ**:
- Dynamic: 毎フレーム、PhysXのシミュレーション結果(`PxRigidActor`の姿勢)を`Transform`に書き戻す。ただし`SetFreezeRotation(true)`で回転をロックしている間は、位置だけを反映する(回転は自分のコードに任せる。詳細は次項)
- Kinematic: 毎フレーム、`Transform`の現在値(PlayerControllerなどが書き換えた位置)を`setKinematicTarget()`でPhysXへ渡す。次の`Physics::Update()`でそこまで動き、他のDynamicな物体を正しく押せるようになる
- 位置も回転も、DirectXとPhysXで同じ数値のまま受け渡している(`ToPxQuat`/`SyncTransformFromActor`)。DirectXの四元数とPhysXの`PxQuat`は、同じ(x,y,z,w)なら同じ座標を同じ場所へ回すため(例: Y軸まわりにθ回すと、どちらも+Zが(sinθ, 0, cosθ)へ行く)。以前は「左手系と右手系で向きが逆」として四元数のX/Y成分の符号を反転していたが、位置は反転していなかったので、PhysXの中の形が見た目と逆向きに回っていた(修正済み)

### Player(キャラクター)の移動をPhysXに任せる

最初はPlayerを`Kinematic`(`PlayerController`が直接`Transform.position`を書き換える方式)にしていましたが、その場合は「ぶつかった時にどちらへ・どれだけ押し戻すか」を`Physics.cpp`側で自前計算する必要があり、軸を1つ計算し忘れる(実際にZ軸の押し戻しを書き忘れて、Playerが正面衝突時に止まらないバグが起きました)といったミスが起きやすい、という問題がありました。

そこで、PlayerもPhysXの標準的なキャラクター制御の作法(**`Dynamic`＋速度指定**)に変更しました。PhysXの生のAPI(`PxRigidDynamic`など)は`RigidbodyComponent`の中に隠してあるので、`Game/`側では触らずに済みます:

```cpp
// Player.cpp(生成時に1回だけ)
auto* rigidbody = player->AddComponent<SphereRigidbodyComponent>(BodyType::Dynamic, 50.0f, 1.0f);
rigidbody->SetUseGravity(false);       // 重力を受けない(落下しない)
rigidbody->SetFreezeRotation(true);    // ぶつかってもゴロゴロ回転しない
rigidbody->SetFreezePositionY(true);   // 上下(Y)に押し出されない
```

必要な設定だけ個別に呼べます。例えば「重力は受けるが回転はしたくない」オブジェクトなら`SetFreezeRotation(true)`だけ呼べばOKです。

```cpp
// PlayerController::Update()(毎フレーム)
DirectX::XMFLOAT3 velocity = { 0.f, 0.f, 0.f };
if (Input::GetKeyPress(KEY_W)) velocity.z += speed;
// ...

// 基底クラス(RigidbodyComponent)経由で探すので、Box/Sphereどちらでも動く
RigidbodyComponent* rigidbody = owner->GetComponent<RigidbodyComponent>();
if (rigidbody) rigidbody->SetVelocity(velocity);
```

- `Transform.position`を直接書き換えるのではなく、`SetVelocity()`で「速度」だけを渡す。実際に動く処理・何かにぶつかって止まる処理は、全てPhysX自身の計算に任せる
- `SetUseGravity(false)`/`SetFreezeRotation(true)`/`SetFreezePositionY(true)`はそれぞれ独立していて、必要なものだけ呼べる。Playerは3つ全部、Sphereはどれも呼ばない(重力あり・自由に回転)、といった使い分けができる
- **`SetVelocity()`は毎回3軸全部を上書きする**ので注意してください。`PlayerController`はX/Zだけ操作したいのですが、`{velocity.x, 0, velocity.z}`のように**Y成分に0を渡すと、重力で付いたY速度が毎フレーム消されてしまい、自由落下がほとんど効かなくなります**(`SetFreezePositionY(true)`の時はY速度自体が無意味なので問題になりませんが、重力を使う設定に変えると表面化します)。触らない軸は`GetVelocity()`で今の値を読んでから渡してください
- `SetFreezeRotation(true)`にしてあるので、PhysXの回転はTransform.rotateへ反映されない。そのため見た目の向き(Sample2Dでは`rotate.z`=マウスの方向、Sample3Dでは`rotate.y`=カメラの向き。どちらも弾の発射方向にも使う)は、物理側に上書きされることなく`PlayerController`が直接書き換えたまま維持される(`SetFreezeRotation`は「ぶつかっても回転しない」かどうかと、「物理の回転をTransformへ反映するか」を同時に切り替える。詳しくは`RigidbodyComponent.h`のコメント参照)
- **コンポーネントの追加順に注意**: `SphereRigidbodyComponent`は`PlayerController`より先に`AddComponent`すること。Componentは追加順に`Update()`されるため、逆にすると`PlayerController`(Sample2Dでは`CameraFollowComponent`も。§14)が1フレーム古い位置を読んでしまう
- `SetVelocity()`/`SetUseGravity()`/`SetFreezeRotation()`/`SetFreezePositionY()`は全て`Kinematic`/`Static`では何もしません(`Dynamic`専用)。PhysXの生のAPIをもっと細かく触りたい時だけ`GetActor()`を使ってください
- `ObjectPool`で使い回されるオブジェクト(Bulletなど)は、`SetActive()`が呼ばれると`OnActiveChanged()`経由でPhysXのシーンから着脱される。再度有効化された直後は、前回位置からの掃引(スイープ)で誤ったヒット判定が出ないよう、次の`Update()`で`setKinematicTarget()`ではなく直接`setGlobalPose()`でテレポートするようにしている

**重要な注意(ハマりやすいポイント)**: `OnTriggerEnter`/`OnCollisionEnter`などのコールバックは、PhysXのシミュレーションコールバックの中から呼ばれています。PhysXは「コールバックの中でシーンを書き換えるAPI(`scene->addActor()`/`removeActor()`など)を直接呼んではいけない」という制約を持っているため、これらのコールバック内(や、そこから呼ばれる`ObjectPool::Return()`のような処理)で`SetActive(false)`する場合は要注意です。`RigidbodyComponent`はこれに対応済み(`Physics::QueueSceneChange()`で安全なタイミングまで遅延させている)ですが、**今後もし`OnTriggerEnter`などの中から新しくPhysXのシーンを直接触るコードを書く場合は、同じように`Physics::QueueSceneChange()`を使うか、フラグを立てて次のフレームで処理するようにしてください**。直接呼ぶと`Concurrent API write call ... during a callback function are not permitted`というアサーションでクラッシュします。(`RigidbodyComponent::AddForce()`/`AddTorque()`も対応済みで、コールバックの中で呼ばれた時は、PhysXの計算が終わった直後にまとめて加えます。次の節を参照)

`GameEngine::Run()`では、`Physics::Update(dt)`を`SceneManager::Get().Update(dt)`より**先に**呼ぶようにしています。逆順にすると、RigidbodyComponentが1フレーム古い物理結果をTransformに反映してしまいます。

### 物理の問い合わせと力(Raycast / Overlap / AddForce)

「この線の先に何があるか」「この範囲に何がいるか」を調べる関数(`Engine/Physics/Physics.h`)と、剛体に力を加える関数(`RigidbodyComponent`)です。

```cpp
#include "../../../Engine/Physics/Physics.h"

// 正面へ線を飛ばして、最初に当たった敵を調べる
RaycastHit hit;
if (Physics::Raycast(t.position, Math::Forward3D(t), 1000.f, hit, "Enemy")) {
    // hit.object(当たったGameObject)、hit.point(当たった場所)、hit.normal(当たった面の向き)、hit.distance(距離)
}

// 半径300の範囲にいる敵を全部探す(2Dでは円として使う)
for (GameObject* enemy : Physics::OverlapSphere(t.position, 300.f, "Enemy")) {
    // ...
}

// 半径200の範囲にあるアイテム(トリガー)を探す。トリガーを探す時は QueryTriggers::Include を付ける
for (GameObject* item : Physics::OverlapSphere(t.position, 200.f, "Item", QueryTriggers::Include)) {
    // ...
}
```

| 関数 | 内容 |
|---|---|
| `Physics::Raycast(始点, 向き, 最大距離, hit, タグ, トリガー)` | 始点から向きへ線を飛ばし、最初に当たった物を `hit` に入れる。当たればtrue。向きは長さ1でなくてよい(長さ0ならfalse) |
| `Physics::OverlapSphere(中心, 半径, タグ, トリガー)` | 球(2Dでは円)の中にいる物を全部返す(`std::vector<GameObject*>`) |
| `Physics::OverlapBox(中心, 大きさ, タグ, トリガー)` | 箱の中にいる物を全部返す。大きさは `transform.scale` と同じく辺の長さ。箱は回転しない |

最後の引数「トリガー」は `QueryTriggers` で指定します(省略すると `Ignore`):

| 値 | 意味 |
|---|---|
| `QueryTriggers::Ignore`(既定) | トリガーを無視する。視線の判定(Raycast)が、飛んでいる弾などにさえぎられないようにするため |
| `QueryTriggers::Include` | トリガーも探す。アイテムやゴールのような、トリガーそのものを探す時に使う |

**問い合わせの決まり**:
- タグ(省略すると `""` = 全部)を指定すると、そのタグのオブジェクトだけを探します。`Raycast` は、そのタグの物のうち一番近い物を返します(手前に別のタグの物があっても通り抜ける)。
- トリガー(`isTrigger=true` のすり抜ける当たり判定。弾など)は、`QueryTriggers::Include` を渡した時だけ探します(既定は `QueryTriggers::Ignore`)。**タグを指定しても、トリガーは無視されたまま**です。探したいアイテムがトリガーなら、タグと一緒に `QueryTriggers::Include` も必要です。
- 非アクティブ(`ObjectPool` に戻った)・`Destroy()` 済みのオブジェクトは返しません。
- `Raycast` は、始点が中に入っている物には当たりません(自分の中心から線を飛ばしても、自分には当たらない。Unityと同じ)。
- 位置は、最後にPhysXが計算した時(そのフレームの始め)のものです。そのフレームに `transform` を動かしたKinematicや、出したばかりのオブジェクトは、次のフレームから見つかります。
- 読むだけなので、`OnTriggerEnter` などのコールバックの中から呼んでも大丈夫です。

```cpp
// 敵から離れる向きへ、一瞬で速さ600だけ弾く
rigidbody->AddForce(away * 600.f, ForceMode::VelocityChange);
```

| 関数(`RigidbodyComponent`) | 内容 |
|---|---|
| `AddForce(力, ForceMode)` | 力を加える(下の4種類。省略すると `Force`) |
| `AddTorque(回す力, ForceMode)` | 回す力を加える |
| `SetMass(重さ)` / `GetMass()` | 重さを変える / 今の重さ。作った時は「密度 × 体積」で自動で決まっている |
| `SetDrag(空気抵抗)` | 速度が自然に落ちる(0で落ちない、大きいほどすぐ止まる)。重力で落ちる速さにも効く |

| `ForceMode` | 重さで割るか | 時間 | 使いどころ |
|---|---|---|---|
| `Force` | 割る | かける(押し続ける) | 風・エンジンなど、毎フレーム押す |
| `Impulse` | 割る | 一瞬 | 衝撃(ノックバックなど) |
| `Acceleration` | 割らない | かける | 重さに関係なく同じように加速させる(`AccelerateChaseComponent`) |
| `VelocityChange` | 割らない | 一瞬 | 速度を一気に変える(ジャンプなど) |

**力の決まり**:
- **Dynamicにしか効きません。** Kinematic/Staticでは何もせず、出力ウィンドウに1回だけ「Dynamicではないので効きません」と表示されます。非アクティブ(プールに戻っている)間も何もしません。
- ロックした軸(`SetFreezePositionX/Y/Z`・`SetFreezeRotation`)の向きの力は0になります(力をかけても、その向きには動かない・回らない)。
- `AddForce`/`AddTorque` は、`OnCollisionStay` などのコールバックの中で呼んでも大丈夫です(PhysXの計算が終わった直後にまとめて加わります)。`SetMass`/`SetDrag` はコールバックの中では呼ばず、`Init`/`Update` で呼んでください。
- 毎フレーム `SetVelocity()` で速度を上書きしているオブジェクト(Playerなど)は、上書きした時点で力で付いた勢いが消えます。力で動かしている間は `SetVelocity()` を止めてください(下の実例)。
- `Impulse`/`Force` は重さで割られるので、重いオブジェクトほど動きません。重さに関係なく決まった速さにしたい時は、`VelocityChange`/`Acceleration` を使うか、強さを「速さ × `GetMass()`」にします。

**実例(Sample2Dのノックバック)**: `PlayerController::OnCollisionStay` で、敵に当たってダメージを受けた時に、敵から離れる向き(`Math::DirectionTo`、XY平面)へ `AddForce(..., ForceMode::Impulse)` で弾いています。Playerは重い(密度100)ので、強さは「弾かれる速さ(`knockbackSpeed`)× `GetMass()`」にしています。弾かれている0.2秒間(`knockbackTimer`)は、移動キーでの `SetVelocity()` をしないようにして、弾かれた勢いを残しています。

### ワールド単位について

このエンジンの3Dワールドは、Cube/Sphereの`scale=100`が実寸1mくらいに相当する感覚で作られてきています。PhysXは実寸メートル基準で調整されたデフォルト値(重力・スリープ閾値など)を持っているため、`Physics::Init()`では重力を`(0, -981, 0)`(標準の9.81 m/s²を100倍したもの)に、`PxTolerancesScale.length`も`100`にしてあります(これを合わせないと、シミュレーションの精度・安定性が悪くなります)。今後PxRigidActorのサイズ等を決める時も、この「100単位=1m」のスケール感を意識してください。

### ビルド環境について(記録用)

このマシンではVisual Studio 2022しか入っておらず、PhysX 4.1の公式プリセットはVS2019(`vc16win64`)までしか無かったため、`vc17win64`プリセットを自作して生成しました(`physx/buildtools/presets/public/vc17win64.xml`と、`cmake_generate_projects.py`への`vc17`対応追加)。これらはPhysXのソース側(このリポジトリの外、`ThirdParty/PhysX/`には含まれていないビルド作業用のフォルダ)の変更なので、もし将来PhysXを更新・再ビルドする必要が出てきたら、同じ手順を踏んでください。

- `ThirdParty/PhysX/lib/x64-debug/`, `x64-release/`: Debug/Release、x64向けのビルド済み成果物(`.pdb`/`.map`や、GPU支援用のDLL(`PhysXGpu`/`PhysXDevice`)は容量削減のため含めていません。今回はCPUベースの物理演算のみを使う想定です)
- PhysXのinclude/lib設定は`PhysX.props`(ソリューションフォルダ直下)に1箇所にまとめてあり、`Engine.vcxproj`/`Sample2D.vcxproj`/`Sample3D.vcxproj`の全てがこれをImportしています。PhysXのバージョンを上げる、リンクするlibを増減する時はこのファイルだけ直せば全プロジェクトに反映されます。`RuntimeLibrary`も同じファイルで`MultiThreaded`/`MultiThreadedDebug`(静的CRT)に設定済みです。PhysXのビルドが`NV_USE_STATIC_WINCRT=True`だったため、CRTを合わせないとリンクエラーになります(新しいプロジェクトを追加する時も、必ず同じ`RuntimeLibrary`にしてください)

---

## 24. 音(Audio / AudioSourceComponent / AudioListenerComponent / グループ音量)

音を鳴らす仕組みです(`Engine/Audio/Audio.h`、`Engine/Audio/AudioSourceComponent.h`、`Engine/Audio/AudioListenerComponent.h`)。**音が「どこで鳴っているか」で聞こえ方が変わります**(遠い音は小さく、左で鳴った音は左から聞こえる)。同じ効果音を重ねて鳴らせるので、連射しても音が途切れません。BGMと効果音の音量は別々に変えられます。

### 使い方

```cpp
#include "../../../Engine/Audio/Audio.h"
#include "../../../Engine/Audio/AudioSourceComponent.h"
#include "../../../Engine/Audio/AudioListenerComponent.h"

// 聞く側: Playerに1つ付ける(2D。向きは画面の上が正面で固定)
player->AddComponent<AudioListenerComponent>();
// 3Dなら、向きを3D用カメラに合わせる
player->AddComponent<AudioListenerComponent>()->SetUse3DCamera(true);

// 鳴らしっぱなしの効果音(重ねて鳴らせる。鳴り終われば自動で片付く)
Audio::PlayOneShotAt(L"Assets/Sounds/shot.wav", position);   // その場所で鳴らす(遠いほど小さく、左右の向きも変わる)
Audio::PlayOneShot(L"Assets/Sounds/click.wav", 0.8f);        // 位置に関係なく鳴らす(UIの音など)。第2引数は音量

// 音を出すオブジェクトに付ける(焚き火・機械など。オブジェクトが動けば聞こえ方も変わる)
auto* fire = obj->AddComponent<AudioSourceComponent>(L"Assets/Sounds/fire.wav");
fire->SetLoop(true);
fire->SetDistance(200.f, 1500.f);   // 200までは最大音量、1500で聞こえなくなる(その間は直線的に小さくなる)
fire->Play();

// グループごとの音量(0〜1)
Audio::SetGroupVolume(AudioGroup::BGM, 0.5f);
Audio::SetGroupVolume(AudioGroup::SE, 1.0f);
```

| 関数 | 内容 |
|---|---|
| `Audio::PlayOneShot(path, volume = 1)` | 位置に関係なく鳴らす。重ねて鳴らせる。グループはSE |
| `Audio::PlayOneShotAt(path, position, volume = 1)` | `position`(ワールド座標)で鳴らす。聞く位置からの距離と左右で聞こえ方が変わる |
| `Audio::SetGroupVolume(group, volume)` / `GetGroupVolume(group)` | `AudioGroup::BGM` / `AudioGroup::SE` の音量(0〜1。範囲外は収める) |
| `AudioSourceComponent(path)` | 音を出す側。`Play` / `Stop` / `IsPlaying` / `SetVolume` / `SetLoop` / `SetSpatial(false)`(位置を使わない=BGM・UIの音) / `SetDistance(min, max)` / `SetGroup` |
| `AudioListenerComponent` | 聞く側。付けたオブジェクトのワールド位置で聞く。`SetUse3DCamera(true)` で向きを3D用カメラに合わせる |
| `Audio::Load` / `Play` / `Stop` / `Pause` / `Resume` / `SetVolume` / `SetLoop` / `Release` | 今までどおり。IDで鳴らす方法(長く鳴らすBGM向け。タイトルのBGMはこれ)。`Load` の第3引数で、グループを選べる(既定は**BGM**) |

### 仕組みと決まり

- **音のデータと再生口を分けています。** 音のデータ(WAVの中身)は `Resources::LoadSound` で1回だけ読み、みんなで使い回します(§11と同じ参照カウント。0になってもシーン切り替えまで残る)。実際に音を出す「再生口」(`IXAudio2SourceVoice`)は、鳴らす度に借りて、鳴り終わったら返して使い回します。だから、同じ音を何個でも重ねて鳴らせます。
- **同時に鳴らせるのは32個まで**(`Audio::maxVoices`)。超えた分は鳴らさず、出力ウィンドウに1回だけ知らせます。
- **グループの音量は、音を途中でまとめるボイス(サブミックス)で実現しています。** BGMグループの音量を下げれば、IDで鳴らしたBGMがまとめて小さくなります。`PlayOneShot` / `AudioSourceComponent` は既定でSEグループです。
- **聞こえ方の計算は XAudio2 付属の X3DAudio です。** 聞く位置からの距離で小さくなり(`minDistance` までは最大音量、`maxDistance` で聞こえなくなる。既定は 300 と 2500)、左右の位置で左右の音量が変わります。モノラルでもステレオのWAVでも動きます。出力はいつもステレオ(2チャンネル)です。
- **向きの決め方**: 2Dは「画面の上が正面、右が右」で固定(Playerが向きを変えても、画面の左にある音は左から聞こえる)。3D(`SetUse3DCamera(true)`)は、3D用カメラの向きです。2Dでは上下(Y)は前後の扱いになり、画面の上にある音は真ん中から聞こえます。
- **`AudioListenerComponent` が2つ以上ある時**は、最後に有効になった(`AddComponent` / `SetActive(true)` された)1つだけが使われ、出力ウィンドウで1回知らせます。**1つも無い時**は、2D用カメラ(`Image::GetCamera()`)の x,y が聞く位置になります(2Dゲームは付けなくても動きます)。**3Dゲームは必ず Player などに付けてください**(付けないと、2D用カメラの位置で聞くことになってしまいます)。
- **ポーズ中・シーン切り替え中(`Time::IsPaused()`)も、音は止まらず鳴り続けます。** 切り替え中にBGMや効果音が途切れないほうが自然なためです(止めたい時は `AudioSourceComponent::Stop` や `Audio::Pause` を使う)。
- **`AudioSourceComponent` の再生口は、`Stop`・オブジェクトの破棄(`Destroy`)・シーン切り替え・`SetActive(false)` で返ります。** 鳴りっぱなしの音は残りません。`SetActive(false)` で止まった音は自動では再開しないので、必要なら `Play` し直してください。`SetLoop` / `SetGroup` は次の `Play` から効きます。
- **`PlayOneShot` / `AudioSourceComponent` が失敗した時**(WAVが読めない・音声デバイスが無い)は、何もせず、出力ウィンドウに1回(パスごと)知らせます。
- `Audio::Update` / `LoadWave` / `ReleaseWave` / `SetListener` / `StartVoice` などはEngine内部用です。ゲーム側からは呼びません。

### 保存と音量

- BGMグループの音量は `SaveData` の `"MusicVolume"`、効果音(SE)グループの音量は `"SoundEffectVolume"` に保存する決まりです。両サンプルの `TitleController::Init()` が、起動時に読んで `Audio::SetGroupVolume` で適用します(保存されていなければ最大)。タイトルのスライダー(§20)が変えるのはBGMグループです。効果音の音量を変えるUIは、まだありません(`SoundEffectVolume` を `SaveData` に書けば、次の起動から効きます)。

### 効果音の素材(`Assets/Sounds/`)

発射音 `shot.wav` と、敵を倒した音 `hit.wav` は、PowerShellで作った自作の音です(著作権の問題はありません)。16bit PCM・モノラル・44.1kHz の短いWAVです(モノラルにしておくと、左右の位置で聞こえ方が変わります)。

- `shot.wav`(0.15秒): 1100Hz から 250Hz へ下がる正弦波 + 最初だけ短いノイズ。急に小さくなる(`exp(-22t)`)。
- `hit.wav`(0.25秒): 520Hz → 780Hz の2段の電子音(倍音を少し足す)+ ノイズ。`exp(-12t)` で小さくなる。

自分で作る時は、サンプル値を `sin(2π × 周波数 × 時間)` で並べて、時間とともに音量を下げ(`exp(-速さ × t)`)、16bitの整数にして WAV ヘッダー(44バイト)の後ろに書けば作れます。`Assets/` はビルドの時にフォルダごと(`Sounds/` も)出力先へコピーされます。

### Sampleでの使われ方

- 両サンプルの `BulletManager::Fire()` が、弾を出す時に `Audio::PlayOneShotAt(L"Assets/Sounds/shot.wav", position)`。
- 両サンプルの `EnemyController::OnTriggerEnter()` が、敵を倒した時に `hit.wav` を倒された場所で鳴らす。
- 両サンプルの `Objects/Player.cpp` に `AudioListenerComponent`(Sample3Dは `SetUse3DCamera(true)` 付き)。

### 注意

- 位置の単位はワールド座標です(2Dはピクセル、3Dのサンプルは1000で地面のタイル1枚)。ゲームの広さに合わせて `SetDistance` で調整してください。`PlayOneShotAt` は既定の 300 / 2500 です。
- WAV は 16bit PCM(または8bit)を使ってください。フォーマットが違う音は同じ再生口を使い回せないので、種類が増えると再生口を作り直します(動きはしますが、1つのフォーマットにそろえたほうが軽いです)。

---

## 25. CSV(Csv)

敵の強さや出現の順番などを、コードではなく表(Excel)で調整するための仕組みです。表のファイル(`.csv`)を読んで、**列の名前**で値を取り出します。

```cpp
#include "../../../Engine/Data/Csv.h"

// Assets/Data/Enemies.csv
// Name,Speed
// Slime,60

Csv table = Csv::Load(L"Assets/Data/Enemies.csv");     // ComponentのInitで1回だけ読む
int row = table.FindRow("Name", L"Slime");              // Nameが Slime の行(無ければ-1)
float speed = table.GetFloat(row, "Speed", 60.f);       // 無い・読めなければ 60
```

| 関数 | 内容 |
|---|---|
| `Csv::Load(パス)` | 読み込む。1行目は列の名前として扱う。**呼ぶ度にファイルを読み直す**(キャッシュはしない) |
| `IsValid()` | 読み込めたか(ファイルが無い・16MBより大きい・中身が空だとfalse。falseの表の `Get○○` は、いつも無い時の値を返す) |
| `GetRowCount()` | 行の数(1行目の列の名前は数えない) |
| `GetColumnCount()` / `HasColumn(列)` | 列の数 / その名前の列があるか |
| `GetString(行, 列)` | 文字として読む(無ければ `L""`) |
| `GetInt(行, 列, 無い時の値 = 0)` | 整数として読む(`"1.5"` や `"abc"` は無い時の値。`SaveData` と同じ厳しさ) |
| `GetFloat(行, 列, 無い時の値 = 0)` | 小数として読む(無限大・NaNは読まない。PCの地域設定に関係なく小数点は `.`) |
| `FindRow(列, 値)` | その列の値が同じ最初の行を探す(無ければ-1。前後の空白や大文字小文字が違うと別の値) |
| `IsFileUpdated()` | 読み込んだ後にファイルが変わったか(更新日時か大きさが違えばtrue)。ファイルが今は無い時はfalse。**軽いが、毎フレームは呼ばない**(0.5秒ごとなど) |
| `Reload()` | 同じパスを読み直す。成功したら中身を入れ替えてtrue。**失敗したら今の中身のまま**falseを返す |

**使い方の決まり**:
- **ComponentのInitで読みます**(毎フレーム `Load` しない)。Sampleの `EnemyManager::Init()` のように、`Csv` と読んだ値はComponentのメンバに入れて使います。CSVのパスの文字は、1か所(`EnemyManager.cpp` の先頭)にだけ書きます。実行中の反映は、下の「実行中の反映」を見てください
- 行の番号は0から数えます(表の2行目が `row = 0`)。`FindRow` が返した番号をそのまま `Get○○` に渡せます(-1だった時は無い時の値になり、出力ウィンドウに知らせます)
- **列の名前で取り出す**ので、Excelで列の順番を入れ替えても壊れません。列の名前は半角英数字(`Name`, `Speed`)にします。**大文字小文字は区別します**(`"name"` と `"Name"` は別の列)。列の名前の前後の空白は無視し、同じ名前の列が2つあれば最初のものを使います
- 列が無い・行が範囲外・値が空・数に読めない時は、無い時の値を返して、Visual Studioの出力ウィンドウに `★ Csv: ...` と知らせます(列ごと・ファイルごとに1回だけ)
- `Csv` は値として持てます(コピーしても表の中身は共有するので軽いです)

**ファイルの形式**:
- 1行目が列の名前、2行目からが値で、値はカンマで区切ります
- `"` で囲んだ値の中には、カンマ・改行・`""`(`"` 1文字)も書けます(Excelが書き出す形式です)
- 改行はCRLF・LF・CRのどれでも読めます。最後の行に改行が無くても読めます。全部の値が空の行(空行や `,,`)は飛ばします
- 行ごとに値の数が違っていても読めます(足りない分は空、列の名前より多い分は無視。出力ウィンドウに知らせます)

### Excelで編集する時の注意(文字コード)

日本語版のExcelは、普通に保存すると **Shift-JIS** という文字コードになり、日本語が文字化けします。**「名前を付けて保存」で、ファイルの種類を「CSV UTF-8(コンマ区切り)」にして保存してください**。

- UTF-8(BOMがあってもなくてもよい)を既定で読みます。UTF-8として読めないバイトがあるファイルは、Shift-JISとして読み直して、出力ウィンドウに `★ Csv: ... Shift-JIS のようです。Excel では『CSV UTF-8(コンマ区切り)』で保存してください` と警告します(動きはします)
- 列の名前や数だけで日本語を含まないファイルは、どちらで保存しても同じ内容なので警告は出ません
- UTF-16のファイル(Excelの「Unicodeテキスト」)は読めません
- Excelで開いたままでも読めるようにしてあります。それでも読めない時は(出力ウィンドウに `読み込めませんでした` と出ます)、保存の途中かもしれないので少し待ってください。ずっと読めなければExcelを閉じてください
- 反映のしかたは、次の「実行中の反映」を見てください(ゲームを動かしたまま、保存するだけで反映されます)

### 実行中の反映(ホットリロード)

ゲームを動かしたまま表を保存すると、起動し直さずにゲームに反映されます(Sampleでは `EnemyManager` がやっています)。

```cpp
// メンバ: Csv table;  Timer reloadTimer{ 0.5f, true };

// Init
table = Csv::Load(L"Assets/Data/Enemies.csv");

// Update(0.5秒ごとに確かめる)
if (reloadTimer.Update(dt) && table.IsFileUpdated()) {
    if (table.Reload()) { /* 値を取り出し直す */ }
}
```

- **どのファイルを編集するか**: 実行中は、**出力先**(`x64\Debug\Assets\Data\Waves.csv` など。実行ファイルの隣)を保存してください。ソース側(`3D_FreamWork\Assets\Data`)を編集した時は、ビルド(F5)の時に出力先へコピーされるだけで、実行中には自動でコピーされません
- **確認は0.5秒ごと**: `IsFileUpdated()` はファイルの更新日時と大きさを見るだけなので軽いですが、毎フレーム呼ぶ必要はありません。`Timer` などで0.5秒ごとなどにしてください
- **保存の途中で読めなくても大丈夫**: Excelの保存の途中でファイルが空だったり、ロックされていたりして `Reload()` が失敗しても、今の値のままゲームは動き続けます。読み込み済みの日時は変えないので、次の確認でまた試します(同じ状態の失敗は出力ウィンドウに1回だけ知らせます)。壊れたファイルを保存した時も同じで、直して保存し直せば反映されます
- **ファイルが消えた時**: `IsFileUpdated()` はfalseのままで、今の値で動きます
- **`Reload()` は呼んだ `Csv` の中身だけ入れ替えます**。先にコピーした `Csv` は古い中身のままなので、`Csv` を持つComponentが自分で `Reload` してください
- Excelでは「CSV UTF-8(コンマ区切り)」で保存してください(Shift-JISのままだと、日本語が文字化けします)
- Sampleの `EnemyManager` の場合:
  - `Enemies.csv`: 敵の速さを取り直し、**すでに出ている敵にも新しい速さを渡します**(`PathFollowComponent::SetSpeed`。出ている敵は `ObjectPool::GetObjects()` で調べます)
  - `Waves.csv`: 経過時間はそのままにして、新しい表で「今の経過時間より前に出るはずだった回」を**出した扱い**にし、次に出す時間だけを決め直します。そのため、すでに出した分を二重に出さず、これから出す分も飛ばしません(例: 12.3秒の時に表を変えると、12.3秒より後の回から新しい表になります)。先に出ている敵はそのままです

### 実例: 敵の速さと出現ウェーブ(両サンプルの `EnemyManager`)

`Assets/Data/Enemies.csv`(敵の速さ):
```
Name,Speed
Slime,60
```
`EnemyManager::Init()` が `Name` が `Slime` の行の `Speed` を読み、敵に付けた `PathFollowComponent`(§36)に渡します。

`Assets/Data/Waves.csv`(出現ウェーブ。1行が1ウェーブ):
```
Time,Count,Interval,Repeat
1.0,1,2.5,0
```

| 列 | 意味 |
|---|---|
| `Time` | そのウェーブが始まる時間(秒。シーンが始まってから) |
| `Count` | 1回に出す敵の数 |
| `Interval` | 出す間隔(秒)。0なら1回だけ |
| `Repeat` | 出す回数。**0なら、次のウェーブが始まるまで出し続ける**(最後のウェーブなら、ずっと) |

上の1行は「1秒後から、2.5秒ごとに1体、ずっと」で、今までの動き(最初の1体が1秒後、その後2.5秒ごと)と同じです。行を足すと、後のウェーブが始まった時に前のウェーブは終わります。たとえば、

```
Time,Count,Interval,Repeat
1.0,1,2.5,0
30,2,2.0,0
60,5,5.0,3
```

は「30秒までは2.5秒ごとに1体 → 30秒から2秒ごとに2体ずつ → 60秒から5秒ごとに5体ずつ、3回だけ」になります。行は時間の順でなくても動きます(始まる時間の順に並べ直します)。`Count` が1以上でない行は飛ばします。

読み込めなかった時(ファイルが無い・使える行が無い)は、今までの値(1秒後から2.5秒ごとに1体、速さ60)にして、出力ウィンドウに知らせます。

**自分のゲームに足す時**: 表の持ち主は、それを使うComponent(Sampleでは `EnemyManager`)です。Sceneには書きません。マス目の配置図(ステージ)のように表をオブジェクトに変えたい時は、Sceneではなく工場関数 `CreateStage(scene, L"Assets/Data/Stage1.csv")` のようにして、`Game/Objects/` に置いてください(「Sceneは配置だけ」のルール。§22)。

**注意**: `Assets/` はビルドの時に出力先(`x64\Debug\Assets\` など)へコピーされます。ソースの `Assets/Data/*.csv` を編集してから、実行(F5)かビルドをしてください(Engineのビルドのたびに、新しく保存した方のファイルだけがコピーされます。Assetsだけの編集でも反映されます)。出力先のファイルを直接編集しても動きますが(実行中の反映は出力先を保存します)、ソース側の方が新しいと次のビルドで上書きされるので、気に入った値はソース側にも書いてください。

---

## 26. パーティクル(ParticleRenderer / CreateEffect)

爆発・煙・火花などを、たくさんの小さな粒で出す仕組みです。粒1つ1つはGameObjectではなく、`ParticleRenderer`の中のデータ(位置・速さ・残り時間)で、まとめて描きます(数百個出しても重くなりにくい)。使うヘッダーは `Engine/Render/ParticleRenderer.h`(`ParticleSettings`もここ)と `Engine/Render/Effect.h`(`CreateEffect`)です。

### 使い方1: 使い捨ての爆発(`CreateEffect`)

```cpp
ParticleSettings settings;
settings.spreadAngle = 360.f;     // 全方向に飛び散る
settings.additive = true;         // 加算合成(重なるほど明るい)
GameObject* effect = CreateEffect(scene, settings, position);   // 位置にエフェクトを作る
effect->GetComponent<ParticleRenderer>()->Burst(30);           // 一度に30個出す
```

- `CreateEffect`は、`ParticleRenderer`の付いたオブジェクトを作ります。**作った直後は粒を出していない**ので、続けて`Burst(個数)`を呼びます
- 粒がすべて消えたら(`IsPlaying()`がfalseになったら)、オブジェクトは**自動でDestroy**されます。後片付けは要りません
- `Burst`を呼ばないと、次のフレームに粒が0のまま消えます。作ったらすぐ`Burst`してください
- `Play()`で出し続けた場合は、**`Stop()`して粒がすべて消えるまで残ります**(`Stop()`しないと消えません)。出し続けるエフェクト(炎など)は、自分で`Stop()`するか、オブジェクトを`Destroy()`してください
- 手前に描かれるよう、描画順(`drawPriority`)を大きくしてあります

### 使い方2: オブジェクトに付ける(`ParticleRenderer`)

```cpp
auto* particles = obj->AddComponent<ParticleRenderer>(settings);
particles->Play();      // emitRate個/秒ずつ出し続ける
particles->Stop();      // 新しく出すのをやめる(出ている粒は寿命まで残る)
particles->Burst(10);   // Playしているかに関係なく、一度に10個出す
```

| 関数 | 意味 |
|---|---|
| `Play()` | 出し始める(`emitRate`の数ずつ。`Stop()`するまで) |
| `Stop()` | 新しく出すのをやめる(出ている粒は寿命まで残る) |
| `Burst(count)` | 一度に`count`個出す(爆発) |
| `IsPlaying()` | 出している、または粒が残っているか |
| `GetParticleCount()` | 今出ている粒の数 |
| `SetDestroyWhenFinished(true)` | `IsPlaying()`がfalseになったらオブジェクトをDestroyする(`CreateEffect`がこれを使う) |

作った直後は出していません(`Play()`か`Burst()`で出し始めます)。

### ParticleSettings の項目

| 項目 | 意味 | 既定 |
|---|---|---|
| `texture` | 粒の画像。空なら`Assets/particle.png`(柔らかい白い円) | 空 |
| `emitRate` | 1秒に出す数(`Play`中だけ。`Burst`には関係ない) | 10 |
| `lifetimeMin` / `lifetimeMax` | 粒が消えるまでの時間(秒。この間でランダム) | 0.5 / 1.0 |
| `speedMin` / `speedMax` | 粒の速さ(1秒に動く距離。この間でランダム) | 50 / 100 |
| `direction` | 出る向き | (0,1,0)=上 |
| `spreadAngle` | 向きのばらつき(度。向きを中心に全体でこの範囲。360なら全方向) | 30 |
| `gravity` | 重力(1秒に速度が変わる量。`(0,-300,0)`なら下へ落ちていく) | (0,0,0) |
| `startSize` / `endSize` | 出た時と消える時の大きさ(寿命に沿って変わる。0以下になった粒は描かない) | 16 / 0 |
| `startColor` / `endColor` | 出た時と消える時の色(`w`は濃さ。寿命に沿って変わる) | 白 / 白で透明 |
| `additive` | 加算合成(重なるほど明るくなる。炎・光向け)。falseは、ふわっと重なる合成(煙向け) | false |
| `worldSpace` | true=出た場所に残る(煙)、false=オブジェクトと一緒に動く(オーラ) | true |
| `use3D` | false=2D画面(Sample2D)、true=3D空間(Sample3D) | false |

長さの単位は、オブジェクトの`position`・`scale`と同じです(2Dはピクセル、Sample3Dはワールド単位。Sample3Dの敵の大きさが100なので、粒も大きめにします)。

### 2Dと3D

**同じAPIで書けます。違いは`use3D`だけです。**

| | 2D(`use3D = false`、既定) | 3D(`use3D = true`) |
|---|---|---|
| 描き方 | `SpriteRenderer`のUI(`SpriteMode::UI`)と同じ(2Dカメラでスクロールする。zで大きさは変わらず、必ず最前面。`drawPriority` → 作った順) | `SpriteRenderer`の3D(`SpriteMode::World3D`)と同じ。粒は**常にカメラの方を向く板(ビルボード)**で、手前の3Dオブジェクトに隠れる |
| `spreadAngle` | XY平面で向きを±(角度÷2)回す | 向きを軸にした円錐(360なら全方向の球) |
| 粒の位置 | `x, y`(`z`は使わない) | `x, y, z` |

3Dでの対応範囲: 粒は半透明なので、手前の物には隠れますが、粒どうしの前後は描いた順で決まります(加算合成なら見た目はほとんど変わりません)。回転する粒・影・ライトの影響は対応していません。

### 加算合成とふわっとした合成

- `additive = true`: 粒の色を背景に足します。重なるほど明るくなるので、爆発・炎・火花・光に向いています
- `additive = false`: 粒の濃さに応じて、背景に色を重ねます(ふわっとした煙のような見た目)。重ねても明るくなりません
- どちらも、既存のスプライトの描画(見た目・描画順)には影響しません

**粒の画像の作り方(重要)**: `Shader.hlsl`は「アルファが0.5未満のピクセルは描かない」ので、アルファで柔らかさを表すことができません。そのため粒の画像は、**明るさ(RGB)で形と濃さを表す白黒の画像にします(中心が白、外側が黒、アルファは不透明)**。`Assets/particle.png`(64x64)がその例です。普通のアルファ付きの画像(外側が透明)を指定すると、縁がくっきり切れたり、背景が黒い四角で見えたりします。

### 上限と時間

- 1つの`ParticleRenderer`が持てる粒は最大**2000個**(`ParticleRenderer::maxParticles`)。超える分は新しく出しません(出ている粒が消えれば、また出ます)
- 粒の動きはシーンの時間(`Time::GetDeltaTime()`)で進みます。ポーズ(`SetTimeScale(0)`)中は粒も止まり、スローなら粒もゆっくりになります
- 画像は`Resources`で共有されるので、爆発を何十回出しても画像の読み込みは1回です

### Sampleでの使われ方

両サンプルの`EnemyController::OnTriggerEnter()`が、弾に当たって倒された位置で`CreateExplosion()`(`Game/Objects/Explosion.cpp`。Sample2DとSample3Dに、それぞれ別の設定で置いてある)を呼びます。`CreateExplosion`は、オレンジ色の加算合成の粒を全方向に出す`CreateEffect`+`Burst`です(Sample3Dは`use3D = true`で、重力で少し落ちる)。粒が消えるとエフェクトのオブジェクトは自動で消えます。Sceneには置きません(敵を倒した時に呼びます)。

なお、`EnemyController`の弾に当たった処理の先頭で「もう消えている(`GetIsActive()`がfalse)なら何もしない」を確認しています。同じフレームに2発当たっても、「倒した」の知らせ(`"EnemyDefeated"`)・音・爆発が2回出ないようにするためです。

---

## 27. FBXモデル(Model / ModelRenderer)

Mayaなどで作ったモデル(`.fbx`)を、パーツ(ノード)の親子関係を保ったまま読み込んで表示する仕組みです。読み込みには外部ライブラリ **ufbx**(`ThirdParty/ufbx/`。バージョン0.23.1。[github.com/ufbx/ufbx](https://github.com/ufbx/ufbx)。MITかパブリックドメインのどちらかを選べるライセンスで、条文は `ThirdParty/ufbx/LICENSE`)を使っています。`ufbx.h` / `ufbx.c` は**書き換えないでください**(更新する時は、新しい版のファイルに丸ごと置き換えます)。

> **今は「段階1: 読み込み(静止)」「段階2: パーツの動き(ノードアニメーション)」「段階3: 骨のスキンアニメーション」まで、すべて使えます。** 形と画像を表示し、パーツを手で動かすことも、Mayaで打ったキーフレームを再生することも、骨に合わせて体を曲げることもできます(アニメーションは下の「アニメーション」、骨は「骨で曲げるモデル」)(`docs/EngineRoadmap.md` の 5.16)。

### 使い方

```cpp
// オブジェクトにFBXを付ける(モデルの読み込みは中で自動。同じファイルは1回だけ読んで使い回す)
auto* renderer = obj->AddComponent<ModelRenderer>(L"Assets/Models/Tank.fbx");
renderer->SetTexture(L"Assets/tank.png");        // 全パーツに同じ画像を貼る(省略すると、FBXに書かれたパーツごとの画像)
renderer->SetColor(1.f, 0.5f, 0.5f, 1.f);        // 色(MeshRendererと同じ。白=そのまま)

// パーツを名前で探して、手で動かす(砲台だけ回す)
Transform* turret = renderer->GetNodeTransform("Turret");
if (turret) turret->rotate.y += 90.f * dt;       // 見つからない時はnullptr
```

| 関数 | 説明 |
|---|---|
| `ModelRenderer(パス)` | モデルのファイル(`.fbx`。`.obj` も読めて、その場合はパーツ1つのモデルになる)。読み込めなかった時は何も描かず、出力ウィンドウに理由が出る |
| `SetTexture(パス)` | 全パーツに同じ画像を貼る |
| `SetColor(r, g, b, a)` | 色 |
| `GetNodeTransform(名前)` | パーツの動き(`Transform`)を返す。無い名前は `nullptr` |
| `Model::Load(パス)` / `GetNodeCount()` / `FindNode(名前)` | モデルのデータそのもの(`Engine/Render3D/Model.h`)。普通は `ModelRenderer` が使うので、直接使うことは少ない |
| `Resources::LoadModel(パス)` | モデルを借りる。同じパスは同じ `Model` を返す(§11)。`ModelRenderer` が中で自動で使う |

### 決まり(重要)

- **パーツの動きは「親のパーツからの相対」です。** `GetNodeTransform` が返す `Transform` の `position` / `rotate`(度数) / `scale` は、親のパーツから見た値で、一番上のパーツだけは「このオブジェクトの `Transform`」から見た値です。親のパーツを動かすと、子のパーツも一緒に動きます。最初の値は、FBXに入っていた値になっています。
- **この `Transform` には親子関係(`SetParent`)を設定していません。** 値を読み書きするだけで使ってください(`GetWorldMatrix()` を呼んでも、親の動きは入りません)。オブジェクトの位置・向き・大きさは、今まで通り `obj->transform` で動かします。
- **モデルのデータと、パーツの動きは別です。** 形や親子関係(`Model`)は同じファイルのオブジェクトみんなで共有し、パーツの今の位置・回転は `ModelRenderer` が1体ごとに持ちます。同じ戦車を3台置いて、それぞれ砲台の向きを変えられます。
- 描画は `MeshRenderer` と同じ仕組み(`Shader3D.hlsl`、3D用カメラ、シーン共通の光源)です。パーツごとに「親→子へ累積したワールド行列」で描きます。
- 読み込んだモデルは、画像と同じく `Resources` が管理します。誰も使わなくなったモデルは、シーン切り替えの時に解放されます。

### 単位と座標系(読み込む時に変換します)

- **単位**: このエンジンは1単位=1cmです。FBXの中に書かれた単位の設定を読んで変換するので、Mayaの既定(cm)ならそのままの数字、mで作った物は100倍されます。
- **座標系**: Mayaなどの右手系・Y上を、このエンジンの左手系・Y上に変換します(Zの向きを反転し、三角形の頂点の順番や法線も直します)。Z上で作ったデータ(Blenderなど)も、向きが合うように変換されます。たとえばMayaで `Z=+100` に置いた物は、このエンジンでは `Z=-100` になります。
- **三角形化**: 四角形以上の面は、読み込む時に三角形に分けます(Mayaで事前に Triangulate しなくてもかまいません)。
- **UV**: 上下(V)を反転して読みます(`LoadOBJ` と同じ)。法線が無いモデルは、ufbxが自動で作ります。

### アニメーション(ModelAnimationComponent)

Mayaで打ったキーフレーム(パーツの位置・回転・スケール)を再生します。`ModelRenderer` を付けた**後に**、同じオブジェクトへ付けます(骨に合わせて体が曲がるスキンも、同じ使い方で動きます。次の「骨で曲げるモデル」)。

```cpp
auto* renderer = tank->AddComponent<ModelRenderer>(L"Assets/Models/Tank.fbx");
auto* anim = tank->AddComponent<ModelAnimationComponent>();    // ModelRendererの後に付ける
anim->Play("Idle");                                            // Mayaで付けたクリップ名

// PlayerController::Update など
if (moving) anim->CrossFade("Run", 0.2f);                      // 今のポーズから0.2秒で滑らかに切り替え
else        anim->CrossFade("Idle", 0.2f);                     // 再生中と同じ名前なら何もしない(毎フレーム呼んでよい)

anim->Play("Attack", false);                                   // ループしない。終わったら最後のポーズで止まる
if (anim->IsFinished()) anim->CrossFade("Idle", 0.2f);
```

| 関数 | 説明 |
|---|---|
| `Play(クリップ名, ループ = true)` | 最初から再生する。**再生中の同じクリップなら何もしない**(毎フレーム呼んでも最初に戻らない)。終わったクリップなら、もう一度最初から |
| `CrossFade(クリップ名, 秒)` | 今のポーズから新しいクリップへ、指定の秒数で滑らかに切り替える(新しいクリップはループ再生)。再生中と同じ名前なら何もしない。秒が0以下なら、すぐ切り替える |
| `IsFinished()` | ループしない動きが最後まで終わったか |
| `SetSpeed(倍率)` | 再生の速さ(1=等倍、0=止まる。0未満は0) |
| `GetClipNames()` | 入っているクリップ名の一覧(確認用) |

- **クリップ名はMayaで付けた名前**です(FBXの中の「アニメーションスタック」の名前)。名前が違う・クリップが無い時は、出力ウィンドウに知らせるだけで何も起きません(落ちません)。
- 時間は `Update` の `dt`(`Time` の倍率が掛かった時間)で進むので、`Time::SetTimeScale(0)` のポーズ中は止まり、スローならゆっくりになります。
- 補間は、位置・スケールは直線、回転は球面線形補間(クォータニオンのslerp)です。読み込む時に、全パーツの動きを1秒30個の細かさで読み取って持ちます(1つのクリップは最大20000個=約11分まで)。
- 位置は1単位=1cm、座標系は静止モデルと同じ変換をしてあります(Mayaの `Z` が反転するので、Y軸まわりの回転も向きが逆になって表示されます。Mayaで見た通りの動きになります)。
- **手で動かした時の優先順位**: アニメーションを再生している間は、毎フレームアニメーションが `GetNodeTransform` のパーツを上書きします(手で動かしても次のフレームにアニメの値に戻る)。ループしない動きが終わった後は書き込まなくなるので、そのパーツを手で動かせます。アニメーションが動かさないパーツ(キーの無いパーツ)も、最初のポーズ(キーが無ければFBXの値)を毎フレーム書き込みます。手で動かしたい場合は、そのモデルにアニメーションを付けないか、`SetEnabled(false)` で止めてください。
- 同じモデルのオブジェクトを何体置いても、クリップのデータは共有され、再生の時刻・切り替えは1体ごとに別です。

### 骨で曲げるモデル(スキンアニメーション)

人や動物のように、**骨に合わせて体が曲がる**モデルも、そのまま `ModelRenderer` + `ModelAnimationComponent` で動かせます(コードは変わりません。読み込んだFBXに骨とスキンが入っていれば、自動で曲げて描きます)。

- **仕組み**: 頂点に「どの骨に・どれだけ影響されるか」を持たせ、毎フレーム、骨の今の位置(アニメーションで動いたパーツ)から「スキン行列(逆バインド行列 × 骨の行列)」を作ってGPUへ送り、頂点シェーダーで頂点を動かします。骨のパーツ自体は形が無いので描かれず、メッシュ(体)だけが曲がります。
- **新しいシェーダー**: `Shader3DSkin.hlsl`(`Shader3D.hlsl` と同じ場所。`Engine.vcxproj` の `CopyRuntimeFiles` が出力先へコピーします)と、描画クラス `Engine/Render3D/SkinMesh.h/.cpp`、頂点形式 `SkinVertex`(位置・法線・色・UV + 骨の番号4つ + 影響度4つ)です。**今の `MeshVertex` / `Shader3D.hlsl` / `Shader.hlsl` は変えていません。** ライティング・色・テクスチャの計算は `Shader3D.hlsl` と同じです(今は両方が共通の `Shader3DCommon.hlsli` を使っています。§30)。シェーダーは実行時にコンパイルされ、エラーは出力ウィンドウに出ます。
- **頂点バッファは読み込み時に1回だけ作ります**(変更不可)。毎フレームの頂点のコピーは無く、骨の行列(最大64本)だけを送ります。同じモデルを何体置いても頂点バッファは共有です。
- **上位4つに絞られます**: 1つの頂点に影響できる骨は最大**4本**です。それより多く影響していると、影響度の大きい上位4つだけを使い、合計が1になるように直します(Mayaの Bind Skin の「Max Influences」を **4以下**にしておくと、見た目が変わりません)。
- **骨の最大数は、1つのメッシュにつき64本**です。超えるメッシュは、出力ウィンドウに知らせて、曲げずに静止の形のまま表示します(骨の数を減らしてください)。
- **Mayaでの手順**: ①Skeleton(Skeleton → Joint Tool)で骨を作る → ②メッシュを選んでから骨を選び、Bind Skin(Skin → Bind Skin。**Max Influences は4**、Maintain Max Influences をオン)→ ③骨にキーを打つ → ④FBXの書き出しで **Skins と Animation をオン**(Deformed Models → Skins)。Bake Animation もおすすめです。骨には名前を付けておくと、`GetNodeTransform("名前")` で手でも動かせます。
- **パフォーマンス**: 頂点の計算はGPUで行うので、骨が動くだけなら軽いです。ただし、骨のパーツの行列計算は毎フレーム全パーツぶん、スキン行列の送信はスキンのメッシュごとに1回あります。同じモデルを大量に置くとその分増えるので、置く数は様子を見て決めてください。また、アニメーションのデータは全パーツ全フレームを持つので、骨が多く長いクリップはメモリを使います。
- **制限**: 1つのモデル(ノード)に付くスキンは最初の1つだけです。ブレンドシェイプ(顔の表情など)・デュアルクォータニオンスキニングには対応していません(リニアブレンドスキニングです)。スキンのメッシュのパーツ自体を `GetNodeTransform` で動かしても、頂点は骨に付いて動くので見た目は変わりません(骨のパーツを動かしてください)。
- 動作確認用に、骨2本で曲がる細長い箱 `Assets/Models/TestSkin.fbx`(手書きのASCII FBX。クリップ「Bend」=先端の骨が曲がって戻る)があります。Sample3Dの `CreateSkinDisplay()`(`Game/Objects/SkinDisplay.cpp`)が `(1000, -50, 800)` に置いて再生します(ファイルが無い時は何も置きません)。

### Mayaでの書き出し設定

- 形式は **FBX**(バイナリでもASCIIでも読めます)。Up Axis は **Y**、単位は **センチメートル**(違っていても変換されますが、cmが一番分かりやすい)
- **Triangulate** はオンでもオフでもかまいません
- **Embed Media(メディアを埋め込む)は使いません。** 画像は別に **PNG** で `Assets/` に置きます
- 画像の貼り方: FBXに書かれた画像の**ファイル名だけ**を使い、`Assets/ファイル名` から探します(例: Mayaで `tank_body.png` を貼ったら `Assets/tank_body.png`)。フォルダ名は無視します。PNG以外(tga など)は読めないので、PNGにして `Assets/` に置いてください。パーツに画像が無ければ白で表示されます(`SetTexture` や `SetColor` で変えられます)
- 1つのパーツ(メッシュ)に複数のマテリアルを付けても、貼れる画像は**1つだけ**です(画像のある最初のマテリアルのもの)
- モデルのファイルは `Assets/Models/` に置くとよいです。元の `.mb` も `Assets/Source/` などに残しておくと、後で直せます
- アニメーション(キーフレーム)の作り方と書き出し: Mayaでパーツ(グループやメッシュ)を動かしてキーを打ち、FBXの書き出しで **Animation をオン**にします。Maya側では、**Bake Animation(アニメーションのベイク)をオン**にするのがおすすめです(IK・拘束・エクスプレッションなどを使っていても、キーの値に焼き込まれて確実に再生できる)。複数の動き(歩く・走る・攻撃)は、**Game Exporter でクリップ(Take)に分けて**書き出すか、1つずつ別のクリップとして書き出します。クリップの名前がそのまま `Play("名前")` の名前になります。骨(スキン)で体を曲げる動きは、次の「骨で曲げるモデル」の手順で書き出します

### 注意

- パーツは名前(Mayaのオブジェクト名)で探します。Mayaで名前を変えると `GetNodeTransform("Turret")` が `nullptr` になるので、コード側も直してください。同じ名前のパーツが2つあると、最初の1つだけが見つかります
- Mayaが作る空のグループなども、パーツとして数えられます(形が無いだけで、親子関係には使われます)
- ufbx は大きなCのファイル(`ufbx.c`)なので、`Engine.vcxproj` で「Cとしてコンパイル・警告なし・Debugでも最適化」にしてあります(最適化しないとDebugでの読み込みが遅くなるためです)

### Sampleでの使われ方

Sample3Dの `CreateModelDisplay()`(`Game/Objects/ModelDisplay.cpp`)が、`Assets/Models/TestAnim.fbx`(立方体2つ。親の「Base」と、その子の「Top」。クリップ「Spin」=Topが1回転(2秒)、「Bob」=Topが上下(1秒))を地面の端(位置 `(800, -50, 800)`)に置き、`ModelAnimationComponent` で再生します。`ModelDisplayController` が3秒ごとに `CrossFade("Spin" / "Bob", 0.3f)` で切り替えます。`TestAnim.fbx` が無い時は静止の `Assets/Models/TestParts.fbx` を置き(`ModelDisplayController` が `GetNodeTransform("Top")` の `rotate.y` を増やしてTopだけを回します)、どちらも無い時は何も置かず出力ウィンドウに知らせるだけなので、ファイルを消してもゲームは動きます。2つのFBXは、動作確認のために手で書いた最小のASCII形式のFBXです(Mayaで書き出した物ではありません)。 骨で曲げる見本は `CreateSkinDisplay()`(`Game/Objects/SkinDisplay.cpp`)が `Assets/Models/TestSkin.fbx` を `(1000, -50, 800)` に置いて再生します(上の「骨で曲げるモデル」)。

---

## 28. 視界判定(Visibility / VisionComponent)

「カメラに映っているか」と「敵の視野(扇形)に入っているか」を調べる仕組みです(`docs/EngineRoadmap2.md` の 6.1)。2つに分かれています。

| 調べたいこと | 使う物 | 例 |
|---|---|---|
| 画面の外にある物を描かない・処理を省く | `Visibility`(関数)と、各 Renderer の `SetFrustumCulling` | 画面の外の敵のスプライトを描かない |
| 見張り役の視野に相手が入ったか(壁にさえぎられていないか) | `VisionComponent`(Component) | ゆっくり回る見張りが、Playerを見つけたら赤くなる |

### (a) カメラに映っているか: Visibility

```cpp
// 3D: 今の3Dカメラの視錐台(カメラに映る台形の空間)に、球・箱が少しでも入っているか
if (!Visibility::IsSphereInView3D(enemy->transform.position, 100.f)) return;   // 画面の外なら何もしない

// 2D: 今の2Dカメラの画面に、円・四角が少しでも入っているか
bool onScreen = Visibility::IsCircleInView2D(bullet->transform.position, 8.f);

// Rendererに任せる(画面の外なら描かない。物理は止まらない)
sprite->SetFrustumCulling(true);
```

| 関数 | 説明 |
|---|---|
| `IsSphereInView3D(中心, 半径)` | 3Dカメラ(`Image::GetCamera3D()`)の視錐台に、球が少しでも入っているか |
| `IsBoxInView3D(中心, 半分の大きさ)` | 同じく軸に沿った箱(回転しない)。半分の大きさは、辺の長さの半分 |
| `IsCircleInView2D(位置, 半径)` | 2Dカメラ(`Image::GetCamera()`)の画面に、円が少しでも入っているか。位置はワールド座標(`z`は使わない) |
| `IsRectInView2D(位置, 半分の大きさ)` | 同じく四角(回転しない) |
| `GetCulledCount()` | 前のフレームに、描かずに省いた数(HUDに出して確認する) |
| `SpriteRenderer` / `MeshRenderer` / `ModelRenderer` の `SetFrustumCulling(bool)` | 画面の外なら描かない。**既定は `false`(今までと同じ。全部描く)** |

**2D と 3D の違い**
- 3Dは「視錐台」と「球・箱」で調べます。視錐台は 1フレームに1回、`GameEngine` が `CameraShake::BeginDraw()` の後に作ります(揺れが入ったカメラで判定するので、画面の端で揺れても一瞬消えたりしません)。ゲーム側が準備を呼ぶ必要はありません。
- 2Dは「2Dカメラの画面(画面中央が(0,0)、`Y+`=上、単位はピクセル)」と「円・四角」で調べます。`SpriteRenderer`(UIモード)と同じ計算(`z`は大きさに影響しない)を使います。2Dカメラは毎回その場の値を読むので、準備は要りません。
- どちらも、最初のフレーム(準備の前)は「映っている」を返します(消さない側に倒しています)。

**Rendererの画面外判定(`SetFrustumCulling(true)`)の決まり**
- `SpriteRenderer`: `transform.scale` の幅・高さの対角線の半分を半径にした円(UI・2Dモード)か球(3Dモード=`SetWorldSpace(true)` のビルボード)で調べます。`scale` が0以下で画像の大きさを使う時は、大きさが分からないので判定しません(必ず描く)。
- `MeshRenderer`: 頂点の「原点から一番遠い点」×最大の拡大率の球で調べます(立方体なら約0.87×`scale`)。`SetDepthWrite(false)`(スカイボックスなど画面全体を覆う物)は、`true` でも判定しません。
- `ModelRenderer`: 読み込み時に求めた「止まっている姿の一番遠い頂点」(`Model::GetBoundingRadius()`)×1.5(余裕)の球で調べます。骨・アニメーションで大きく動くモデルは、画面の端で消えて見えることがあるので、その時は `false` に戻します。
- 描画を省くだけで、物理(当たり判定)は止まりません。物が突然消えて見えたら、半径が小さすぎます(`GetCulledCount()` を見ながら確認)。
- 影の描画(将来)では、カメラの外にある物も影を落とすので、この判定は使いません。

### (b) 敵の視野: VisionComponent

敵などに付けて、「扇形(角度と距離)の中にいて、壁にさえぎられていないか」を調べます。見えた後の動き(追いかける・色を変える)は、付けた側のComponentが決めます。

```cpp
auto* vision = guard->AddComponent<VisionComponent>();
vision->SetFieldOfView(90.f);               // 視野の角度(全体)。90なら左右45度ずつ
vision->SetRange(1500.f);                   // 見える距離
vision->SetPlane(MovePlane::XZ);            // 見る平面(3Dの地面=XZ、2D=XY)
vision->SetEyeOffset({ 0.f, 50.f, 0.f });   // 目の位置(足元ではなく胸の高さなど)

// 動き側のComponentで
bool seen = vision->CanSee(player);         // 見えたら赤くする・追いかけ始める
```

| 関数 | 説明 |
|---|---|
| `SetFieldOfView(度)` | 視野の角度(全体。0〜360。既定90) |
| `SetRange(距離)` | 見える距離(既定1000) |
| `SetPlane(MovePlane)` | 見る平面(`MoveComponent` と同じ `MovePlane`。既定は `XY`)。角度・距離は、この平面の上だけで考える(XZなら高さ`Y`の差は無視) |
| `SetEyeOffset(オフセット)` | 目の位置(オーナーの位置からの相対。既定は0) |
| `SetObstacleTag(タグ)` | このタグの物は、視線をさえぎる壁(既定 `"Wall"`)。`""` にすると壁は考えない |
| `SetFacing(向き)` | 正面を手で指定する。長さ0に戻すと `transform` の向きを使う |
| `SetCheckInterval(秒)` | 壁の判定(Raycast)をやり直す間隔(既定0.1秒。`CanSee(GameObject*)` だけ) |
| `IsInCone(点)` | 扇形(角度と距離)の中か。壁は考えない |
| `CanSee(点)` | 扇形の中で、壁にさえぎられていないか。呼ぶ度にRaycastする |
| `CanSee(GameObject*)` | 上と同じ(相手の位置を使う)。壁の判定は `SetCheckInterval` の間隔で覚えた結果を使う(角度と距離は毎回調べる) |
| `GetFacing()` | 今の正面(長さ1) |
| `VisionComponent::SetDebugDrawEnabled(bool)` | 扇形を線で描く(見えている時は赤、見えていない時は緑)。`RigidbodyComponent::SetDebugDrawEnabled` と同じ使い方 |

**決まり(重要)**
- **正面**: `SetFacing` で指定した向きが最優先です。指定が無ければ、`XZ` は `rotate.y`(`Math::Forward3D`)、`XY` は `rotate.z`(`Math::Forward2D`)を使います。Sample3Dの敵のように `rotate` を使わない物(ビルボード)に付ける時は、毎フレーム `SetFacing`(進む向き)を呼んでください。
- **壁**: 目から相手へ `Physics::Raycast(…, 壁のタグ)` を飛ばして、相手より手前に壁があればさえぎられています。**壁のオブジェクトにタグ `"Wall"` を付け忘れると素通しになります。** 地面・ほかの敵・弾は壁になりません(斜め下を見る視線は地面にさえぎられません)。
- 相手の「1点(位置)」だけを見ます。相手の中心が壁の陰なら、体が少しはみ出していても見えません。
- Raycastは1つ前の `Physics::Update` の位置を使うので、動いたばかりの壁には1フレーム遅れて反映されます。壁の中に目があると、壁に当たらず見えてしまいます。
- 敵が何体もいても重くならないよう、壁の判定は `SetCheckInterval`(既定0.1秒、±20%ばらす)ごとにやり直し、間は覚えた結果を返します。

### Sampleでの使われ方

- **見張り役 `Guard` と壁 `Wall`**(Sample2D・Sample3Dどちらにも): `GameScene::Init()` は `CreateWall(...)` と `CreateGuard(*this, player)` を並べるだけ。`Objects/Guard.cpp` が `VisionComponent`(Sample3Dは `XZ`、Sample2Dは `XY`)と見た目と `GuardController` を付け、`GuardController::Update()` が回転(1秒に40度)と、`vision->CanSee(player)` で見えた時に赤くする(見えていない時は青)処理を担当します。`Objects/Wall.cpp` はタグ `"Wall"` のStaticの箱で、見張り役とPlayerの間に1枚、離れた所に1枚置いてあります。`GameManager::Init()` で `VisionComponent::SetDebugDrawEnabled(true)` にしているので、扇形が線で見えます(不要なら `false` に)。
- **画面外の判定**: Sample3Dの `GroundController::Draw()` は、地面のタイルを `Visibility::IsSphereInView3D` でカメラに映る物だけ描きます(以前の「描画距離の円」の手書き判定の置き換え)。両サンプルの敵(`SetupEnemy`)は `SetFrustumCulling(true)` で、画面の外にいる間は描きません。HUD(`HUDController`)の「Culled: n」が、前のフレームに描かずに省いた数です。

---

## 29. カプセルコライダー(CapsuleRigidbodyComponent / Capsule2DRigidbodyComponent)

円柱の両端が半球になった形の当たり判定です(`docs/EngineRoadmap2.md` の 6.2)。立った人や敵に向いています。箱と違って角が無いので、段差や坂道で引っかからず、滑らかに歩けます。

| 使う Component | 使う場面 |
|---|---|
| `CapsuleRigidbodyComponent` | 3Dのゲーム |
| `Capsule2DRigidbodyComponent` | 2Dのゲーム。中身は同じで、`Circle`/`Square` と同じく `SetFreezePositionZ(true)` で使い、枠線表示は断面だけ |

```cpp
// 3D: 立って歩く敵(半径40・高さ100)
auto* rigidbody = obj->AddComponent<CapsuleRigidbodyComponent>(BodyType::Dynamic, 40.0f, 100.0f, 1.0f);
rigidbody->SetFreezeRotation(true);   // 人・敵には必須(付けないと、ぶつかった時に倒れて転がる)

// 2D: 横長のカプセル(半径20・全体の長さ100)
auto* rb = obj->AddComponent<Capsule2DRigidbodyComponent>(BodyType::Dynamic, 20.0f, 100.0f, 1.0f,
    false, false, CapsuleAxis::X);
rb->SetFreezePositionZ(true);
rb->SetFreezeRotation(true);
```

引数の並びは `(BodyType, radius, height, density, isTrigger, isStaticForPush, axis)` です。

| 引数 | 意味 |
|---|---|
| `radius` | 半径。既定(0)ならOwnerの `transform.scale.x` の半分 |
| `height` | **全体の高さ**(両端の半球を含む。Unityと同じ)。既定(0)なら `transform.scale.y`。`2×radius` より小さい時は、ほぼ球になる |
| `axis` | 長い向き。`CapsuleAxis::Y`(既定=立つ)/ `X` / `Z`。2Dでは `Y`(縦長)か `X`(横長)を使う |
| `density` など | 他の形(`Box`/`Sphere`)と同じ |

見た目用のメッシュは `Mesh::CreateCapsule(radius, height)` で作れます(`MeshRenderer` に渡す)。立方体・球と違って、両端の半球が潰れないよう **実際の大きさで作る** ので、`transform.scale` は `{1, 1, 1}` のままにします。

```cpp
obj->transform.scale = { 1.f, 1.f, 1.f };
obj->AddComponent<MeshRenderer>(Mesh::CreateCapsule(50.f, 180.f));
obj->AddComponent<CapsuleRigidbodyComponent>(BodyType::Dynamic, 50.f, 180.f, 100.f);
```

### 気を付けること

- PhysXのカプセルは元々 **X軸方向に伸びる** ので、`axis` が `Y`/`Z` の時は、Engineが形(shape)だけを回して向きを変えています(Y: Z軸まわりに+90度、Z: Y軸まわりに−90度)。Ownerの `transform.rotate` や、アクターの回転とは別です。`PlayerController` のように `setGlobalPose` で `rotate.y` に合わせて回す使い方は、立ったカプセルがY軸まわりに回るだけなので、そのまま使えます。
- カプセルは `radius > 0` かつ円柱部分の長さ(`height − 2×radius`)が 0 より大きくないとPhysXで無効な形になるため、足りない時は円柱部分に小さな長さを足しています。
- `Physics::OverlapSphere` / `OverlapBox` は、カプセルの形にも当たります。カプセルで調べる関数は、まだありません。

### 枠線表示(デバッグ表示)

`RigidbodyComponent::SetDebugDrawEnabled(true)` のとき(§6)、カプセルも緑/黄の枠で表示されます。大きさで形が変わるので、立方体・球のように「大きさ1の枠をscaleで伸ばす」ことができず、`Mesh::CreateCapsuleOutline(radius, halfHeight, axis, segments, crossSectionOnly)` で実際の大きさの枠を作って、各 `RigidbodyComponent` が自分用に持っています(初めて描く時に作り、`Uninit` で解放)。`Capsule2D` は断面(角の丸い四角)だけです。

### Sampleでの使われ方

- **Sample3D の敵**: `EnemyManager` の `SetupEnemy` で、箱(`BoxRigidbodyComponent`)の代わりに `CapsuleRigidbodyComponent`(半径40・高さ100)を付けています。見た目の画像(100×100)より横が少し細い判定です。
- Sample2D、Sample3D の Player は、今は円・箱のままです(Playerの見た目は立方体のため)。

---

## 30. マテリアル(Material)

3Dメッシュの見た目の設定(色・画像・光沢・自発光・半透明)を、1つの `Material`(`Engine/Render3D/Material.h`)にまとめました(`docs/EngineRoadmap2.md` の 6.3)。`MeshRenderer` と `ModelRenderer` が1体ごとに持ちます。**何も設定しなければ、マテリアルが無かった頃と見た目は同じです**(ハイライトなし・自発光なし)。

### 使い方

```cpp
auto* meshRenderer = sphere->AddComponent<MeshRenderer>(Mesh::CreateSphere());
meshRenderer->SetTexture(L"Assets/grid.png");
meshRenderer->GetMaterial().specularStrength = 0.8f;   // つやつや(光が当たる所が光る)
meshRenderer->GetMaterial().shininess = 64.f;          // ハイライトを小さく鋭く

// まとめて設定したい時は、Materialを作って渡す
Material material;
material.color = { 0.4f, 0.7f, 1.0f, 0.4f };    // aが透明度
material.texture = L"Assets/grid.png";           // 画像のパス(Resourcesで読む。空なら白)
meshRenderer->SetMaterial(material);
```

`ModelRenderer`(FBXモデル。§27)にも、同じ `SetMaterial` / `GetMaterial` があります。`material.texture` が空なら、FBXに書かれたパーツごとの画像をそのまま使い、パスを入れると全パーツにその画像を貼ります(`SetTexture` と同じ)。

今までの書き方も、そのまま使えます。中でマテリアルの項目を書き換えているだけです。

| 今までの書き方 | マテリアルでの意味 |
|---|---|
| `SetColor(r, g, b, a)` / `GetColor()` | `material.color` |
| `SetTexture(パス)`(`ModelRenderer` は全パーツに貼る) | `material.texture`(パスを入れると、`Resources` で読んで借りる) |
| `SetUnlit(true)`(`MeshRenderer`) | `material.lit = false` |
| `SetDepthWrite(false)`(`MeshRenderer`) | `material.depthWrite = false` |

### 項目と既定値

| 項目 | 意味 | 既定 |
|---|---|---|
| `color` | 色。**aが透明度**(1=不透明)。頂点の色とテクスチャに掛ける | `{1, 1, 1, 1}` |
| `texture` | 画像のパス(空なら白)。`MeshRenderer`/`ModelRenderer` が読み、手で書き換えても次に描く時に切り替わる | 空 |
| `lit` | `false` にすると光の影響を受けず、いつも一定の明るさ(スカイボックス) | `true` |
| `depthWrite` | `false` にすると深度バッファに書き込まない(自分より後ろの物を隠さない。半透明・スカイボックス) | `true` |
| `specularColor` | 光沢(ハイライト)の色 | 白 |
| `specularStrength` | 光沢の強さ。**0=ハイライトなし**。0.5〜1くらいで目立つ | 0 |
| `shininess` | 光沢の鋭さ。大きいほどハイライトが小さく鋭い(4=ぼんやり広い、64=小さくくっきり) | 32 |
| `emissiveColor` | 自分で光る色 | 白 |
| `emissiveStrength` | 自発光の強さ。**0=光らない**。暗い所でもその色で見える | 0 |
| `castShadow` | `true` にすると、影を落とす(`Shadow` が有効な時だけ意味がある。§32) | `false` |
| `receiveShadow` | `true` にすると、影を受ける(太陽・スポットの光が、影で暗くなる。§32) | `false` |
| `rimColor` | 縁の光の色(§33) | 白 |
| `rimStrength` | 縁の光の強さ。**0=光らない**(§33) | 0 |
| `rimPower` | 縁にどれだけ集まるか。大きいほど縁の細い線だけが光る(§33) | 3 |
| `outlineColor` | 枠線の色。**aは濃さ**(1=不透明。§33) | `{1, 1, 0, 1}`(黄) |
| `outlineWidth` | 枠線の太さ(**画面のピクセル**)。**0=枠線なし**(§33) | 0 |

- **光沢**は、光とカメラの真ん中の向きに法線が近いほど光る計算(ブリン・フォン)です。光の色(`Image::GetLight()` の `color`)も掛けます。`lit = false` の時は出ません。
- **自発光**は、光の計算の後に色を足すだけです。画面全体ににじむ本物の「光る感じ」(ブルーム)ではありません。
- ライト(ポイントライト・スポットライト)は `Material` の項目ではなく、別の部品 `LightComponent` です(§31)。**光沢は、ポイントライト・スポットライトにも効きます**(同じ `specularColor` / `specularStrength` / `shininess`。光の色と強さ・距離の減衰も掛かる)。`lit = false` の物は、これらの光の影響も受けません。
- **影**は `castShadow`(影を落とす)・`receiveShadow`(影を受ける)の2つの項目で指定します(**どちらも既定は `false`**。`Shadow` を有効にした時だけ意味があります。§32)。
- **縁の光**(`rim*`)と**枠線**(`outline*`)も `Material` の項目です(**どちらも既定はオフ**)。`MeshRenderer` / `ModelRenderer` の `SetRimLight` / `SetOutline` が、これらを書き換えます。使い方と注意は§33。

### 半透明にする時

`color.a` を1より小さくすれば半透明になります(ブレンドは前から有効です)。ただし次の点に気を付けてください。**自動では何も変えません**(`color.a` を変えるだけで `depthWrite` や `drawPriority` を勝手に変えることはしません)。

```cpp
panel->SetDrawPriority(10);          // 不透明な物より後に描く(大きいほど後)
material.color.w = 0.4f;             // 透明度
material.depthWrite = false;         // 自分より後ろの物を隠さない
```

- 描く順番は `drawPriority` → `z` の小さい順で、**カメラからの距離順ではありません**(UIモードのスプライトは別扱いで、最後に必ず手前へ描かれます。§4)。半透明の物は、不透明の物より後に描くように `SetDrawPriority` を大きくしてください。
- 半透明の物どうしの前後は、見る角度によってずれることがあります(距離で並べ替える処理が無いため)。
- `clip` で、ほぼ透明なピクセルは今まで通り描きません。
- 波打つ水面は、`Material` ではなく専用の `WaterRenderer`(§35)で作ります(色・透明度・光沢は `WaterSettings` で決めます)。

### Mesh の直接の使い方(Engineを触る人向け)

`Mesh::Draw` / `SkinMesh::Draw` に、`Material` を受け取る版があります。今までの `color` を渡す版も、既定のマテリアルで新しい版を呼ぶだけなので、そのまま使えます(`RigidbodyComponent` の枠線表示は変えていません。`GroundController` は、影を受けるために `Material` 版へ変えました。§32)。

```cpp
mesh.Draw(transform, camera, material, light);          // マテリアル版
mesh.Draw(transform, camera, {1, 1, 1, 1}, light);      // 今まで通り(色だけ)
```

`Material::texture` のパスは `Mesh::Draw` では使いません。画像は `Mesh::SetTexture(SRV)` で渡します(`MeshRenderer`/`ModelRenderer` が、`Resources` で読んだ画像を渡しています)。

### Meshの作り直し(頂点を毎回コピーしない)

マテリアルと同時に、`Mesh` を地形・影にも使える作りに直しました。

- **頂点バッファは作る時に1回だけ送ります(変更不可)**。以前は描く度に頂点を全部コピーして色を上書きしていました。**色は頂点ではなく、マテリアルの色として(定数バッファ `b2` で)渡し**、頂点の色に掛けます(今ある形は頂点の色がすべて白なので、見た目は同じです)。
- **インデックスバッファに対応**: `mesh.InitIndexed(頂点, インデックス)` で、同じ頂点を使い回せます(立方体なら36頂点が24頂点+36個の番号に)。頂点が多い形(地形)で頂点の数が大きく減ります。`Init(頂点, topology)` の呼び方は今まで通りです。
- 輪郭の線(`LINELIST`)や `CreateCube` など、今ある作り方は変わりません。

### シェーダーの共通化(Shader3DCommon.hlsli)

`Shader3D.hlsl`(普通のメッシュ)と `Shader3DSkin.hlsl`(骨で曲げるFBX)で、見た目の計算(光・色・テクスチャ・光沢・自発光・縁の光)は、共通の `Shader3DCommon.hlsli` の `ShadePixel()` に書いてあります(枠線用の `OutlineClipPos()` / `ShadeOutline()` もここです。§33)。**見た目の計算を変える時は、ここだけを直せば、2つのシェーダーが同じ結果になります。**

| レジスタ | 中身 |
|---|---|
| `b0` | ワールド・ビュー・射影・平行光源(今まで通り) |
| `b1` | 骨の行列(`Shader3DSkin.hlsl` だけ。色は `b2` に移した) |
| `b2` | マテリアル(色・光沢・自発光・光を使うか・縁の光・枠線。`float4` が7本=112バイト) |
| `b3` | カメラの位置(光沢のため) |
| `b4` | ポイントライト・スポットライト(最大8個。§31。`Lighting` が持つ) |
| `b5` | 影(光から見た行列・バイアスなど。§32。`Shadow` が持つ) |
| `t0` / `s0` | 画像 / サンプラー |
| `t1` / `t2` / `s1` | 太陽の影・スポットの影の深度テクスチャ / 比較サンプラー(§32) |

- シェーダーは実行時にコンパイルされるので、`Shader3DCommon.hlsli` も実行ファイルの隣に必要です。`Engine.vcxproj` の `CopyRuntimeFiles`(コピー)と `UpToDateCheckInput` に登録済みです(**新しい `.hlsli` を足したら、この2か所にも足すこと**)。`#include` が使えるよう、`D3DCompileFromFile` には `D3D_COMPILE_STANDARD_FILE_INCLUDE` を渡しています。
- 書き方に間違いがあると、何も描かれなくなり、出力ウィンドウにエラーが出ます。

### Sampleでの使われ方

- **Sample3D の `GlassPanel`**(`Objects/GlassPanel.cpp`): 半透明(`color.a = 0.4`・`depthWrite = false`・`SetDrawPriority(10)`)で、光沢と少しの自発光がある水色の板です。位置 `(400, 50, 300)`。
- **Sample3D の `Sphere`**(`Objects/Sphere.cpp`。今のシーンでは未使用): `specularStrength = 0.8`・`shininess = 64` でつやつやの球になります。
- **`Skybox`** は今まで通り `SetUnlit(true)` / `SetDepthWrite(false)`(中でマテリアルの項目を書き換える)。
- 当たり判定のデバッグ表示(Sample2Dにも出る枠線)は、`Mesh::Draw` の色指定版を通ります。見た目は今まで通りです。`GroundController` のタイルは、`receiveShadow = true` の `Material` を渡して描きます(影が無効の間は、見た目は今まで通りです。§32)。

### まだ無いもの

- FBXに書かれたマテリアル(スペキュラ・自発光)の読み込みはしません(今は色と画像だけ)。
- マテリアルをファイルにして使い回す仕組みはありません(1体ごとに持ちます)。

---

## 31. ライト(LightComponent)

今までの光は「全体に同じ方向から当たる光(平行光源。`Image::GetLight()`)」だけでした。そこに、**ランタンのように周りを照らす光(ポイントライト)**と、**懐中電灯のように円すい形に照らす光(スポットライト)**を足せます(`docs/EngineRoadmap2.md` の 6.4)。光を付けたいオブジェクトに `LightComponent`(`Engine/Render3D/LightComponent.h`)を付けるだけです。**光を1つも置かなければ、見た目は今までと1ピクセルも変わりません**(平行光源・環境光のまま)。

### 使い方

```cpp
// ポイントライト(周り全体を照らす。位置はオーナーの位置)
auto* lantern = obj->AddComponent<LightComponent>(LightType::Point);
lantern->SetColor(1.0f, 0.7f, 0.35f);   // 光の色(オレンジ)
lantern->SetIntensity(1.5f);            // 強さ(2なら2倍明るい)
lantern->SetRange(800.f);               // ここまで届く(100単位=1m。800=8m)

// スポットライト(円すい形に照らす)
auto* torch = obj->AddComponent<LightComponent>(LightType::Spot);
torch->SetSpotAngle(15.f, 30.f);        // 内側(全開)15度、外側(消える)30度
torch->SetDirection({ 0.f, -1.f, 0.f }); // 向き(毎フレーム変えてもよい)。指定しなければオーナーの水平の正面
```

オブジェクトと一緒に動きます。`SetActive(false)` で消え、戻すと点きます(`ObjectPool` で休んでいる物の光は使われません)。`Destroy` したり、シーンが変わって `Uninit` されたりすると、登録も自動で外れます(古いポインタは残りません)。

### 項目と既定値

| 関数 | 意味 | 既定 |
|---|---|---|
| `LightComponent(LightType)` | 種類。`LightType::Point`(点)か `LightType::Spot`(スポット) | `Point` |
| `SetColor(r, g, b)` | 光の色 | 白 |
| `SetIntensity(value)` | 強さ。色に掛ける(0以下は0) | 1 |
| `SetRange(distance)` | 光が届く距離。**ここでちょうど0になる**。遠いほど暗い | 800 |
| `SetDirection({x, y, z})` | スポットの向き(長さは何でもよい)。**長さ0を渡すと、オーナーの水平の正面(`Math::Forward3D`)に戻る** | 指定なし |
| `SetSpotAngle(inner, outer)` | スポットの内側(全開)と外側(消える)の角度(度)。内側 ≦ 外側 | 20 / 35 |
| `SetCastShadow(bool)` | `true` にすると、このスポットの光が影を作る(スポットだけ。`Shadow` が有効な時に意味がある。同時に影を作れるのは、カメラに近い1つ。§32) | `false` |

- 位置は**オーナーの位置**(親子関係も反映)です。スポットの向きを `rotate.y` に合わせたいだけなら、`SetDirection` を呼ばなくて構いません(水平の正面になります)。下向きなど水平でない向きは、`SetDirection` で渡します。
- `Material::lit = false`(`SetUnlit(true)`)の物は、光の影響を受けません(スカイボックス、`Lantern` の球など)。
- **光沢(`specularStrength`)はポイント/スポットにも効きます**(光の色×強さ×減衰を掛けた、同じブリン・フォンの式)。自発光は今まで通り、光の後に足すだけです。
- **2D(Sample2D)では使えません。** スプライト(`Shader.hlsl`)は光の計算をしないためです(3Dメッシュ=`MeshRenderer` / `ModelRenderer` だけに効きます)。Sample3D の敵・弾のスプライトも同じで、光は当たりません。

### 光の式

光が当たる強さ(光ごとに計算して、平行光源と環境光に**足します**):

```
減衰   atten = saturate(1 - (距離 / range)^2)^2          距離が range 以上なら 0
スポット  s = (cosθ - cos外側) / (cos内側 - cos外側) を0〜1に収め、 s*s*(3 - 2*s) を atten に掛ける
         (cosθ = 光の向きと「光からピクセルへの向き」の内積。内側は全開、外側で0、間は滑らかにつながる)
明るさ  色 × 強さ × atten × saturate(dot(法線, ピクセルから光への向き))
```

- 物理的な `1/距離^2` ではなく、「`range` でちょうど0になる」式にしています。`range` の外では計算が要らず、光が遠くまでいつまでも残りません。
- **ピクセルごと**に計算します(面が大きくても、光の近くが正しく明るくなります)。
- **法線の注意**: 今のシェーダーは法線に `world` をそのまま掛けています。`scale` が均等(`{100,100,100}`)なら問題ありませんが、**不均等に伸ばした球などは、光の当たり方が少し不自然になる**ことがあります(立方体は問題なし)。気になったら別途相談してください。

### 最大数(8個)

同時に使える光は **8個**(`Lighting::maxLights`)です。**9個以上ある時は、3Dカメラに近い8個だけ**が光り、残りは光りません。1フレームに1回だけ選ぶので、オブジェクトごとに近い光を選ぶわけではありません(遠くの大きなオブジェクトが、近くの光を無視することがあります)。超えている間は、出力ウィンドウに「光の数が上限(8個)を超えました」と**1回だけ**出ます。たくさん置きたい時は、画面の外の光を `SetActive(false)` で消してください。

### 環境光について

光の効果は、周りが明るいと目立ちません。ライトを置いたシーンで効果を強く見せたい時は、**使う側のシーンで**環境光を暗くしてください(Engine側では暗くしません)。

```cpp
Image::GetLight().ambient = { 0.1f, 0.1f, 0.12f };   // 暗くする(今は 0.25)
```

### 仕組み(Engineを触る人向け)

- 光は `Lighting`(`Engine/Render3D/Lighting.h/.cpp`)が集めます。`LightComponent` の `Init` / `Uninit` / デストラクタで登録・解除し、`GameEngine` が毎フレーム `Lighting::Update()`(`Visibility::BeginFrame()` の後)を呼んで、アクティブな光をカメラに近い順に8個選び、定数バッファ **`b4`** に書きます。`Mesh::Draw` / `SkinMesh::Draw` は `Lighting::Bind()` で `b4` をセットするだけです。
- シェーダー側は `Shader3DCommon.hlsli` の `LightBuffer`(`int4 lightCounts` + `GpuLight lights[8]`)と、`ShadePixel()` の中のループです(`Shader3D.hlsl` と `Shader3DSkin.hlsl` が同じ結果になります)。C++の `Lighting::LightConstantBuffer`(528バイト)と並びが一致するよう `static_assert` を入れています。光が0個なら、ループは1回も回りません。
- レジスタは `b3`(カメラ位置。描く度)、`b4`(光。1フレームに1回)です。影(`docs/EngineRoadmap2.md` の 6.5。§32)は `b5` を使い、`GpuLight` の `innerPad.y` が「影を引く光(1つだけ)」の印です(`Lighting` が、`castShadow` のスポットから1つ選んで印を付けます)。

### Sampleでの使われ方

- **Sample3D の `Lantern`**(`Objects/Lantern.cpp`、`Components/LanternController.cpp`): オレンジ色のポイントライトを持つ、光る小さな球(`SetUnlit(true)`)。原点の周りを半径650でゆっくり回り、sinで上下に揺れます。近くの地面・壁・箱がその色に照らされます。
- **Sample3D の `Guard`**(`Objects/Guard.cpp`、`Components/GuardController.cpp`): 視線の向きに、スポットライト(内側15度・外側30度・届く距離1500)の懐中電灯を持っています。`GuardController` が毎フレーム、視野の正面を12度下へ傾けた向きを `SetDirection` に渡すので、回ると地面の光の輪が一緒に動きます。
- Sample3D の環境光は**暗くしていません**(今までの見た目のまま)。光が当たる範囲だけ少し明るくなります。
- Sample2D では使っていません(2Dには光が効きません)。

---

## 32. 影(Shadow)

3Dのメッシュ(`MeshRenderer` / `ModelRenderer`)に、**地面や他の物へ落ちる影**を付けられます(`docs/EngineRoadmap2.md` の 6.5)。太陽(平行光源。`Image::GetLight()`)の影と、懐中電灯(スポットライト)の影が作れます。**影を有効にしない(`Shadow` を使わない・`castShadow` / `receiveShadow` を指定しない)限り、見た目は今までと1ピクセルも変わりません**。

### 使い方

3つのことをします。

```cpp
// (1) 影を有効にする(普通は、シーンの最初に1回)
Shadow::GetSettings().enabled = true;
Shadow::GetSettings().distance = 2500.f;   // カメラの前方、この大きさの範囲に影を作る(省略すると 3000)

// (2) 影を落としたい物に、castShadow を付ける
auto* box = obj->AddComponent<MeshRenderer>(Mesh::CreateCube());
box->GetMaterial().castShadow = true;      // この物は影を落とす
box->GetMaterial().receiveShadow = true;   // この物は影を受ける(自分や他の物の影が落ちる)

// (3) 影を受けたい物(地面など)に、receiveShadow を付ける
ground->GetMaterial().receiveShadow = true;
```

- **`castShadow` と `receiveShadow` は、どちらも既定が `false` です**(`Material` の項目。§30)。`MeshRenderer` と `ModelRenderer`(骨で曲げるFBXも含む)で使えます。`Mesh::Draw` を直接呼ぶ物(`GroundController` の地面のタイルなど)は、`Material` を作って `receiveShadow = true` を入れて渡します。
- 影の設定は**全シーン共通**です。そのシーンの間だけ使うなら、終わる時に戻してください(`Shadow::SetSettings(Shadow::Settings())`)。Sample3D は `ShadowManager`(下の「Sampleでの使われ方」)が `Init` で有効にし、`Uninit` で戻しています。
- Sceneには「何を置くか」だけを書く決まりなので、影の設定は、管理役のオブジェクト(Sample3D の `CreateSun`)に付けた Component に書いています。

### 設定項目と既定値(`Shadow::Settings`)

| 項目 | 意味 | 既定 |
|---|---|---|
| `enabled` | 影を使うか。**`false` の間は影の処理を一切しません(コスト0)** | `false` |
| `resolution` | 影のテクスチャの大きさ(正方形)。1024 / 2048 / 4096。大きいほどくっきりして、メモリも使う(2048=16MB、4096=64MB) | 2048 |
| `distance` | カメラの前方、この大きさの範囲に影を作る。**広いほど影がぼやける** | 3000(30m) |
| `depthBias` | 影の判定のゆとり。小さすぎると縞模様(シャドウアクネ)、大きすぎると影が物から離れて浮く | 0.0005 |
| `normalBias` | 面の法線の方向へずらして判定する量(ワールド単位)。縞模様が消えない時に増やす | 2.0 |
| `strength` | 影の濃さ。0=影なし、1=平行光源の光が真っ暗(環境光は残る) | 0.6 |
| `pcfRadius` | 影の縁のぼかし。0=ぼかさない、1=3x3、2=5x5 | 1 |
| `spotResolution` | スポットライトの影のテクスチャの大きさ(正方形) | 1024 |
| `spotDepthBias` | スポットの影の判定のゆとり(ワールド単位) | 3.0 |
| `spotStrength` | スポットの影の濃さ。0=影なし、1=そのスポットの光が真っ暗 | 1.0 |

`SetSettings(settings)` でまとめて渡すこともできます。値は、使える範囲に収められます(`resolution` は256〜8192、`pcfRadius` は0〜4など)。

### 影が出る光と、出ない光

- **影が出る光は、平行光源(太陽)と、`SetCastShadow(true)` にしたスポットライト1つだけ**です。**ポイントライト(点光源)の影は作れません**(`SetCastShadow` を呼んでも無視されます)。
- 影は、その光だけを暗くします。**環境光は暗くなりません**(影の中でも、環境光ぶんの明るさが残る。環境光が明るいと、影が薄く見えます)。ポイントライトの光も、太陽の影の中で暗くなりません。
- `Material::lit = false`(`SetUnlit(true)`)の物は、影を受けません(スカイボックスなど)。
- 2D(`SpriteRenderer`)の物は、影を**落とせません**(画像の板の影は不自然なため)。Sample3D の敵・弾は、影を落としません(足元の丸い影などは、まだありません)。Sample2D は、影を使いません。

### スポットライトの影

```cpp
auto* torch = obj->AddComponent<LightComponent>(LightType::Spot);
torch->SetCastShadow(true);   // この懐中電灯の光は、壁などでさえぎられる(影になる)
```

- 同時に影を作れるスポットは**1つ**です。`SetCastShadow(true)` の光が複数ある時は、**3Dカメラに一番近い1つ**だけが影を作ります(残りは、影なしで光ります)。
- 影の範囲は、スポットの円すい(外側の角度)です。**外側の角度が60度より大きい部分には影が出ません**(影のテクスチャの視野が最大120度のため)。
- 影を落とす物(`castShadow = true`)と、影を受ける物(`receiveShadow = true`)は、太陽の影と同じです。

### 調整のしかた

実機で見て、次のように直してください(見え方はグラフィックボードや解像度で変わります)。

| 見え方 | 直し方 |
|---|---|
| 影が縞模様(つぶつぶ)になる | `depthBias` を少し増やす(0.0005 → 0.001)。まだ出るなら `normalBias` を増やす(2 → 4) |
| 影が物から離れて浮いて見える | `depthBias` と `normalBias` を減らす |
| 影の縁がギザギザ・ぼやける | `resolution` を上げる(2048 → 4096)か、`distance` を小さくする(範囲が狭いほどくっきり)。`pcfRadius` で柔らかさを変える |
| 遠くの影が出ない | `distance` を大きくする(その分ぼやける) |
| 影が濃すぎる・薄すぎる | `strength` を変える |
| 重い | `resolution` を下げる。影を落とす物(`castShadow`)を減らす |

### 範囲と決まり

- 影の範囲は「3Dカメラの前方 `distance / 2` の点を中心に、一辺 `distance` の正方形(光に垂直な面)×奥行き `2 × distance`(光の向きに、中心の前後 `distance` ずつ)」の箱です。**範囲の外では影が出ません**(明るいまま)。範囲の縁(4%)は影が弱くなって、急に消えて見えないようになっています。
- カメラが動いても、影の縁がちらつかないよう、影の範囲は1テクセル(`distance / resolution`)ずつ動きます。
- カメラが高い所から真下を見る時は、床が奥行きの範囲(中心から光の向きに `distance` まで)の外に出て、影が出なくなります。その時は `distance` を大きくしてください。
- 画像の透明な部分(`png` のアルファ)も、**板全体が影になります**(切り抜きの影にはなりません)。半透明の物(`GlassPanel` など)は `castShadow` を付けないのがおすすめです。
- 影を落とす物は、**アクティブ(`SetActive(true)`)で有効(`SetEnabled(true)`)な物だけ**です。`ObjectPool` で休んでいる物は影を落としません。`Destroy` やシーンの切り替え(`Uninit`)で、登録は自動で外れます。
- カスケード(遠近で影の細かさを変える)・ポイントライトの影・柔らかい影(PCSS)は、ありません(ポイントライトの影は、作らないことにしました。設計だけ `docs/EngineRoadmap2.md` の 6.10 に残してあります)。

### 性能

- 影が有効な間は、毎フレーム、`castShadow` の物の数だけ描画命令が増えます(太陽の影のパス1回。スポットが影を作る時はもう1回)。Sampleの規模なら問題ありません。
- 影のテクスチャは `resolution` × `resolution` × 4バイト(2048で16MB)です。`enabled = true` にした最初のフレームで作られ、解像度を変えると作り直されます。`enabled = false` に戻すと(と終了時に)解放されます。スポットの影のテクスチャも、影を作るスポットが無い間は持ちません。
- 影が無効(`enabled = false`)の間は、影のパス・影のテクスチャの作成はしません。

### 仕組み(Engineを触る人向け)

- **登録簿方式**です。`MeshRenderer` / `ModelRenderer` が `Init` で `Shadow::Register` に自分を登録し(`Uninit` とデストラクタで解除)、`GameEngine::Run` が毎フレーム、`Lighting::Update()` の次(`Text::BeginDraw()` の前)に `Shadow::Render()` を呼びます。`Component` / `GameObject` / `Scene` には手を入れていません。
- `Shadow::Render()` は、①今の描画先・深度・ビューポート・ラスタライザ・ブレンド・深度ステンシル・ピクセルシェーダーを覚える ②影のテクスチャ(`D32_FLOAT` の深度。読む時は `R32_FLOAT`)を描画先にして、登録された物の `DrawShadow()` を呼ぶ(`Mesh::DrawDepth` / `SkinMesh::DrawDepth`。ピクセルシェーダーは無く、深度だけ) ③**必ず元へ戻す**、の順です。影のテクスチャを描く前に、読む側のバインドを外します(同じ物を同時に読み書きしない)。
- 本番の描画では、`Mesh::Draw` / `SkinMesh::Draw` が `Shadow::Bind()` で、`b5`(`ShadowBuffer`)・`t1`(太陽の影)・`t2`(スポットの影)・`s1`(比較サンプラー)をセットします。`Shader3DCommon.hlsli` の `ShadePixel()` が、影を受ける物(`receiveShadow`)だけ、光から見た位置の深度と比べて、太陽(とスポット)の光を暗くします。影が無効の時は `nullptr` をセットするので、シェーダーは0として読み、影なしになります。
- 影のにじみ(アクネ)は、①影のパスのラスタライザの傾き補正 ②本番で引く `depthBias` ③法線方向へずらす `normalBias` の3つで抑えています。縁のぼかし(PCF)は、比較サンプラー(`SampleCmpLevelZero`)で行います。
- スキンメッシュ(骨で曲げるFBX)は、`Shader3DSkin.hlsl` の `VSShadow` が、骨で曲げた後の位置を深度に書きます。
- レジスタの割り当て(§30)は、`b5` = 影、`t1` = 太陽の影、`t2` = スポットの影、`s1` = 比較サンプラーです。

### Sampleでの使われ方

- **Sample3D の `Sun`**(`Objects/Sun.cpp`、`Components/ShadowManager.cpp`): 見た目の無い管理役です。`ShadowManager` が、`Init` で影を有効にして `distance` を2500にし、`Uninit` で設定を既定に戻します。`GameScene` には `CreateSun(*this)` を置くだけです。
- 影を受ける物: 地面のタイル(`GroundController` が、`Material` の `receiveShadow = true` を渡して描く)。影を落とし、受ける物: `Player`・`Wall`・`ModelDisplay`・`SkinDisplay`(`Guard` は受けるだけ。懐中電灯の光源が自分の立方体の内側にあるので、落とすと自分の面が光をさえぎってしまう。`castShadow = true` と `receiveShadow = true`。`SkinDisplay` は骨で曲げた形の影が落ちます)。
- **`Guard` の懐中電灯**は `SetCastShadow(true)` で、`Wall` や `Player` の後ろに光が届かなくなります(影を落とすスポットは、カメラに近い1つだけ)。
- **影を有効にしたので、Sample3D の見た目は、地面に影が落ちること・懐中電灯の光が壁などでさえぎられること、の2点が変わります。** `GlassPanel`・`Lantern`・`Skybox`・敵・弾は影の対象にしていません。Sample2D は影を使いません(見た目は変わりません)。
- 実機で見る時は、太陽の向き(`Image::GetLight().direction` の既定は `(0, -1, 0.5)`)の通り、影は奥(+Z)の向きに伸びます。縞模様や、影が物から浮く時は、上の「調整のしかた」で直してください。

### まだ無いもの

- 敵(画像)の足元の丸い影(ブロブシャドウ)・ポイントライトの影・カスケード・柔らかい影(PCSS)
- 画像の透明部分を切り抜いた影

---

## 33. 枠線とリム発光(SetOutline / SetRimLight)

物の縁を、色付きで光らせられます(`docs/EngineRoadmap2.md` の 6.6)。選んだ物・取れる物・敵を目立たせるのに使えます。2種類あります。

| 名前 | 見え方 | 向いている物 |
|---|---|---|
| **枠線(アウトライン)** | 物の**外側**に、はっきりした色付きの線が出る | 「これを選んでいる」「近づくと取れる」「見つかった」の合図 |
| **リム発光(縁の光)** | 物の**内側**の、視線に斜めになった縁がぼうっと光る | 魔法の光・ホログラム・主人公をきわだたせる |

**どちらも既定はオフです。指定しない限り、見た目は今までと1ピクセルも変わりません**(Sample2D のデバッグ表示も同じです)。`MeshRenderer` と `ModelRenderer`(FBXモデル。骨で曲げるモデルも)で使えます。

### 使い方

```cpp
// 縁の光: 色・強さ・(省略可)縁への集まり方
meshRenderer->SetRimLight({ 0.3f, 0.7f, 1.0f }, 1.0f);       // 縁が青く光る。強さ0で消える
meshRenderer->SetRimLight({ 0.3f, 0.7f, 1.0f }, 1.0f, 5.0f); // 5.0にすると、縁の細い線だけが光る

// 枠線: 色と太さ(画面のピクセル)
meshRenderer->SetOutline({ 1.0f, 0.9f, 0.2f, 1.0f }, 3.0f);  // 太さ3ピクセルの黄色い枠線
meshRenderer->SetOutline({ 1.0f, 0.9f, 0.2f, 1.0f }, 0.0f);  // 太さ0で消える

// 壁越し(seeThrough): 手前に壁があって隠れていても、枠線だけは見える
obj->SetDrawPriority(5);                                          // 壁などより後に描く(下の注意)
meshRenderer->SetOutline({ 1.0f, 0.9f, 0.2f, 1.0f }, 3.0f, true);

// Materialの項目を直接書いても同じ(SetRimLight / SetOutline は、これを書き換えるだけ)
meshRenderer->GetMaterial().outlineWidth = 3.0f;
```

「近づくと光る」のように、枠線の太さを毎フレーム切り替える使い方もできます(`outlineWidth` を 0 と 3 で切り替えるだけ)。Sample3D の `HighlightController` が例です。

### 項目と既定値

| 項目(`Material`) | 意味 | 既定 |
|---|---|---|
| `rimColor` | 縁の光の色 | 白 |
| `rimStrength` | 縁の光の強さ。**0=光らない**。1くらいで見える | 0 |
| `rimPower` | 縁への集まり方。大きいほど細い線だけが光る(1=広くぼんやり、6=細くくっきり) | 3 |
| `outlineColor` | 枠線の色。**aは濃さ**(1=不透明。小さいと背景と混ざる) | `{1, 1, 0, 1}`(黄) |
| `outlineWidth` | 枠線の太さ。**画面のピクセル**。**0=枠線なし** | 0 |

### 壁越しに見える枠線(seeThrough)

`SetOutline` の3つ目の引数を `true` にすると、**手前に壁などがあって本物が隠れていても、枠線が見えます**(敵や取れる物の位置を、壁越しに教える使い方)。**既定は `false`** で、指定しない物は今まで通りです(描画の回数も増えません)。`Material` の項目は `outlineSeeThrough` です。

- 枠線は、**本物のシルエット(画面に映る形全体)の外側の輪**だけを、深度テストなしで描きます。シルエットの内側(本物が壁に隠れている部分)には、枠線は出ません(壁はそのまま見えます)。
- **描画順に注意**: 枠線は、その物を描く時に描かれるので、**後から描かれる物(壁など)が上書きします**。壁越しに見せたい物は、**`SetDrawPriority` を壁より大きくして**(例: 5。壁は既定の0)、壁より後に描かれるようにしてください(描画順は `drawPriority` の小さい順 → `z` の小さい順です)。
- 半透明・`depthWrite = false` の物には、今まで通り枠線を描きません。影のパスには出ません。スキンメッシュでも同じです。
- 同じ色のまま出ます(隠れている間だけ色や濃さを変えることは、まだできません)。

### 太さの単位(画面のピクセル)

枠線の太さは**ワールドの長さではなく、画面上のピクセル数**です。**物がカメラから遠くても近くても、同じ太さに見えます**(ワールド単位だと、遠くの物の枠線は細くなって消えてしまうため)。画面の解像度が違うと、画面に対する太さの割合は変わります(`3` は、1280x720 でも 1920x1080 でも3ピクセル)。

### 枠線の仕組みと見え方

- 本物を描いた**直後**に、同じ物を「画面上で `outlineWidth` ピクセルだけ外へ膨らませて」もう一度描き、**裏向きの面だけ**を枠線の色で残します。本物は先に描いてあるので、膨らんだ分(外側の輪)だけが線として見えます。
- 膨らませる向きは、**同じ位置にある頂点の法線を平均した向き**です(立方体のように面ごとに法線が違う形でも、角で線が割れません)。この平均した法線は、`Mesh` / `SkinMesh` が作る時に1回だけ作って持ちます(1頂点あたり12バイト増えます)。
- 枠線は**深度に書きます**(本物の裏側の深さを、太さに応じて少しだけカメラ側へずらした値。ずらす量は「太さ(ピクセル)×3 を、その位置のワールドの長さに直したもの」)。ずらすのは、**地面に立つ物の接地部**(膨らんだ殻が地面と同じ高さか下に入る所)の枠線が地面に隠れて切れないようにするためです。**本物の内側には出ません**(本物の見えている画素にはステンシルで描かない)。**手前にある別の物には隠されます**(ずらした量より大きく手前にある壁など。それより近い物は、枠線が少し突き抜けて見えることがあります)。後ろにある物の上には出ます。
- **影・光とは無関係**です。枠線は影を落とさず、影も受けず、平行光源・ポイントライト・スポットライトの光でも色が変わりません(いつも `outlineColor` のままです)。
- **スキンメッシュ(骨で曲げるFBXモデル)にも出ます**。骨で曲げた後の形の外側に出て、骨の動きに付いてきます。`ModelRenderer` は、パーツごとに「パーツを描いた直後」に枠線を描きます。

### 縁の光(リム発光)の式

`光の強さ = (1 - |法線と視線の内積|) の rimPower 乗 × rimStrength` を、`rimColor` に掛けて、最後に足します。視線に垂直な面(縁)ほど明るく、真正面を向いた面は光りません。ライトの光や影には関係なく足します(自発光に近い扱い)。`lit = false`(スカイボックスなど)の物にも付きます。

- 球・カプセルのように丸い形では、輪郭に沿ってきれいに光ります。**立方体のような平らな面は、面全体が同じ明るさ**で光ります(斜めに見える面ほど明るくなる)。

### 制限・気を付けること

- **角ばった形の枠線は、辺の所が角より細くなります**(立方体で、辺は指定した太さの約7割)。丸い形は指定どおりの太さです。
- **枠線が突き抜けることがあります**: 手前の壁が、本物より「太さ×3ピクセルぶんのワールドの長さ」より近いと、壁の向こうの枠線が壁の上に少し見えることがあります(壁が本物の十分手前にあれば隠れます)。
- **厚みの無い1枚の板**(表だけの面)には、枠線が出ません(裏向きの面が無いため)。立方体や球のように閉じた形か、厚みのある板で使ってください。
- **`depthWrite = false` の物**(半透明の板・スカイボックス)には、枠線を描きません(本物が深度に書かれないと、膨らませた殻が内側まで枠線の色になってしまうため)。半透明の物を目立たせたい時は、リム発光を使ってください。
- **画像の透明な部分(`png` のアルファ)は考えません**。透明を切り抜いた形でも、枠線は元の板(立方体など)の形の外側に出ます。
- アルファで抜く画像(透明部分のある画像)のメッシュは、透明部分でも深度とステンシルが書かれるため、透明部分には枠線が出ません(抜き絵の輪郭の枠線は付きません)。
- `outlineColor.a` を1より小さくすると、ごく一部の画素(輪が重なる所)が少し濃くなることがあります。普通は1のままにしてください。
- 線で描くメッシュ(当たり判定のデバッグ表示など)には、枠線は付きません。
- `SetFrustumCulling(true)` の判定は、枠線の太さを考えません(画面の端で、枠線だけ先に消えることがあります)。
- **「光る」のにじみ(ブルーム)はありません**。枠線は、画面の外へにじまない、くっきりした線です(画面全体をぼかして足すブルームは、別の仕組みなので作っていません)。
- 敵(画像のスプライト)には付きません(枠線は `Shader3D.hlsl` の物だけです)。

### 性能

- 枠線を付けた物は、描画命令が**1体につき3回**増えます(本物の印・枠線・印を消す。`ModelRenderer` はパーツごと)。枠線の太さが0の物には、何も増えません。
- 平均した法線のバッファは、三角形のメッシュを作る時に1回だけ作ります(頂点数に比例。`Init` 時に少し時間がかかります)。
- リムは、ピクセルシェーダーに数行足しただけです。`rimStrength = 0` の物は、その計算を飛ばします。

### 仕組み(Engineを触る人向け)

- **ファイル**: 新しいファイルはありません。`Shader3DCommon.hlsli`(`MaterialBuffer` に `matRim` / `matOutline` / `matOutlineParams` を追加。`ShadePixel()` の最後にリム。`OutlineClipPos()` と `ShadeOutline()` を追加)、`Shader3D.hlsl` / `Shader3DSkin.hlsl`(`VSOutline` / `PSOutline`)、`Mesh.h/.cpp` / `SkinMesh.h/.cpp`(`DrawOutline`)、`MeshRenderer.h` / `ModelRenderer.h/.cpp`、`Material.h` を変えました。
- **`b2` の大きさ**: `MaterialConstantBuffer` は `float4` が7本(112バイト)になりました(`color` / `specular` / `emissive` / `params` / `rim` / `outline` / `outlineParams`)。C++側に `static_assert(sizeof == 16 * 7)` があり、HLSLの `MaterialBuffer` と並びを合わせています。`params.y` は `rimPower` です。
- **`Mesh::DrawOutline(world, camera, color, widthPixels)`**(`SkinMesh::DrawOutline` も同じ形で、骨の行列を受ける): 頂点バッファの slot 0 に今まで通りの頂点、slot 1 に平均した法線を渡し、枠線専用の頂点シェーダー・ピクセルシェーダー・入力レイアウトで描きます。描画ステートは普通の物と同じ(アルファブレンド・深度テストと書き込みあり・カリングなし)で、描いた後は slot 1 を外してラスタライザを既定に戻します。
- **画面上の太さ**: `OutlineClipPos()` が、クリップ空間の位置に、法線が画面上で動く向きへ `太さ × 2 / 画面の大きさ × w` だけ足します(`w` を掛けるので、遠くても同じピクセル数になります)。画面の大きさは `Graphics::width` / `Graphics::height` です。深度は、view空間の距離を `太さ × 1ピクセルのワールドの長さ × 3` だけ手前にした値に直します(xy は動かしません)。
- **本物の内側に出さない(ステンシル)**: `DrawOutline` は3回に分けて描きます。①本物と同じ深度を通った画素(=本物が見えている画素)のステンシルを1にする(深度だけの頂点シェーダー。色も深度も書かない) ②ステンシルが1でない画素にだけ、枠線を描く ③ステンシルを0に戻す。ステンシルは毎フレームはクリアしませんが、③で戻すので他の描画には影響しません。**枠線を付けた物は、描画命令が1体につき3回増えます**。
- **裏向きの面だけを残す**: ポリゴンの巻き方向(表裏)はメッシュによって違うので、カリングは使わず、`ShadeOutline()` が「元の法線とカメラへの向きの内積が正なら `discard`」で、手前向きの面を捨てます。

### Sampleでの使われ方

- **Sample3D の `Player`**: `SetRimLight({0.3, 0.7, 1.0}, 1.0)` で、縁が青く光ります(斜めに見える面が少し青くなります)。
- **Sample3D の `Guard`**: 相手(Player)を見つけて赤くなった時は太さ4ピクセルの赤い枠線、見つけていない時は太さ2ピクセルの青い枠線が出ます(`GuardController`)。**壁越し(seeThrough)**なので、`Wall` の向こうにいる `Guard` の場所が、壁越しに分かります。`Guard` は `SetDrawPriority(5)` で壁より後に描いています(`Guard.cpp`)。
- **Sample3D の `ModelDisplay`(黄色)と `SkinDisplay`(緑。骨で曲げたモデル)**: Playerが半径700以内に近づくと、太さ3ピクセルの枠線が光り、離れると消えます(`HighlightController`。`Physics::OverlapSphere` で近くの "Player" を調べるだけで、Playerは何も知りません)。
- Sample2D は使いません。

### まだ無いもの

- ブルーム(画面全体をぼかして足す、にじむ光)
- 敵(画像のスプライト)の枠線
- 平らな1枚の板の枠線(ステンシル方式)・加算合成で光らせる枠線

---

## 34. 地形(Terrain / TerrainRenderer)

高さの画像(白=高い、黒=低い)から、起伏のある地面を作れます(`docs/EngineRoadmap2.md` の 6.7)。今の `GroundController` は平らなタイルを並べているだけですが、地形を使うと丘や谷のある地面が作れます。

見た目は `TerrainRenderer`、**物理の当たり判定(PhysX)は `TerrainRigidbodyComponent`**(`docs/EngineRoadmap2.md` の 6.8。下の「地形の当たり判定」)が担当します。同じ `Terrain` を2つに渡すので、見た目と当たり判定は同じ高さのデータから作られ、ずれません。**その場所の地形の高さを調べる `GetHeightAt`** も使えるので、敵や物を地形の上に置くこともできます。

### 使い方

```cpp
// 1. 設定を決めて、高さの画像から地形のデータを作る(読めなければ nullptr)
TerrainSettings settings;
settings.width = 4000.0f;        // X方向の大きさ
settings.depth = 4000.0f;        // Z方向の大きさ
settings.maxHeight = 600.0f;     // 画像が真っ白の所の高さ(黒は0)
settings.cellsX = 128;           // X方向のマスの数(細かいほどなめらかで重い)
settings.cellsZ = 128;
settings.uvRepeat = 4.0f;        // 画像を地形全体に何回くり返して貼るか
settings.texture = L"Assets/bg_tile.png";   // 地形に貼る画像(空なら白)
auto terrain = Terrain::LoadFromImage(L"Assets/Terrain/heightmap.png", settings);

// 2. オブジェクトに TerrainRenderer を付ける(オブジェクトの位置が、地形の「左手前の角」)
GameObject* hills = scene.CreateObject("Hills");
hills->transform.position = { 1500.0f, -50.0f, -2000.0f };
auto* renderer = hills->AddComponent<TerrainRenderer>(terrain);
renderer->GetMaterial().receiveShadow = true;   // 影を受ける(§32)
hills->AddComponent<TerrainRigidbodyComponent>(terrain);   // 当たり判定(物が乗れる。下の「地形の当たり判定」)

// 3. その場所の地面の高さ(ワールド座標)を調べる
float groundY = renderer->GetHeightAt(enemyX, enemyZ);
enemy->transform.position.y = groundY + 50.0f;
```

`Sample3D/Game/Objects/Hills.cpp` が実例です。

### 高さの画像の作り方

- **画像の色**: 白=高い、黒=低い。グレースケール画像がおすすめです。カラー画像の時は、**赤(R)の値だけ**を高さに使います。形式は PNG・BMP・JPEG など(Windows が読める画像)。
- **向き**: **画像の左が X=0、画像の上が Z=最大**(上から地図を見た時と同じ向き)です。画像の左下が地形の「左手前の角」になります。
- **精度**: 普通の画像(8ビット)は 256 段階です。`maxHeight` が大きいと段々になりますが、見た目は滑らかに補間されます(法線を高さの差から計算するので、段差は目立ちません)。**もっとなめらかにしたい時は、16ビットのグレースケールPNG**(65536段階)を使えます(サンプルの `heightmap.png` は16ビットです)。
- **大きさ**: 画像の大きさと `cellsX` / `cellsZ` は同じでなくても使えます(画像を引き伸ばして読みます)。ただし、**画像より細かい分割にしても、細かさは増えません**。ちょうど「画像の幅 = `cellsX` + 1」にすると、頂点が画素の位置にぴったり合います(サンプルは 129 x 129 で `cellsX = 128`)。
- 画像は GIMP などで描いたり、ゲームに使う地形の高さを塗って作ったりできます。サンプルの `Assets/Terrain/heightmap.png` は、数式で作った小さな画像(129 x 129、16ビット)です。**外周の25%の幅(約1000)は、smoothstep で端に向かって下がり、端の高さは周りの地面より1.5だけ高い(ほぼ0)スロープ**にしてあるので、地面から歩いて登れ、地面とチラつきません(最大の傾きは約33度)。

### 表(CSV)から作る

画像の代わりに、表(CSV。§25)からも作れます。

```
x0,x1,x2,x3
0,0.2,0.4,0.2
0.1,0.5,1.0,0.4
0,0.2,0.4,0.1
```

```cpp
auto terrain = Terrain::LoadFromCsv(L"Assets/Data/Heights.csv", settings);
```

- **1行目は `x0,x1,x2,...`**(列の名前。必ずこの名前にします)。2行目からが高さのデータで、**1マスに0〜1の数を1つ**書きます(範囲外は丸めます。1が `maxHeight`)。
- **最初のデータ行が Z=最大**(画像の上と同じ向き)、列 `x0` が X=0 です。
- 大きさは 2列2行以上。分割数が表と違っても、画像と同じように引き伸ばして読みます。

### プログラムで作る

```cpp
std::vector<float> samples = { 0.f, 0.5f, 0.5f, 1.f };   // 0〜1の数。横2 x 縦2(最初の行が Z=最大)
auto terrain = Terrain::CreateFromSamples(samples, 2, 2, settings);
```

### 項目(`TerrainSettings`)と既定値

| 項目 | 意味 | 既定 |
|---|---|---|
| `width` | X方向の大きさ | 10000 |
| `depth` | Z方向の大きさ | 10000 |
| `maxHeight` | 高さのデータが1(真っ白)の所の高さ。黒は0 | 1000 |
| `cellsX` / `cellsZ` | マスの数(頂点は +1 個)。**1〜256**(範囲外は丸めて、出力ウィンドウに知らせる) | 128 |
| `uvRepeat` | 貼る画像を、地形全体に何回くり返すか(1マスごとではなく地形全体で) | 20 |
| `texture` | 貼る画像のパス(空なら白)。`TerrainRenderer` の最初のマテリアルに入る | 空 |

### 地形の座標と、高さの調べ方

- **地形の座標**: オブジェクトの位置が、地形の**左手前の角**(X・Zが最小の角)で、**Yがそのまま高さ0**です。地形の中心ではないので注意してください(中心を原点にしたい時は、`position = { -width/2, 高さ, -depth/2 }`)。
- `renderer->GetHeightAt(worldX, worldZ)`: その場所の**地面のワールドのY**(オブジェクトのYを足した値)。地形の範囲の外は、一番近い端の高さを返します。範囲内かは `renderer->Contains(worldX, worldZ)` で調べられます。
- `renderer->GetNormalAt(worldX, worldZ)`: その場所の**面の向き**(長さ1)。真上なら `(0,1,0)`、坂道は傾きます。坂が急かどうかの判定に使えます。
- 地形のデータ自体(`Terrain`)にも同じ関数があります(`terrain->GetHeightAt(localX, localZ)`)。こちらは「角を原点にした座標」で、ワールドへの変換はしません。`GetHeights()`(頂点の高さの配列)・`GetVertexCountX/Z()`・`GetCellSizeX/Z()` で、元のデータを取り出せます(`TerrainRigidbodyComponent` が同じデータを使います)。

### 描画・UV・法線

- **メッシュ**: `(cellsX+1) x (cellsZ+1)` 個の頂点を並べ、1マスを2枚の三角形にします。頂点とインデックス(32ビット)は**作る時に1回だけ送り**(変更不可)、描く時は1回の `DrawIndexed` です。最大の 256 x 256 マスでも、頂点は66049個で軽く描けます。
- **三角形の分け方**: 全マスで、手前左 `(x, z)` から奥右 `(x+1, z+1)` への対角線で分けます。**`GetHeightAt` も同じ三角形の上で計算する**ので、地形の上に置いた物が浮いたり埋まったりしません。
- **UV**: `u = X座標 / width × uvRepeat`、`v = Z座標 / depth × uvRepeat`。画像は地形全体で `uvRepeat` 回くり返します(サンプラーは `WRAP`)。
- **法線**: 隣の頂点との高さの差(中心差分)から計算するので、丘がなめらかに光ります(8ビットの画像でも段が目立ちにくい)。`GetNormalAt` は三角形1枚ごとの面の向きなので、描画の法線とは少し違います(物理の当たり判定と同じ面です)。

### ライト・影・マテリアルとの関係

`TerrainRenderer` は `MeshRenderer` と同じ `Material`(§30)を持ち、同じ描き方(同じ `Mesh::Draw`)で描くので、**ライトは同じように効きます**。

- **色・光沢・自発光**: `renderer->GetMaterial().color` などで変えられます。画像は `GetMaterial().texture` で差し替えられます。
- **平行光源・ポイントライト・スポットライト**(§31): 地形にも当たります。
- **影**(§32): `GetMaterial().receiveShadow = true` で、他の物の影を受けます。`castShadow = true` にすると地形も影を落とします(地形全体を影の描画に含めるため重くなります。山が大きな影を落としたい時だけ)。**どちらも既定は `false`** です。
- **縁の光(リム)**: `Material` の `rimStrength` で付けられます。**枠線(`SetOutline`)は付けません**(地形全体の枠線に意味が無いため)。
- **画面外の判定**(§28): `renderer->SetFrustumCulling(true)` にすると、地形全体を囲む球がカメラに映らない時は描きません。**既定は `false`** です(1枚の大きな地形は、たいてい一部が映っているため)。地形を小さなかたまり(チャンク)に分けて、映る所だけ描くことはしていません。

### 地形の当たり判定(TerrainRigidbodyComponent)

地形に `TerrainRigidbodyComponent` を付けると、**Dynamic の箱・球・カプセルが起伏の上に乗り、坂道を転がり、歩けます**。PhysX の「高さフィールド」で作る、いつも **Static**(動かない)の当たり判定です。`Physics::Raycast` / `OverlapSphere` / `OverlapBox`(§23)も地形に当たります(視線をさえぎる地形にしたい時は `SetTag("Wall")` を付けます。§28)。

```cpp
auto terrain = Terrain::LoadFromImage(L"Assets/Terrain/heightmap.png", settings);
GameObject* hills = scene.CreateObject("Hills");
hills->transform.position = { 1500.0f, -50.0f, -2000.0f };
hills->AddComponent<TerrainRenderer>(terrain);               // 見た目
hills->AddComponent<TerrainRigidbodyComponent>(terrain);     // 当たり判定(同じ terrain を渡す)
```

- **同じ `terrain` を渡す**: 見た目と当たり判定が同じ高さのデータ(`GetHeights()`)から作られるので、ずれません。
- **原点と向き**: オブジェクトの位置が地形の**左手前の角**(`TerrainRenderer` と同じ)で、Yが高さ0です。PhysX の高さフィールドは「行 = X方向、列 = Z方向」で、`Terrain::GetHeights()` の並びと同じです。1マスの対角線も `TerrainRenderer` と同じ `(x,z)→(x+1,z+1)` です(PhysX の `tessFlag` を全サンプルに立てて作ります)。
- **精度**: PhysX は高さを16ビットの整数で持つので、(いちばん高い所 ÷ 32767)の刻みに丸まります。サンプル(最大高さ600)で**約 0.015 刻み(誤差は最大でもその半分の約 0.008)**。ゲームの1単位(1cm)より十分に小さいです。
- **制限**:
  - **動きません**。作った時の位置で固定で、後から `transform.position` を動かしても当たり判定は動きません。回転・拡大縮小も使えません(`TerrainRenderer` と同じ)。
  - **実行中に高さを変えられません**(作り直しになります)。穴や洞窟(地形の下に空間がある形)は作れません。
  - **トリガーにできません**(`isTrigger = true` を渡しても、出力ウィンドウに知らせて普通の当たり判定になります)。
  - **地形の下側からは通り抜けます**(面は上向きの1枚です)。**地形の横(端)には壁が無い**ので、端が周りの地面より高いと、地面から歩き込んだ物は地形の下にもぐってしまいます。**サンプルの高さ画像は端が(ほぼ)0のスロープなので、地面から歩いて登れます。自分の画像でも、端を0(周りの地面と同じ高さ)に下げると、もぐりません。**
  - 地形の外には何もありません(落ちます)。
  - **デバッグ表示(`SetDebugDrawEnabled`)は、地形には枠線を描きません**(大きすぎて意味が無いため)。
- **歩ける傾き**: 箱の Player(`SetVelocity` で水平だけ与える)でも、サンプルの丘の傾き(最大約37度)は越えられます。ただし箱は坂の途中で角が引っかかりやすいので、急な坂を滑らかに歩かせたい時は `CapsuleRigidbodyComponent`(§29)にしてください。
- **解放**: アクターを解放してから高さフィールドを解放します(`Uninit` で自動)。シーン切り替えで確実に解放されます。

### 制限・気を付けること

- **`TerrainRenderer` だけでは当たり判定は付きません**。物を乗せたい時は `TerrainRigidbodyComponent` も付けてください(下の「地形の当たり判定」)。
- **オブジェクトの `scale` は `{1,1,1}`、回転なしのまま**にしてください(大きさは `TerrainSettings` で決めます)。拡大縮小や回転があると、`GetHeightAt` と見た目がずれ、法線もずれます。位置(と親)だけが使われます。
- **地形の外は何もありません**(落ちます)。Sample3D は、見えない大きな床(`Ground`)を残してあるので落ちません。
- **画像・CSVが読めなかった時は `nullptr`** が返ります(出力ウィンドウに理由が出ます)。`TerrainRenderer` に `nullptr` を渡すと何も描かないので、`Hills.cpp` のように `nullptr` を確かめてから使ってください。
- **画像の貼り方は1枚をくり返すだけ**です。高さで画像を混ぜる(低い所は草、高い所は岩、など)ことは、まだできません。
- 分割数は最大 256 x 256 です(頂点66049個)。それ以上の広い地形は、地形を複数に分けて並べてください。

### Sampleでの使われ方

- **Sample3D の `Hills`**(`Objects/Hills.cpp`): `Assets/Terrain/heightmap.png`(なだらかな丘と谷)から、4000 x 4000 の地形(最大高さ600、分割128)を作り、Playerの東の遠く、位置 `(1500, -50, -2000)` を角にして置いています。地面と同じ画像(`bg_tile.png`)と色で貼り、影を受けます。`TerrainRigidbodyComponent` も付けてあり、Playerが東へ歩いていくと丘が見えて、**丘の起伏の上を歩けます**(丘の端はスロープで、地面から歩いて登れます)。
- **今ある地面は残しています**。`GroundController` のタイルと見えない大きな床(`Ground`)は、そのままです(床の上面は Y=-50 で、丘の底と同じ高さ。丘の端は床の1.5上までなだらかに下がって、床につながります。丘の内側では床は丘の下に隠れるだけで、見た目も動きも変わりません。丘の外に落ちない保険にもなります)。
- Sample2D は使いません。

### 仕組み(Engineを触る人向け)

- **ファイル**: 新しく `Engine/Render3D/Terrain.h/.cpp`(高さのデータ・高さと法線の計算・メッシュの元データ作り)と `Engine/Render3D/TerrainRenderer.h/.cpp`(描く Component)を足しました。既存のEngineファイル・シェーダーは変えていません(`Mesh::InitIndexed` と `Material` と `Shadow::Caster` を、そのまま使っています)。
- **画像の読み込み**: `Texture::Load` は GPU 用のテクスチャしか作らず、画素を CPU で読めないため、`Terrain` が自分でWIC(`IWICImagingFactory`。`SpriteTransition` と同じ)を使って読みます。16ビットのグレースケール(`GUID_WICPixelFormat16bppGray`)はそのまま、他の形式は `GUID_WICPixelFormat32bppRGBA` に変換して赤(R)を使います。
- **高さの配列**: `GetHeights()[x * GetVertexCountZ() + z]`(行=X方向、列=Z方向。PhysX の高さフィールドと同じ並び)。`maxHeight` を掛けた後のワールドの高さです。
- **当たり判定(6.8)**: `RigidbodyComponent.h/.cpp` に `TerrainRigidbodyComponent` を、`Physics.h/.cpp` に `GetCooking()`(`PxCreateCooking`。最初に呼ばれた時に作り、`Physics::Uninit()` で解放)を足しました。`GetHeights()` を `PxHeightFieldSample`(16ビット整数 + `tessFlag`)に直し、`PxCooking::createHeightField` で `PxHeightField` を作って、`PxHeightFieldGeometry`(`rowScale = GetCellSizeX()`、`columnScale = GetCellSizeZ()`)で Static のアクターにします。`PhysX.props` には `PhysXCooking_64.lib` が元から入っていたので変えていません。**`tessFlag` を立てた向きが `Terrain` の対角線と同じ**ことは、`GetHeightAt` と `Physics::Raycast` の高さを24万点以上で比べて確かめました(`docs/EngineRoadmap2.md` の 6.8)。

### まだ無いもの

- 高さで画像を混ぜる(草・岩・雪など)貼り方
- 地形を小さく分けて、映る所だけ描くこと(チャンク)
- 地形の編集(実行中に高さを変える)

---

## 35. 水面(WaterRenderer)

波打つ半透明の水面を置けます(`docs/EngineRoadmap2.md` の 6.9)。池・湖・海のように、**地形(§34)のくぼみに水を張る**使い方がいちばん分かりやすいです。波は頂点シェーダーが時間から計算するので、毎フレームCPUで頂点を書き換えることはしません。**水面を置かない(`WaterRenderer` を付けない)限り、今までの見た目は1ピクセルも変わりません**(専用のシェーダー `Shader3DWater.hlsl` も、水面を作った時に初めて読まれます)。

### 使い方

```cpp
// 1. 設定を決める(決めなかった項目は既定値)
WaterSettings settings;
settings.size = { 650.0f, 700.0f };              // 水面の大きさ(X, Z)
settings.cells = 96;                             // 格子の細かさ
settings.color = { 0.1f, 0.4f, 0.6f, 0.55f };    // 水の色(wが透明度。0=透明、1=不透明)
settings.waveHeight = 8.0f;                      // 波の高さ(水位から上下に最大8)
settings.waveLength = 300.0f;                    // 一番大きい波の長さ
settings.waveSpeed = 70.0f;                      // 波の進む速さ

// 2. オブジェクトに WaterRenderer を付ける(オブジェクトの位置が水面の中心で、Yが水位)
GameObject* water = scene.CreateObject("Water");
water->transform.position = { 2075.0f, -16.0f, 1350.0f };
water->SetDrawPriority(100);                     // 不透明な物(地形・Player)より後に描く(大事。下の「描く順番」)
auto* renderer = water->AddComponent<WaterRenderer>(settings);

// 3. その場所の水面の高さ(ワールドのY)を調べる。浮かべる物・水の中かどうかの判定に使える
float surfaceY = renderer->GetHeightAt(player.x, player.z);
if (player.y < surfaceY && renderer->Contains(player.x, player.z)) { /* 水の中 */ }

// 4. 実行中に手直しもできる(次のフレームから反映)
renderer->GetSettings().color.w = 0.3f;
```

`Sample3D/Game/Objects/Water.cpp` が実例です。

### 項目(`WaterSettings`)と既定値

| 項目 | 意味 | 既定 |
|---|---|---|
| `size` | 水面の大きさ(X, Z)。実行中に変えられる | `{10000, 10000}` |
| `cells` | 格子の分割数(**1〜512**。範囲外は丸める)。多いほど波が細かく、頂点が増える。変えると格子を作り直す | 128 |
| `color` | 水の色。**wが透明度**(0=透明、1=不透明) | `{0.1, 0.4, 0.6, 0.6}` |
| `skyColor` | 空の色。浅い角度で水面に映る色(反射の簡易表現) | `{0.6, 0.8, 1, 1}` |
| `waveHeight` | 波の高さ。水位から、上下に**最大でこの高さ**まで動く(山の高さの半分) | 15 |
| `waveLength` | 一番大きい波の、山から山までの長さ | 400 |
| `waveSpeed` | 一番大きい波の、1秒に進む距離 | 120 |
| `waveDirection` | 波の進む向き(X, Z。長さは使わない。`{0, 0}` は `{1, 0}` として扱う) | `{1, 0.4}` |
| `specularStrength` | 太陽・光のきらめきの強さ。**0=きらめかない** | 1 |
| `shininess` | きらめきの鋭さ(大きいほど小さく鋭い) | 128 |
| `fresnelPower` | 浅い角度でどれだけ空が映るか(大きいほど、水面すれすれだけ映る。0なら全体が空の色) | 4 |
| `receiveShadow` | `true` にすると、他の物の影を受ける(§32)。水面は影を**落とさない** | `false` |

範囲外の値(波の長さ0以下・大きさ0以下など)は、壊れないよう内部で丸めて使います。

### 波の仕組み

- **4本の波を重ねます**。一番大きい波は設定の値そのままで、残りの3本は、少しずつ細かく・弱く・向きをずらします。

  | 波 | 長さ | 高さ(`waveHeight` に対する割合) | 向き(`waveDirection` から) | 速さ |
  |---|---|---|---|---|
  | 1 | `waveLength` | 50% | そのまま | `waveSpeed` |
  | 2 | x 0.62 | 25% | +25度 | x 0.79 |
  | 3 | x 0.37 | 15% | -40度 | x 0.61 |
  | 4 | x 0.23 | 10% | +65度 | x 0.48 |

  (高さの割合の合計は100%なので、波がいちばん重なっても `waveHeight` を超えません。速さは「長い波ほど速い」関係です。)
- **高さ** `h = Σ 振幅 × sin(向き・位置 × 周波数 − 位相)`(周波数 = 2π ÷ 波の長さ)。位置は**ワールドのXZ座標**なので、水面のオブジェクトを動かしても、波の模様は同じ場所に残ります(水面の板だけが動く)。
- **法線(面の向き)**は、同じ式の傾き(微分)から**ピクセルごとに**求めます。頂点の間でも滑らかで、波がカクカクしません。
- **時間**は `Time::GetTime()`(§10)です。**ポーズ中(倍率0)は波も止まり、スロー再生中は波もゆっくりになります**。シーン切り替えの演出中も止まります。波ごとの「位相」(0〜2π)は、C++ 側で `double` を使って求めてからGPUへ送るので、何時間遊んでも `sin` の精度が落ちず、波は荒れません。(`Time::GetTime()` 自体は `float` なので、100時間を超えるような長い遊びでは、波の動きの刻みが粗くなります。)
- **`GetHeightAt` はシェーダーと同じ式**(CPUにも同じ式が書いてあり、同じ設定から計算します)。別に書いた式との違いは0.0001未満で、描かれた水面の頂点の高さも、この式と一致することを確かめました。ただし**描かれる水面は、頂点の間を直線でつないだ形**です。格子が波に対して十分細かければ(下の「格子の細かさ」の目安)、`GetHeightAt` の値(式そのものの値)との違いは0.1以下です。**格子が荒いと(たとえば、既定の大きさ10000に対して分割数128のままだと)、頂点の間の直線と式の値が、数ずれます**(最大で `waveHeight` の3分の1ほど)。大きな水面は `cells` を増やしてください。
- **波の傾きの目安**: 最大の傾きはおよそ `11 × waveHeight ÷ waveLength`(既定なら約0.4=約22度)です。波を高く・短くしすぎると、水面が針のようにとがって見えます。

### 色と半透明

- **`color.w` が透明度**です。下の物(水の中の球・地面など)が透けて見えます。0なら水の色は出ず、1なら下は見えません。
- **見た目の式**(平行光源 `L`、カメラへの向き `V`、波の法線 `N`):

  ```
  fresnel = pow(1 - saturate(dot(N, V)), fresnelPower)      // 浅い角度ほど1に近づく
  水の色  = color.rgb × (環境光 + 平行光源 × saturate(dot(N, L)) + 点光源・スポットライトの光)
  色      = lerp(水の色, skyColor, fresnel)                  // 浅い角度ほど空の色が映る(空の色は光の影響を受けない)
  光沢    = 太陽・光の光沢(ブリン・フォン。pow(dot(N, H), shininess) × specularStrength)
  出力    = 色 × color.w + 光沢      (ブレンド: ONE / INV_SRC_ALPHA)
  ```

- **きらめき(光沢)は透明度で薄まりません**。`color.w = 0` でも、光沢だけは見えます(消したい時は `specularStrength = 0`)。
- **水の中から見上げても**水面が見えます(両面を描き、法線を見ている側へ向けます)。水中の霧や色の変化(水の中専用の表現)はありません。
- 色は `Material`(§30)ではなく `WaterSettings` で決めます。縁の光(リム)・枠線(§33)は付けません(フレネルが似た役目をします)。

### 描く順番(大事)

- 水面は半透明なので、**不透明な物(地形・Player・立方体・球)の後**に描く必要があります。**`water->SetDrawPriority(100)`** のように、`drawPriority` を大きくしてください(Sample3D は100。`Hills` と `Ground` は `INT_MIN + 1`、`GlassPanel` は10)。
- **深度**: 深度テストは**する**(手前の物に水面が隠される)が、深度は**書かない**(`depthWrite = false`。水面の中にある物が消えない)。
- **両面描画**(カリングなし)です。
- 描く順番は `drawPriority` → `z` の小さい順で、**カメラからの距離順ではありません**(§30)。そのため、**水面の中にいる半透明のもの(敵の画像など)と水面の前後は、正しくならないことがあります**(水面より先に描かれた物は水越しに、後に描かれた物は水面の上に出ます)。許容しています。
- 波が高い・カメラが水面すれすれにある時は、**波の裏側が重なって、そこだけ少し濃く見える**ことがあります(深度を書かないため、視線が水面を2回またぐと、水面が2枚重なって描かれます)。波の傾き(上の目安)がカメラの俯角より急だと起きます。
- **水面と地形が交わる所は、くっきり切れて見えます**。「浅い所は薄く、深い所は濃く」「岸辺の白い泡」は、シーンの深度バッファを読む必要があり(今の深度バッファは読めない形です)、まだ作っていません。

### ライト・影との関係

- **平行光源**(`Image::GetLight()`)と環境光で、水の色が明るくなり、太陽のきらめきが出ます。**ポイントライト・スポットライト**(§31)も、水面を照らし、光沢で光ります(最大8個。`Mesh` と同じ光の一覧を使います)。光が無ければ、環境光だけです。
- **影**(§32): 水面は影を**落としません**(影のパスには出ません)。`receiveShadow = true` の時だけ、他の物の影を受けて、平行光源(と影を落とすスポットライト)の光が暗くなります。既定は `false` です。
- 影が有効でも、`receiveShadow = false` の水面は何も変わりません。

### 画面外の判定

`renderer->SetFrustumCulling(true)` にすると、水面全体(波の高さの余裕込み)を囲む球がカメラに映らない時は描きません。**既定は `false`** です(`TerrainRenderer` と同じ。§28)。省いた数は `Visibility::GetCulledCount()` に数えられます。

### 制限・気を付けること

- **オブジェクトの `scale` は `{1,1,1}`、回転なし**にしてください(大きさは `WaterSettings::size` で決めます)。位置(と親)だけが使われます。拡大縮小や回転があると、`GetHeightAt` と見た目がずれます。
- **当たり判定は付きません**(通り抜けられます)。水に入ると遅くなる・物が浮く、といった処理は、`GetHeightAt` と `Contains` で調べて自分で作ってください(Sampleでは作っていません)。
- **格子の細かさ**: 波1つに格子が4個以上入るようにします(**既定の `size` = 10000・`cells` = 128 の組み合わせは、格子が荒すぎます**。大きな水面は `cells` を増やしてください)。**一番短い波は `waveLength` の0.23倍**なので、`cells` の目安は `size ÷ (waveLength × 0.23) × 4` 以上です(`size = 700`・`waveLength = 300` なら41以上。Sampleは96)。細かすぎる格子は頂点が増えます(128で頂点16641個・三角形32768枚、512で頂点263169個・三角形524288枚)。
- **水位は、くぼみの縁より少し下に**してください(波の山が `waveHeight` だけ上がります)。
- **地形の外の水面の縁**は、そのまま四角く切れて見えます。くぼみが閉じていない(外へ低くなっている)場所に置くと、水面の縁が空中に浮いて見えるので、縁が地形に隠れる場所に置くか、見える側から離しておいてください(Sampleの池は、北と西の縁が少し浮いて見えます)。
- 水面を何枚置いてもよいですが、**重なった所は2枚ぶん濃く**なります(深度を書かないため)。
- 実行中に `GetSettings()` を変えると、次のフレームから反映されます。`cells` を変えた時は、格子を作り直します。
- シェーダー(`Shader3DWater.hlsl`)は実行時にコンパイルされます。書き方に間違いがあると、水面だけが描かれなくなり、出力ウィンドウにエラーが出ます。

### 性能

- **1つの水面は1回の描画**(`DrawIndexed`)です。頂点とインデックスは作る時に1回だけ送り、描く時のコピーはしません(`TerrainRenderer` と同じ)。
- 波の計算は、頂点シェーダーとピクセルシェーダーで**毎回4本ぶん**(`sin` / `cos`)です。**画面いっぱいに水面が映ると、ピクセルシェーダーの分が重くなります**(光の数ぶんのループも加わります)。実機のGPUでは測っていません。重い時は、`SetFrustumCulling(true)`・`cells` を減らす・水面を小さくする、で軽くなります。

### Sampleでの使われ方

- **Sample3D の `Water`**(`Objects/Water.cpp`): 丘(`Hills`。§34)の北西のくぼみ、位置 `(2075, -16, 1350)` に、650 x 700 の池を置いています(水位は、丘の地面の高さ0から数えて34。くぼみの底は19〜24ほど、東と南の縁は38〜40ほどで、そこが岸になります)。青緑(`{0.1, 0.4, 0.6, 0.55}`)で、`waveHeight = 8`・`waveLength = 300`・`waveSpeed = 70`・`cells = 96`、`receiveShadow = true`。`SetDrawPriority(100)` で、不透明な物より後に描きます。**当たり判定は付けていない**ので、Playerは通り抜けます(水に入っても遅くなりません)。
- **`GameScene` では `CreateWater(*this, 位置)` を1行並べるだけ**です(Sceneには「何を置くか」だけを書く)。
- **Sample2D** は水面を使いません(シェーダーも読まれません)。

### 仕組み(Engineを触る人向け)

- **ファイル**: 新しく `Engine/Render3D/WaterRenderer.h/.cpp` と、ソリューション直下の `Shader3DWater.hlsl` を足しました(`Engine.vcxproj` の `CopyRuntimeFiles` と `UpToDateCheckInput` に登録済み)。**既存のEngineファイル・`Shader.hlsl`・`Shader3D.hlsl`・`Shader3DSkin.hlsl`・`Shader3DCommon.hlsli`・`PhysX.props` は変えていません。**
- **格子**: `(cells+1)^2` 個の頂点(`float2` の位置だけ。-0.5〜0.5)と、32ビットのインデックスを、`Mesh::InitIndexed` と同じ「作る時に1回だけ送る(`IMMUTABLE`)」作りで `WaterRenderer` が持ちます(`Mesh` は専用のシェーダーを使えないため、`Mesh` そのものは使っていません)。大きさは、シェーダーが `size` を掛けて決めます。
- **レジスタ**: `b0`=`MeshConstantBuffer`(`Shader3D.hlsl` と同じ形)、`b3`=カメラの位置、`b4`=ポイントライト・スポットライト(`Lighting::Bind()`)、`b5` / `t1` / `t2` / `s1`=影(`Shadow::Bind()`)、**`b6`=`WaterBuffer`(水面だけ)**。`b6` は `float4` が9本(144バイト)で、C++ の `WaterConstantBuffer` と並びが一致するよう `static_assert`(大きさと項目の位置)を入れています(シェーダーの反射(`D3DReflect`)でも、大きさ・項目の位置・レジスタ番号が一致することを確かめました)。
- **`Shader3DCommon.hlsli` を `#include`** して、`b0`〜`b5` の宣言と `SunShadow` / `SpotShadow` を使い回しています。点光源・スポットのループは、`ShadePixel`(`Material` 用)とは別に、水面用に書き直してあります(**`ShadePixel` の光の式を変えた時は、`Shader3DWater.hlsl` も見直すこと**)。
- **波の式は2か所**: `Shader3DWater.hlsl` の `EvalWaves`(GPU)と `WaterRenderer.cpp` の `EvalWaves`(`GetHeightAt` 用)。**片方を変えたら、もう片方も同じに直すこと**(波の本数・割合・向きのずれの定数は `WaterRenderer.cpp` の先頭にあり、設定から作った波の表を `b6` に送ります)。
- **ブレンド・深度・ラスタライザ**のステートは、`WaterRenderer` の `static` 共有物です(`Mesh` と同じく、最後の1体が手放した時に解放)。**描く前の状態を覚えておき、描いた後に元へ戻します**(ブレンド係数・サンプルマスク・ステンシルの参照値も)。水面だけが使う `b6` も外します。
- **確認(WARP=ソフトウェアのD3D11)**: ①水面を一度も作らない絵と、水面を作って描いて壊した後の絵が、1ピクセルも違わない(描画状態が残らない)。②平らな水面・波のある水面(時間0・2.5秒・36000秒)・光の向きを変えた絵・ポイントライトとスポットライトを足した絵・水中から見上げた絵・影を受ける絵を、別に書いたCPUの計算(波の式・フレネル・光・光沢・ブレンド・影)と画素ごとに比べて、**最大1/255**で一致(水中から見上げた絵は、波の縁の1画素が10/255)。③波で持ち上げた水面の形を、真横から見て式の形と比べて、食い違いなし。④水面を作って壊す度に、D3D11の生存オブジェクトの数が元に戻る。(実際のゲームでの見え方・実機GPUの速さは、まだ確かめていません。)

### まだ無いもの

- **浅い所は薄く・深い所は濃く**、**岸辺の白い泡**(深度バッファを読む表現。今の深度バッファが読めない形なので、`Graphics.cpp` の変更が要ります)
- 本物の反射(反射用の別の画面を描く)・屈折・波しぶき
- 水に入ると遅くなる・物が自動で浮かぶ処理(`GetHeightAt` で調べて、自分で作ります)
- 水の中専用の見た目(霧・色の変化)
- 波の本数や形(波の割合・向きのずれ)を設定で変えること

---

## 36. 経路探索(NavGrid / PathFollowComponent)

敵が**壁・障害物を避けながら**目的地(Player)へ向かうための部品です。`ChaseComponent`(§13)は「まっすぐ向かう」だけなので、壁の向こうにPlayerがいると壁に張り付いて止まります。`PathFollowComponent` は、地面を升目(格子)にして**最短の道を探し**(A\*=エースター)、その道に沿って歩きます。2D(Sample2D)でも3Dの地面(Sample3D)でも、同じ部品で使えます。

| 部品 | 役割 | 置き方 |
|---|---|---|
| `NavGridComponent` | 地面の地図。壁(タグ `"Wall"`)を自動で読み、通れるマス・通れないマスを覚える。経路を探す役(`PathFinder`)も持つ | シーンに**1つ**置く(`Objects/Navigation.cpp`) |
| `PathFollowComponent` | 地図を見て経路を探し、その道を歩く。`MoveComponent` の子なので `ChaseComponent` と同じ呼び方 | 敵などに付ける |
| `NavGrid` / `PathFinder` | 地図のデータと探す処理そのもの(Componentではない。`NavGridComponent` の中にある) | 直接使うのは、自分で経路だけ欲しい時 |

### 使い方

```cpp
#include "../../../Engine/Behavior/NavGridComponent.h"
#include "../../../Engine/Behavior/PathFollowComponent.h"

// ① シーンに地図を1つ置く(壁をすべて置いた後、Scene::Init の最後の方で)
GameObject* navigation = scene.CreateObject("Navigation");
NavGridSettings settings;
settings.origin = { -3000.f, -3000.f, 0.f };   // 地図の左手前の角
settings.size = { 6000.f, 6000.f };            // 地図の大きさ
settings.cellSize = 50.f;                      // 1マスの大きさ
settings.plane = MovePlane::XY;                // 2D=XY、3Dの地面=XZ
settings.agentRadius = 50.f;                   // 歩く物の半径
NavGridComponent* nav = navigation->AddComponent<NavGridComponent>(settings);   // 壁(タグ "Wall")は自動で読まれる

// ② 敵に PathFollowComponent を付ける(ChaseComponent の代わり)
auto* path = enemy->AddComponent<PathFollowComponent>();
path->SetTarget(player);          // 追いかける相手(MoveComponent と同じ)
path->SetSpeed(60.f);
path->SetPlane(MovePlane::XZ);    // 3Dの地面の上を動く時
path->SetNavigation(nav);         // 地図(ObjectPool の敵は Spawn の度に渡す)
```

壁は、置くだけで地図に反映されます(Staticな `BoxRigidbodyComponent` で、タグが `"Wall"` の物。`Wall` の工場関数がこれをしています)。

### 設定(`NavGridSettings`)

| 項目 | 既定値 | 内容 |
|---|---|---|
| `origin` | 0 | 地図の左手前の角(ワールド座標)。2Dは x,y,z、3Dは x,z(高さは各マスが覚える) |
| `size` | 6000 x 6000 | 地図の大きさ(2Dは横とたて、3DはXとZ)。**地図の外では敵は経路を探せません(止まります)** |
| `cellSize` | 50 | 1マスの辺の長さ。大きいほど軽くて粗い。**歩く物の半径くらい**がおすすめ。上限(512 x 512 = 262,144マス)を超えると、自動で大きくして出力ウィンドウに知らせる |
| `plane` | `XZ` | 歩く平面(`MoveComponent` と同じ) |
| `agentRadius` | 40 | 歩く物の半径。障害物をこの分だけ太らせる(下) |
| `obstacleTag` | `"Wall"` | このタグの物が障害物。`""` にすると物理の障害物は無し |
| `baseHeight` | 0 | (3D)地形の無い所の地面の高さ(Sample3D の `Ground` なら `-50`) |
| `agentHeight` | 100 | (3D)歩く物の背の高さ。壁を探す高さの上の端 |
| `maxStepHeight` | 30 | (3D)登れる段差の高さ。壁を探す高さの下の端(これより低い物は障害物にしない) |
| `maxSlopeDegrees` | 40 | (3D)登れる坂の角度。これより急なマスは通れない |

**`agentRadius` で障害物を太らせます**: 壁から半径ぶん離れたマスも「通れない」ことにします。こうすると、歩く物を**1つの点**として歩かせても、体が壁にこすりません。半径が大きいと、狭い通路が全部ふさがります(体が入らないので正しい動きですが、「通れそうなのに遠回りする」ように見えます)。

### 障害物の取り込み

| 取り込み方 | 内容 | 使い所 |
|---|---|---|
| **物理から(自動)** | 全マスについて、`Physics::OverlapBox`(マスの大きさ + 2 x `agentRadius`)でタグ `"Wall"` の物を探し、見つかったマスを通れなくする。Staticでも、動く(Kinematic)壁でも同じ | 壁を置くだけ |
| **手で登録** | `nav->GetGrid().SetBlockedBox(中心, 大きさ)`(`SetBlocked(cellX, cellZ, true)` は1マス)。高さは見ずに、`agentRadius` ぶん太らせる | 物理の無い障害物(池・見えない柵など) |
| **地形から(3D)** | `nav->SetTerrain(TerrainRenderer*)`。各マスの地面の高さを覚え、**急な坂**(傾きが `maxSlopeDegrees` 超え)と**大きな段差**のマスを通れなくする | Sample3D の `Hills`(§34) |

- **読まれるのは2回目の `Update`**: 物理に出したばかりの物は次のフレームから見つかるので、`Init` では読めません。`NavGridComponent` が2回目の `Update` で1回だけ焼きます(`IsBaked()` が true になる)。焼き終わるまで、`PathFollowComponent` は止まって待ちます。
- **2D(XY)**: 壁の奥行き(Z)が100あり、敵は `z = 0` にいます。壁を探す箱はZを10の薄さにしてあります(Zが離れた壁は2Dの敵には関係ない)。
- **3D(XZ)**: 壁を探す箱の高さは「地面の高さ + `maxStepHeight` から 地面の高さ + `agentHeight` まで」です。**地面そのもの(`Ground`)や低い段差は障害物になりません**(タグが `"Wall"` の大きな床があっても平気)。
- **回転した壁**も正しく見つかります(PhysXが形で調べる)。
- 池(`Water`)は当たり判定が無いので、**そのままでは通れます**。通れなくしたい時は `SetBlockedBox` で手で登録します(Sampleでは通れるまま)。
- 他の敵や Player は地図に入れません(敵どうしは物理で押し合います)。

### 3Dの地形(高さ・坂・段差)

3Dは「平面の升目 + 各マスの地面の高さ」(2.5次元)です。洞窟・橋の下のような、**同じ場所に床が複数ある所は扱えません**。

- 地形があるマスは `TerrainRenderer::GetHeightAt`、無いマスは `baseHeight`。
- 傾き: マスの中心と4つの角の `GetNormalAt` を調べ、1つでも `maxSlopeDegrees` を超えたらそのマスを通れなくします。
- 段差: 隣のマスとの高さの差が、「`maxStepHeight`」と「坂の傾き(`tan(maxSlopeDegrees)` x マスの距離)」の大きい方を超えたら、そのマスを通れなくします(ゆるやかな坂が段差と間違われないように)。
- 敵が動くのは平面(XZ)の上だけで、**高さ(Y)は重力と地形の当たり判定に任せます**(丘の斜面を登る時は、物理が足元を押し上げる)。経路の点の高さは、デバッグの線のためです。
- Sample3D の `Hills` の三角形の傾きは**最大で約33度、平均で約13度**です(実測)。`maxSlopeDegrees = 40`(既定)では**丘の坂は1マスも通れなくなりません**。20度にすると丘のマスの約15%、30度にすると約1%が通れなくなります。坂を避けさせたい時は、この値を下げてください。

### 動く壁(扉・動く壁)

壁のオブジェクトに `"Wall"` タグを付けておけば、焼き直しで反映されます。

| 方法 | 内容 |
|---|---|
| `nav->SetAutoRebakeInterval(1.f)` | 1秒ごとに全体を焼き直す(0=焼き直さない。既定) |
| `nav->RequestRebake()` | 次のフレームで全体を焼き直す |
| `nav->RebakeRegion(中心, 大きさ)` | 次のフレームで、範囲だけ焼き直す(軽い。壁を動かした側が呼ぶ) |

焼き直しはどれも**次のフレームの `Update`** で行います(Kinematicの壁は、動かしたフレームの次の `Physics::Update` で物理の位置が変わるため)。焼き直しで障害物が変わると格子の `GetVersion()` が増えるので、歩く側が気付いて経路を引き直します(変わらなければ増えません)。

### 歩き方の決まり(`PathFollowComponent`)

- **経路が見つからない時は、止まります**(速度0)。次の場合です: 敵または目的地が地図の外 / 壁で囲まれていて行けない / 地図がまだ焼けていない / 1回の探索で調べるマスの上限(20000)を超えた。**止まっている間も探し直す**ので、目的地や障害物が変わって行けるようになったら、また動き出します。
- 経路は「見通せる限り先の点へ飛ばす」直線化をしてあるので、カクカクしません。壁の角を斜めに抜ける道は探しません。
- **探し直す時**: 一定の間隔(`SetRepathInterval`。既定0.5秒。同じフレームに集中しないよう±20%ばらす)/ 目的地が1マス以上動いた時 / 格子が変わった時 / **詰まった時**(1秒間に「速さ x 0.2」ぶんも進めなかった時。敵どうしの押し合いの保険)。経路が見つからない間は、目的地が1マス動いても間隔の下限(4倍=約2秒)を守ります(行けない所の探索は重いため。格子が変わった時=壁が動いた時は、すぐ探し直します。行けない敵50体で、目的地が動き続けても探索は10秒で約260回=1フレーム平均0.4回)。
- **1フレームに探索する回数は8回まで**(`nav->SetMaxSearchesPerFrame`)。超えた敵は次のフレームにまわります。
- **途中の点**は `SetWaypointRadius` の距離(既定: `cellSize` の半分)に近づいたら次へ進みます。最後の点は目的地です。
- **目的地が壁際**(太らせた障害物の中)の時は、近くの通れるマスの中心まで行き、**残りは目的地へまっすぐ**向かいます(Playerが壁にくっついていても、敵は体が壁に当たる所まで近づく)。**出発点が壁際**の時も、近くの通れるマスへ出てから探します。
- 目的地の相手が非アクティブの間は止まります(`MoveComponent` の決まり)。`ObjectPool` で使い回された時(`SetActive(true)`)は、経路・タイマーをリセットします(相手・速さ・地図の設定は残る)。
- 動かし方は `MoveComponent` と同じです(§13。DynamicはSetVelocity、XZならYの速度は残る)。

### デバッグ表示

```cpp
NavGridComponent::SetDebugDrawEnabled(true);      // 通れないマスの境目を赤い線で描く(壁を太らせた範囲が見える)
PathFollowComponent::SetDebugDrawEnabled(true);   // 今の経路を緑の線で描く(敵がたくさんいると線が増える)
```

`RigidbodyComponent::SetDebugDrawEnabled`(§23)と同じ使い方です(`GameManager::Init()` の `VisionComponent::SetDebugDrawEnabled` の近くに書くと切り替えやすい。Sampleでは、どちらもOFFです)。線を描く部分は、OFFの時は何もしません。赤い線は、障害物が変わった時(焼き直し)だけ作り直します。

### 制限・気を付けること

- 地図は**有限の範囲**です。Playerが地図の外まで歩くと、敵は経路を探せず**止まります**(敵の出現位置は `NavGrid::ClampToGrid` で地図の中に収めるので、外で出て止まる敵は出ません。Playerが外にいる間は、地図の縁に出て止まります)。地図は、Playerが歩き回る範囲より広くしてください(Sampleは Sample2D が 6000 四方、Sample3D が 9000 四方)。
- 経路は**他の敵やPlayerを避けません**(壁などの動かない物だけ)。敵どうしは物理で押し合います。
- 敵の体が大きいと、狭い通路は通れません(`agentRadius` ぶん太らせるため)。通れるはずなのに回り道をする時は、`agentRadius` と `cellSize` を見直してください。
- 目的地が壁際で、寄せ先から目的地までの間が狭い極端な場合、敵が寄せ先と目的地の間を行き来することがあります(目的地が動くまで探し直さない決まりで抑えてありますが、完全には防げません)。
- マスより細い隙間・薄い障害物でも、`agentRadius` ぶん太らせるので塞がります。逆に、マスより細かい動きはできません(`cellSize` が大きいと粗くなる)。
- 地形のマスより `cellSize` が大きいと、細い谷・尾根を見逃します(`cellSize` 50〜100なら Sample3D の丘は十分に拾える)。
- 同じ場所に床が複数ある所(橋の下など)は扱えません。
- 「フローフィールド」(目的地が同じ敵が何百体もいる時に、一度の計算で全員の向きを決める方式)は作っていません。必要になったら同じ格子の上に足せます。

### 性能(実測。Release、このPCの値)

| 項目 | 時間 |
|---|---|
| Sample2D の格子(120 x 120 = 14,400マス)を焼く(物理) | 約 0.1〜0.25ミリ秒 |
| Sample3D の格子(180 x 180 = 32,400マス)を焼く(物理) | 約 0.2ミリ秒 |
| Sample3D の丘の高さ・傾きを取り込む(`BakeFromTerrain`) | 約 12ミリ秒(Debugビルドでは約 200ミリ秒)。シーンの読み込み時に1回 |
| 1回の探索: 見通せる(探さずに済む) | 約 0.008ミリ秒 |
| 1回の探索: 壁を回り込む | 約 0.01〜0.04ミリ秒 |
| 1回の探索: 丘をまたぐ長い道(約5,900マスを調べる) | 約 1.1〜1.3ミリ秒 |
| 1回の探索: 行けない(上限20000マスまで調べて諦める) | 約 4.6〜4.8ミリ秒 |

- 物理の問い合わせは、まず 8 x 8 マスをまとめた大きな箱で調べ、何も無ければそのまとまりを飛ばします(1マスずつ調べると、同じ格子で約3倍かかる)。
- メモリ: 格子は1マス約5バイト、探索の作業用が1マス約16バイト(格子ごとに1組で、敵ごとには持たない)。Sample3D の格子で、合わせて約700KBです。
- 敵が100体、探し直しが0.5秒ごとなら、1秒に約200回(1フレームに約3回)。見通せる・壁を回り込む程度なら、1フレームあたり 0.1ミリ秒に満たない計算です。**重くなるのは「行けない」探索が多い時**(間隔を4倍にして、1フレーム8回までに抑えてある)。

### Sampleでの使われ方

- **Sample2D**: `Objects/Navigation.cpp` の `CreateNavigation(scene)` が、原点の周りの 6000 x 6000(`cellSize` 50、`agentRadius` 50=敵は一辺100の四角)の格子を置きます。壁は既存の `Wall` 2枚(タグ `"Wall"`)がそのまま読まれます。`GameScene::Init()` では、壁を置いた後に `CreateNavigation(*this)` を1行並べ、できた格子を `CreateEnemySpawner(*this, player, navigation)` に渡します。
- **Sample3D**: `CreateNavigation(scene, hills)` が、`(-3000, -3000)` から 9000 x 9000(`cellSize` 50、`agentRadius` 40=敵は半径40のカプセル、`baseHeight` -50)の格子を置き、`CreateHills` が返した丘の `TerrainRenderer` を `SetTerrain` で渡します(`maxSlopeDegrees` は既定の40度)。池(`Water`)は通れるままです。
- **両サンプルの敵**: `EnemyManager` が、`ChaseComponent` の代わりに `PathFollowComponent` を付けます(置き換えたのは `SetupEnemy`・`Spawn`・`ApplySpeedToEnemies` の3か所だけ。`Spawn` で `SetNavigation` も渡す)。プール・`Enemies.csv` の速さの反映(`ApplySpeedToEnemies`)・倒した時の返却は今までどおりです。
- **ゲームの難しさが変わります**: 敵が壁を避けて回り込んで来るので、壁の陰に隠れても追いつかれます(今までは敵が壁に張り付いて止まった)。逆に、Playerが地図の外に出ると敵は止まります(敵は地図の縁に出る)。

### 仕組み(Engineを触る人向け)

- **ファイル**: 新しく `Engine/Behavior/NavGrid.h/.cpp`・`PathFinder.h/.cpp`・`NavGridComponent.h/.cpp`・`PathFollowComponent.h/.cpp` を足しました(`Engine.vcxproj` と `.filters` に登録)。**既存のEngineファイル・`Shader*.hlsl`・`PhysX.props` は変えていません**(`MoveComponent` / `ChaseComponent` / `Physics` / `Terrain*` は読むだけ)。
- **障害物は3種類を別のビットで覚えます**(手で登録 / 物理から / 地形から)。焼き直しは自分の種類だけ作り直すので、`SetBlockedBox` で手で登録した分は消えません。`Clear()` は全部消します。
- **A\***: 8方向(斜めは、隣り合う2マスの両方が通れる時だけ)。ゴールまでの見込みは「斜めも使える時の最短」で、実際より大きくならない=必ず最短の道になります(別に書いたDijkstraと、ランダムな迷路で比べて一致を確かめました)。作業用の配列は探索の番号(`stamp`)で「今の探索で見たか」を見分けるので、毎回塗り直しません。
- **直線化**: 経路の点を先から順に、`NavGrid::HasLineOfWalk`(線が通るマスを順にたどる)で見通せる限り飛ばします。線がマスの角をちょうど通る時は、両脇のマスも通れなければ不可にします。`Physics::Raycast` は使いません(太らせた障害物が自動で入り、物理を呼ばないので速い)。
- **まっすぐ行けるなら探さない**: `smooth` の時は、出発点から目的地に `HasLineOfWalk` が通れば、A\*を呼ばずに `[目的地]` だけを返します(広い所ではほとんどこれで済む)。
- **デバッグの線**: `Mesh`(LINELIST)で描きます。格子の赤い線は、境目の辺だけをまとめて1つの `Mesh` にします。経路の緑の線は、長さ1の線の `Mesh` を全員で1つ共有し、`Transform` の `scale`(長さ)と `rotate`(向き)で点と点の間に置きます(`VisionComponent` の扇形と同じ工夫)。
- **確認(物理あり、Debug・Release の両方)**: ①ランダムな迷路600個で、A\*の最短の長さがDijkstraと一致、到達不能は空、直線化した経路が壁を通らない・長くならない ②斜めのすき間は抜けない・上限を超えたら空・格子の外・壁の中の出発点/目的地 ③物理から取り込んだマスが、「マスを半径ぶん太らせた箱が壁と重なるか」の計算と全マス一致(タグ違い・Zが離れた壁・タグ付きの床・低い段差は障害物にならない)④Dynamicの箱(2D)・カプセル(3D)に `PathFollowComponent` を付けて物理の中で動かし、壁を回り込んで目的地に着く(`ChaseComponent` は壁で止まる) ⑤動く壁(扉)が開くと止まっていた敵が動き出す ⑥Hills の上を、傾き上限30度の格子で歩く ⑦一時停止(dt=0)・`ObjectPool` の再利用。(実際のゲームでの見え方・遊んだ時の難しさは、まだ確かめていません。)

### まだ無いもの

- **フローフィールド**(目的地が同じ敵が非常に多い時の方式)・ナビメッシュ・Playerを中心に動く地図
- 他の敵を避ける動き(群れの分離)
- 経路が見つからない時に「まっすぐ追う」ことを選ぶ設定(今は止まる)
- 探索を複数のフレームに分けること(分割A\*)・別スレッドでの探索

---

## 37. LOD(距離で省く)と描画カウンタ(Lod / RenderStats)

遠くの物を軽くするための機能です(`docs/EngineRoadmap3.md` の 7.2)。**今あるのは「段階1」だけ**で、遠い物を **描かない / 影を落とさない / 枠線を描かない** ようにする設定(`Lod`)と、**描画の重さを数えるカウンタ**(`RenderStats`)です。メッシュを軽いものに切り替える(段階2)・地形を区画に分ける(段階3)は、カウンタで測ってから作るか決めるため、まだ作っていません。**何も設定しなければ、見た目も速さも今までと同じです**(既定はすべてOFF。LODを使わない絵は、変更前とピクセル単位で一致することを確かめました)。

### 使い方

`MeshRenderer` / `ModelRenderer` / `TerrainRenderer` が、それぞれ距離の設定 `LodSettings` を1つ持っています。**0は「使わない=いつも描く」**です。

```cpp
auto* meshRenderer = wall->AddComponent<MeshRenderer>(Mesh::CreateCube());
meshRenderer->GetLod().drawDistance    = 6000.f;   // 6000より遠い物は描かない
meshRenderer->GetLod().shadowDistance  = 2500.f;   // 2500より遠い物は影を落とさない
meshRenderer->GetLod().outlineDistance = 1500.f;   // 1500より遠い物は枠線を描かない
// まとめて渡す書き方: meshRenderer->SetLod({ 6000.f, 2500.f, 1500.f });
```

`ModelRenderer`(FBX。骨で曲げるモデルも)も同じ書き方です。`TerrainRenderer` は枠線を付けないので `drawDistance` と `shadowDistance` だけが効きます(`outlineDistance` は無視)。

### 3つの距離

| 項目 | 意味 | 省くと減るもの |
|---|---|---|
| `drawDistance` | これより遠い物は**描かない**(その物の枠線も出ない) | 描画命令・三角形(通常+枠線)。物が多い時に一番効く |
| `shadowDistance` | これより遠い物は**影のパスで描かない**(影を落とさなくなる。本番の絵は描かれる) | 影の描画命令(`Material` の `castShadow` が true の物だけが対象) |
| `outlineDistance` | これより遠い物は**枠線を描かない**(本物は描かれる) | 枠線の描画命令。枠線は1つの物につき命令を3つ出すので、省くと減る数が大きい |

- **描画を省くだけ**で、物理(当たり判定)は止まりません。`GameObject` もそのままです(`SetFrustumCulling` と同じ決まり)。
- 枠線を省いた時も、ステンシル(枠線を本物の内側に描かないための印)の状態は壊れません。枠線の描画そのものを呼ばないので、印を付けないし、消し忘れも起きません。
- `drawDistance` を `SetFrustumCulling(true)` と一緒に使うと、**先に距離を調べ**、遠ければそこで終わりです(画面の外で省いた数 `Visibility::GetCulledCount` ではなく、距離で省いた数 `RenderStats::GetLodCulledCount` に数えます)。近ければ今までどおり画面の外かを調べます。

### 距離の測り方(基準)

- **カメラの位置は `Image::GetCamera3D().position`** です(`Visibility` と同じ。`CameraShake` の揺れが入った位置。揺れは小さいので、判断にはほとんど影響しません)。ズーム(視野 `fovY` の変更)は考えていません。ズームすると、遠い物が大きく見えるのに距離では遠いままです。
- **「カメラから物の中心までの距離 − 物の半径」** で調べます(球の表面までの距離。0より小さい=カメラが球の中なら、必ず「近い」)。長い壁や地形のように大きな物が、中心は遠くても手前の端は近い、という時に消えないようにするためです。**設定の距離「ちょうど」なら省きません**(超えたら省きます)。
- 物の中心は `transform.GetWorldMatrix()` の位置(親子関係があっても正しい)。半径は、`MeshRenderer` は頂点の一番遠い点(`Visibility` と同じ)×行列の一番大きい拡大率、`ModelRenderer` は止まっている姿の一番遠い頂点×1.5(画面判定と同じ余裕)、`TerrainRenderer` は地形の箱を囲む球です。

### 全体の切り替え(Lod 名前空間)

| 関数 | 説明 |
|---|---|
| `Lod::SetEnabled(false)` | **すべての物の距離の設定を無視**します(LODなしの状態と比べる確認用。既定は `true`) |
| `Lod::SetGlobalScale(0.5f)` | すべての距離に掛ける倍率(0.5で全部半分)。まとめて調整する用。既定は1(0以下は1に戻る) |
| `Lod::GetCameraDistance(位置)` | カメラからその位置までの距離。自分のComponentで「遠いなら何かを止める」時に使えます(例: 遠い物の `ModelAnimationComponent` を `SetEnabled(false)` で止める) |
| `Lod::IsBeyond(中心, 半径, 距離)` | 球の表面までの距離が、その距離を超えているか(0以下の距離は常に `false`) |

### 描画カウンタ(RenderStats)

LODが効いているかを確かめる数です。`Visibility::GetCulledCount()` と同じく、**すべて「前のフレームの数」**を返します(描いている途中で読んでも数が揺れません)。**Releaseでも数えます**(単純な足し算だけ)。

| 関数 | 数えるもの |
|---|---|
| `GetDrawCallCount()` / `GetDrawTriangleCount()` | 本番(通常)の描画命令の数 / 三角形の数 |
| `GetShadowDrawCount()` / `GetShadowTriangleCount()` | 影のパスの描画命令 / 三角形 |
| `GetOutlineDrawCount()` / `GetOutlineTriangleCount()` | 枠線の描画命令 / 三角形 |
| `GetDrawCallTotal()` / `GetTriangleCount()` | 上の3種類の合計 |
| `GetLodCulledCount()` | 距離の設定で省いた回数の合計(描画・影・枠線を別々に数える) |

- 数えるのは3Dの描画(`Mesh` / `SkinMesh` / 水面)です。2Dのスプライトと文字は数えません。当たり判定の線(デバッグ表示)は数えます(1つの線も命令1つ)。
- **枠線は、1つの物につき命令を3つ**出します(本物の印を付ける・枠線を描く・印を消す)。なので、枠線を付けた物が100個なら、枠線の命令は300です。三角形も3回分数えます。
- `RenderStats::BeginFrame()` は `GameEngine::Run` が `Visibility::BeginFrame()` の隣で呼びます(ゲーム側は呼びません)。

### Sample3D での見方と測り方

- 画面の左上、HPゲージの下に 2 行増えました。
  - `Draw: 全体 (Shadow ○, Outline ○)` … 描画命令の数(通常+影+枠線の合計)と、その内訳(影・枠線)
  - `Tri: ○  LodCulled: ○  LOD: ON/OFF` … 三角形の数(合計)、距離で省いた数、LODが有効か
  - その上の `Culled` は今までどおり、画面の外で省いた数です。
- **ON/OFFを比べる**: `Sample3D/Game/Scenes/GameScene.cpp` の先頭の `lodEnabled`(`true`/`false`)を書き換えて再ビルドします(`false` で `Lod::SetEnabled(false)` になり、柱の距離の設定が全部無視されます)。キー操作は足していません。
- **多数の物で測る**: `Objects/WallField.cpp` が、細い柱(80 x 300 x 80)を 15 x 15 = 225 本、間隔 300 で並べます(`GameScene` で `CreateWallField(*this, { 0, 0, -6000 }, 15)`)。Playerの南(Z がマイナス側)の遠く、経路探索の格子(Z=-3000 まで)の外です。柱は `Wall` と同じ(影を落とす・受ける。Static。タグ `"Wall"`)で、さらに黒っぽい細い枠線と、`drawDistance = 6000`・`shadowDistance = 2500`・`outlineDistance = 1500` を設定してあります。Playerが南へ歩いて近づくと、柱が順に描かれ、影、枠線の順に出てきます。

### 実測(Releaseのビルド、ソフトウェア描画 WARP、480 x 270)

WARP はCPUで描く遊び用のGPUなので、**時間は参考値**です(実機のGPUは三角形に強く、命令の数が相対的に効きやすい)。信じてよいのは**命令の数・三角形の数**です。柱の並びは WallField と同じ(80 x 300 x 80、影・枠線つき)、カメラは南向きで、位置を変えて測りました。

| 場面 | 設定 | 通常 | 影 | 枠線 | 命令合計 | 三角形 | 時間(ms/フレーム) |
|---|---|---|---|---|---|---|---|
| 15 x 15(225本)、カメラが手前(Z=0) | OFF | 225 | 225 | 675 | 1125 | 13,500 | 11.5 |
| | drawDistance 6000 | 116 | 225 | 348 | 689 | 8,268 | 6.9 |
| | shadowDistance 2500 | 225 | 0 | 675 | 900 | 10,800 | 9.2 |
| | outlineDistance 1500 | 225 | 225 | 0 | 450 | 5,400 | 4.7 |
| | 3つとも | 116 | 0 | 0 | 116 | 1,392 | 1.7 |
| 30 x 30(900本)、カメラが手前(Z=0) | OFF | 900 | 900 | 2700 | 4500 | 54,000 | 42.4 |
| | drawDistance 6000 | 418 | 900 | 1254 | 2572 | 30,864 | 23.7 |
| | shadowDistance 2500 | 900 | 42 | 2700 | 3642 | 43,704 | 34.4 |
| | outlineDistance 1500 | 900 | 900 | 12 | 1812 | 21,744 | 18.4 |
| | 3つとも | 418 | 42 | 12 | 472 | 5,664 | 6.7 |

- カメラが柱の列に近づく(15 x 15 で Z=-3000。列の手前から約900)と、距離で省ける柱が減り、3つとも設定しても命令は 1125 → 375(時間 13.2 → 7.5 ms)でした。**LODの効きは、カメラと物の位置関係で大きく変わります。**
- 時間はほぼ**命令の数に比例**(1命令あたり約10マイクロ秒)でした。枠線は1つの物で命令3つなので、1つあたりの効きが一番大きく、影は1つあたり1命令分です。
- 三角形の数は、今のSampleの物(立方体=12枚、球=768枚)では時間にほとんど効きません。同じ225個を、影・枠線なしで描き比べると、立方体(12枚)2.7ms・球4x6(48枚)2.9ms・球16x24(768枚)6.1ms・球48x72(6912枚、合計155万枚)22.6ms でした。**三角形が効き始めるのは、合計が数十万枚を超えてから**です。
- ほかに確認したこと(合計116件)は、`docs/EngineRoadmap3.md` の 7.2 を見てください。

### 制限・気を付けること

- **急に消えます(ポップ)**。距離で省くだけで、フェード(少しずつ消す)はありません。`drawDistance` はカメラの `farZ`(10000)の少し手前にして、遠景で目立たなくするのが現実的です。
- **`shadowDistance` を短くしすぎない**: 影の範囲は「カメラの前方の箱」ですが、光の向きによっては、**カメラから遠い背の高い物の影が、近くの地面に落ちます**。`shadowDistance` を短くすると、「遠い木の影が突然消える」ように見えます。物の大きさ(半径)は引いて調べますが、既定のOFFのまま使うか、`Shadow::Settings::distance` くらいを目安にしてください。スポットライトの影のパスにも同じ距離が効きます。
- 距離は**カメラからの距離**です。ズームや、カメラから遠いのに画面で大きく見える物は考えていません。
- 描画と影と枠線を省くだけで、物のUpdate・物理・アニメーションの計算は止まりません。遠い物のアニメーションを止めたい時は、Sampleの側で `Lod::GetCameraDistance` を見て `ModelAnimationComponent` を `SetEnabled(false)` にしてください(止まった姿のままになります)。
- 水面(`WaterRenderer`)・スプライト(`SpriteRenderer`)・パーティクルには、距離の設定はありません(水面は描画命令の数には入ります)。

### Sampleでの使われ方

- **Sample3D**: `Objects/WallField.cpp`(柱の格子。`CreateWallField`)と、`HUDController` の `Draw` / `Tri` の表示、`GameScene` の `lodEnabled`(確認用のON/OFF)です。ほかの物(`Wall` など)にはLODを設定していないので、見た目は変わりません。
- **Sample2D**: 変更なし。3Dのメッシュを使わないので、カウンタの数は当たり判定の線の分だけです。

### 仕組み(Engineを触る人向け)

- **ファイル**: 新しく `Engine/Render3D/Lod.h/.cpp`(`LodSettings` と、距離で省く判断)と `Engine/Render3D/RenderStats.h/.cpp`(カウンタ)を足しました。既存のEngineファイルは、`MeshRenderer.h`・`ModelRenderer.h/.cpp`・`TerrainRenderer.h/.cpp`(距離の設定と判断)、`Mesh.cpp`・`SkinMesh.cpp`・`WaterRenderer.cpp`(描画命令の直前に数える。1〜2行)、`GameEngine.cpp`(`RenderStats::BeginFrame()` の1行)に少し足しました。**`Shader*.hlsl`・`PhysX.props` は変えていません**。
- 距離の設定が 0 の物は、**行列も読まずに**そのまま描きます(OFFの時の追加の計算は、`if` 1つだけです)。
- 数え方: `Mesh::Draw` / `DrawDepth` / `DrawOutline`、`SkinMesh` の同じ3つ、水面の `DrawIndexed` が、描画命令を出す直前に `RenderStats::AddDraw` / `AddShadowDraw` / `AddOutlineDraw` を呼びます。三角形は `indexCount / 3`(インデックスなし=`vertexCount / 3`)で、線のメッシュは0です。

### まだ無いもの

- **段階2(メッシュ・モデルの切り替え)・段階3(地形のチャンク分け)・段階4(ディザのフェード。シェーダー変更)**: 上の測定の結果を見て、必要かどうかを決めます(今のSampleは三角形が少ないので、効果が小さい可能性が高い)。
- 遠い物のアニメーションの更新間隔(`ModelAnimationComponent::SetUpdateInterval`)・遠いスプライトを描かない設定(`SpriteRenderer::SetMaxDrawDistance`)。設計書の「任意」の項目で、今回は作っていません(Sample側の `SetEnabled(false)` で代わりにできます)。
- 同じ物をまとめて1回で描く(インスタンス描画)。物が数百個ある場面では、LODより効く可能性があります。別の機能として提案する予定です。

---

## 38. デモ(Demoプロジェクト)

Engineの機能を「見て・触って分かる」ようにした、機能ごとの小さなシーン(デモ)を集めた**専用の実行ファイル(`Demo.exe`)**です。ゲーム(`Sample2D` / `Sample3D`)とは別のプロジェクトなので、ゲーム本体には一切影響しません(以前はサンプルのタイトル画面の「DEMO」ボタンから入っていましたが、Demoプロジェクトへ引っ越しました)。全体の設計は [`docs/DemoPlan.md`](docs/DemoPlan.md) にあります。

### 開き方

- ソリューションの `Demo` を右クリック →「スタートアッププロジェクトに設定」して、F5で実行する(§0の `Sample2D`・`Sample3D` と同じ)。
- または、ビルド済みの `x64\Debug\Demo.exe`(`Release\` も)をダブルクリックする。`Assets\` とシェーダーは、Engineのビルドで同じ場所にコピーされるので、そのまま動く。

### 画面の流れ

起動すると、すぐ**カテゴリ一覧**が出ます(タイトル画面はありません)。

```
カテゴリ一覧 ──ボタン──▶ 機能一覧 ──ボタン──▶ 各デモ
  Esc: アプリを終了        Esc: カテゴリ一覧へ     Esc: 機能一覧へ
```

- **カテゴリ一覧**(`DemoMenuScene`): `Core` `Render` `UI` `Render3D` `Physics` `Behavior` `Audio` `Data` の8個(Engineのフォルダに近い名前)。デモがまだ無いカテゴリがあれば、暗くて押せず「準備中」と出る(今は8個すべてにデモがある)。
- **機能一覧**(`DemoCategoryScene`): 選んだカテゴリの機能が並ぶ。説明の左に印(`[2D/3D]` 2Dと3Dの両方 / `[2D]` 2Dだけ / `[3D]` 3Dだけ / `[ALL]` 2Dと3Dの区別が無い)が付く。**今の側(2D/3D)の版が無い機能は、ボタンが暗くなって押せない**。
- **各デモ**: 左上の板に「デモの名前(と今の側の印)」「何のデモか」「操作方法」「使っているEngineのAPI」の4行、左下に「Back」ボタン(Escと同じ。カテゴリ一覧では「Quit」)、右下に2D/3Dの切り替えボタンが出る。
- シーンの切り替えは、すべて暗転(`FadeTransition`)。

### 2Dと3Dの切り替え

2D版と3D版の両方がある機能(Object・Transform・Math・Sprite・SpriteAnim・Camera・Particle・Tween・Coord・WorldUI・Visibility・Collider・Force・RigidbodySettings・Trigger・Query・Move・Homing・Vision・Path・MovingWall・Sound3D)は、デモの中で切り替えられます。

- **Tabキー**、または**右下のボタン**を押すと、その場で逆の側のデモへ切り替わる(暗転する)。
- 今の側(2Dか3Dか)は**アプリ全体で1つ**だけ覚えます。起動した時は**3D**です。「Particleの3D版を見ていて、Backで機能一覧へ戻り、別の機能を開く」時も、同じ3Dの側で開きます。**保存はしない**ので、アプリを閉じると3Dに戻ります。
- カテゴリ一覧と機能一覧でも、Tabか右下のボタンで側を選べる(画面は切り替わらず、機能一覧の暗いボタンが変わる)。
- 右下のボタンの文字は状態で変わる(ボタンの文字はArial固定で日本語が出ないので、英語):

| 文字 | 意味 |
|---|---|
| `To 2D` / `To 3D` | 逆の側の版へ切り替えられる |
| `3D only` / `2D only` | その機能は片方の側しか無い(暗くて押せない) |
| `Common` | 2Dと3Dの区別が無い機能(UIなど。暗くて押せない) |
| `Mode: 3D` / `Mode: 2D` | (メニューのみ)今選んでいる側。押すと入れ替わる |

### 今あるデモ

3Dのデモは、**右ボタンを押しながらマウスを動かす**と視点が変わり、**Q / E** でズームします(カーソルは固定しないので、ボタンも押せます。ゲーム本体のカーソル固定のカメラ(`CameraController`)とは別の、デモ用の`DemoCameraController`です。Cameraデモの3D版だけは、カメラそのものが題材なので、自分で動かすカメラを使います)。

| カテゴリ | 機能(印) | 学べること | 操作 | 使っているAPI |
|---|---|---|---|---|
| Core | **Object**(2D/3D。`ObjectDemo2DScene` / `ObjectDemo3DScene`) | 中央の物に、動きの部品(Component)を付ける・止める。部品が付いているかは `GetComponent` で調べる(付いていなければ `nullptr`)。クリックで小さな物を出し、3秒たつと `Destroy` で消える。**Engineには部品を外す関数は無い**ので、「止める」は `SetEnabled(false)` | 1: 回る部品(`SpinComponent`)を付ける / 2: 上下に動く部品(`BobComponent`)を付ける / 3: 付いている部品を全部止める・再開 / クリック: 物を出す(3Dは地面をクリック) / X: 出した物を全部消す | `Scene::CreateObject`、`GameObject::AddComponent` / `GetComponent` / `Destroy`、`Component::SetEnabled`(§1) |
| Core | **Transform**(2D/3D。`TransformDemo2DScene` / `TransformDemo3DScene`) | 大きい親と、小さい子。親を動かす・回す・大きくすると、子も一緒に動く。子の `position` は親から見た値(ローカル)で、画面の上の位置(ワールド)とは違うことが数字で分かる。親子を切ると、子はその場に残る。2Dは Z軸、3Dは 3つの軸で回せる | 矢印: 親を動かす / R: 親を回す(3Dは T で別の軸) / 1・2: 親の大きさ / SPACE: 親子をつなぐ・切る | `Transform`(`position` / `rotate` / `scale`)、`SetParent`、`GetWorldTransform`(2D)、`GetWorldMatrix`(3D)(§3) |
| Core | **Time**(共通。`TimeDemoScene`) | 時間の倍率を変えると、`dt` で回る物だけが遅くなり、`GetUnscaledDeltaTime` で回る物はそのまま。ポーズ中も後者は回る。`Timer`(1回だけ・繰り返し)の進みを帯で見る。デモを出る時に倍率を1に戻す | SPACE: スロー(0.3倍) / P: ポーズ / 1: 2秒で1回 / 2: 1秒ごと(もう一度押すと止める) / 3: 5秒で1回 | `Time::GetDeltaTime` / `GetUnscaledDeltaTime` / `GetTime` / `SetTimeScale` / `GetTimeScale` / `IsPaused`、`Timer`(§10) |
| Core | **Input**(共通。`InputDemoScene`) | キーボード(押している間・押した瞬間・離した瞬間)、マウス(位置・動いた量・ワールドの位置・ボタン)、ゲームパッド(スティック・トリガー・ボタン)の今の状態を表示する。**パッド(XInput)が無くても動き**、「未接続」と出る(つながっているかは、0.5秒ごとに調べる) | 何かキーを押す / マウスを動かす・クリック / パッドをつなぐ / F1: カーソルの固定(固定したままデモを出ないよう、出る時に解除する) | `Input::GetKeyDown` / `GetKeyPress` / `GetKeyUp`、`GetMousePosition` / `GetMouseDelta` / `GetMouseWorldPosition`、`GetPadLeftStick` / `GetPadRightStick` / `GetPadTrigger`、`SetCursorLocked` / `IsCursorLocked`(§0・§9) |
| Core | **Math**(2D/3D。`MathDemo2DScene` / `MathDemo3DScene`) | `MathUtil` の結果を、点と線で見る。**2D**: Lerp(マウスで割合を決める)/ Clamp / 距離・向き・角度(腕が目標の角度へ回って追う)/ 乱数(分布と円周上の点)。**3D**: Lerp / 内積・外積 / Normalize / MoveTowards(回る目標を追う点)。線は点を並べて描いている | 1〜4: 見る関数を選ぶ / 2D: マウスで値を動かす・SPACE(乱数の4でサイコロ)・クリック(数え直す) / 3D: 3 で矢印とW・Sでベクトルを動かす | `Math::Lerp` / `Clamp` / `Distance` / `Normalize` / `Dot` / `Cross` / `Length` / `MoveTowards` / `AngleToTarget2D` / `MoveTowardsAngle` / `DeltaAngle` / `Forward2D` / `RandomRange` / `RandomOnCircle`(§9) |
| Core | **Pool**(共通。`PoolDemoScene`) | 弾を出しては消す時、毎回作らずに `ObjectPool` で使い回す。「最初に用意した数・借りた回数・使用中・待機中・作った数」を表示し、プールの中身(緑=使用中、灰色=待機中)を並べる。連射しても、作った数は足りなくなった時だけ増える。`ObjectPool` はComponent(`PoolDemoController`)のメンバ | クリック・SPACE(押し続けると連射): 弾を出す。弾はマウスの方へ飛ぶ | `ObjectPool`(`Prewarm` / `Rent` / `Return` / `GetPoolSize` / `GetObjects`)(§8) |
| Core | **EventBus**(共通。`EventBusDemoScene`) | 「送る側」「受け取る側(スコア)」「記録係」は、お互いを知らず、イベント名だけでつながる。撃破(引数なし)とコイン取得(オブジェクトを1つ渡す)を送ると、スコアが増えて「スコアが変わった」が流れ、記録係が受信した順に一覧にする。ラムダは `this` を使うので、シーンの `Uninit` で `Clear()` する | Defeat / Coin ボタン(1・2キーも同じ) | `EventBus::Get().Subscribe` / `Publish` / `SubscribeObject` / `PublishObject` / `Clear`(§7) |
| Core | **Resources**(共通。`ResourcesDemoScene`) | 同じ画像を何枚置いても読み込みは1回で、`LoadTexture` のIDは全員同じ。画像を全部消しても、`UnloadUnused` を呼ぶまでは残る(使われている間は解放されない)。同じ音・モデルを2回借りても、IDや指す先が同じ。借りた音・モデルは、デモを出る時に返す | Add 10: 画像を増やす / Remove: 全部消す / Unload: 使われていない物を解放 / Sound x2・Model x2: 2回借りる / Release: 返す | `Resources::LoadTexture` / `ReleaseTexture` / `LoadSound` / `ReleaseSound` / `LoadModel` / `UnloadUnused`(§11) |
| Render | **Sprite**(2D/3D。`SpriteDemo2DScene` / `SpriteDemo3DScene`) | 画像のコマ(`SetSpriteIndex`)・色(`SetColor`)・大きさ(`scale`)・回転(`rotate.z`)を変える。**2D**は、下の赤(手前 z=-100)と青(奥 z=+100)を、UI(作った順に重なる。zは使わない)→ 2D(zの深度で前後が決まる。遠近なし)→ 3D(zで大きさも変わる)の3つの描き方に切り替えて、重なり方と大きさを比べる。緑は常にUI(z=+500)で、どのモードでも必ず手前に出る。**3D**は立方体の周りにビルボードのスプライトを置き、固定する軸(`SetBillboardLock`)の違いを、視点を回して見る(手前のスプライトは立方体を隠し、奥のスプライトは隠れる)。**Engineのスプライトには、左右反転の機能は無い**(`scale` が0以下だと画像の大きさになるため) | 1〜4: コマ・色・大きさ・回転の動きの入り切り / 2Dは5: UI・2D・3D / 3Dは右ドラッグで視点 | `SpriteRenderer`(`SetSpriteIndex` / `SetColor` / `SetWorldSpace` / `SetBillboardLock`)、`SetMode(SpriteMode)`、`Transform`(§4) |
| Render | **SpriteAnim**(2D/3D。`SpriteAnimDemo2DScene` / `SpriteAnimDemo3DScene`) | `player.png`(8列x2行)のコマを、時間で送る。ループする3つ(Walk・Scared・All)と、1回だけで止まる1つ(Eyes。止まると `IsFinished` が true になる)。下の段は同じクリップで、`SetRandomStart(true)` によって始まるコマをずらした5つ(上のWalkと見比べる)。3Dは、ビルボードのスプライトで同じ物を再生する | SPACE: 再生(Play)・停止(Stop。次のPlayは最初のコマから) / 1〜4: 速さ(0.5倍・通常・2倍・止める) / 3Dは右ドラッグで視点 | `SpriteAnimationComponent`(`AddClip` / `Play` / `Stop` / `SetSpeed` / `SetRandomStart` / `IsFinished`)(§17) |
| Render | **Camera**(2D/3D。`CameraDemo2DScene` / `CameraDemo3DScene`) | **2D**: 自分(おばけ)を `CameraFollowComponent` で2Dカメラが追う。なめらかさ・デッドゾーン(黄色い四角)・映す範囲(赤い枠。自分は枠の外にも出られる)・対象からのずらしを切り替える。**3D**: 3Dカメラを自分で動かす。三人称(後ろから)と一人称(自分の目から)をVで切り替える。どちらも `CameraShake` で揺らせる(2Dは画面のずれ、3Dはカメラから見た右・上へのずれ)。カーソルは固定しない(右ドラッグで向きを変えるので、Escもボタンもそのまま使える) | 2D: WASD(矢印)で動く / 1: なめらかさ / 2: デッドゾーン / 3: 範囲 / 4: ずらし / SPACE: 揺らす。3D: WASD(矢印)で向いている方へ動く / V: 一人称・三人称 / 右ドラッグ: 向き / Q・E: ズーム(三人称) / 1〜3: 揺れの強さ / SPACE: 揺らす | `CameraFollowComponent`(`SetSmoothTime` / `SetDeadZone` / `SetBounds` / `SetOffset`)、`CameraShake::Shake`、`Image::GetCamera3D`、`SetCamera3DAutoSync`(§5・§14) |
| Render | **ScreenEffect**(共通。`ScreenEffectDemoScene`) | 画面全体の演出を重ねる。Flash(白い光)・Vignette(赤い縁が3回点滅)・Fade(黒/白。暗転し終わったら、渡した関数で明転する)。`IsFading()` の状態と、Fade完了の関数が呼ばれた回数を表示する。演出は文字も隠す。Fadeはシーンをまたいで残るので、デモを出る時に明るい状態へ戻す | 1: Flash / 2: Vignette / 3: Fade(黒) / 4: Fade(白) | `ScreenEffect::Flash` / `Vignette` / `Fade`(`onComplete`) / `IsFading` / `Stop`(§15) |
| Render | **Transition**(共通。`TransitionDemoScene` と `TransitionDemoBScene`) | 演出を選ぶと、別のシーン(A: 2Dの背景 / B: 3Dの舞台)へ切り替わり、もう一度押すと戻る。Fade(黒・白)と SpriteTransition(丸・ワイプ。`transition_circle.png` `transition_wipe.png`)を見比べる。切り替え中は `Time::IsPaused()` が true になり、経過時間と回る箱が止まる。シーンBは機能一覧には出ない(Aから行く)。Escの戻る(黒のFade)も同じ仕組み | 1: Fade(黒) / 2: Fade(白) / 3: 丸 / 4: ワイプ | `SceneManager::ChangeScene<T>(FadeTransition(...))` / `(SpriteTransition(...))`、`SetSwitchFrame`、`Time::IsPaused`(§16) |
| Render | **Particle**(2D/3D。`ParticleDemo2DScene` / `ParticleDemo3DScene`) | 爆発(加算合成)・火花(重力)・煙(普通の合成。大きくなって消える)・花火(色が水色からピンクへ変わる)を、設定の違いで作り分ける。出しっぱなしの焚き火(`Play`)の粒の数も表示する。**3D**は同じ設定を `use3D = true` で3D空間に出す(粒はカメラの方を向く板で、手前の物に隠れる。3Dは画面に対して小さく見えるので、速さ・大きさを3倍にしている) | 1〜4: 種類を選ぶ / クリック: その場所で発射(3Dは、マウスの線が地面に当たる場所) / SPACEを押している間: スロー(`Time::SetTimeScale`) | `ParticleSettings`(`additive` / `gravity` / `startColor` `endColor` / `startSize` `endSize` / `use3D`)、`CreateEffect`、`Burst`、`Play`、`Input::GetMouseWorldPosition`、`Image::ScreenPointToRay3D`(§26) |
| Render | **Tween**(2D/3D。`TweenDemo2DScene` / `TweenDemo3DScene`) | 同じ時間で動く8個。違うのは動きの速さの変化(Ease)だけ。各行に、Easeが返す進み具合(0〜1)が出る(`OutBack`・`OutElastic`は0〜1をはみ出す)。**2D**は丸が右へ動き、位置・色・進み具合を同時に動かす。**3D**は立方体が奥へ動き、位置・大きさ・向き・色・進み具合を同時に動かす | SPACE: 全部を最初からやり直す(出だしの違いが見やすい) | `TweenComponent`(`MoveTo` / `ScaleTo` / `RotateTo` / `ColorTo` / `TweenValue` / `SetLoop(true, true)`)、`Ease::Linear` `InQuad` `OutQuad` `InOutQuad` `OutCubic` `OutBack` `OutElastic` `OutBounce`(§12) |
| Render | **Coord**(2D/3D。`CoordDemo2DScene` / `CoordDemo3DScene`) | **2D**: マウスの画面の位置を `ScreenToWorld2D` でワールドの位置にして、`WorldToScreen2D` で画面に戻す(`GetMouseWorldPosition` と同じ値)。クリックで印を置き、カメラを動かしても、印はワールドの同じ場所に見える。右の3つは奥行き(z)だけが違うスプライトで、zが違っても大きさ・画面の高さは同じに見える(2Dモードで描いている)。**3D**: 立方体の頭の上に、`WorldToScreen3D` で名前と画面の位置を出す(カメラの後ろは出さない)。マウスの位置から `ScreenPointToRay3D` で線を作り、`Physics::Raycast` で、マウスの下の物・距離・当たった場所を調べる。クリックで物を選ぶ(縁が光る) | 2D: マウス / クリック: 印を置く / R: 印を消す / WASD(矢印): 2Dカメラを動かす。3D: マウス / クリック: 物を選ぶ / 右ドラッグ: 視点 | `Image::WorldToScreen2D` / `ScreenToWorld2D` / `WorldToScreen3D` / `ScreenPointToRay3D`、`Input::GetMouseWorldPosition`、`Physics::Raycast`(§14・§23) |
| UI | **UI**(共通。`UiDemoScene`) | スライダーを動かすとゲージが変わる。Damage/Healボタンで、減った分が遅れて縮むHPゲージが動く。「Count UP」ボタンで回数が増える(文字は`TextRenderer`)。左右に動く丸には、文字が付いて動く。(画面の9か所の基準 `Anchor` は、下の Rect デモに分けました) | スライダーをドラッグ / ボタンをクリック | `ButtonComponent`、`SliderComponent`、`GaugeRenderer`(`SetValueSource` / `SetDelayedBar`)、`TextRenderer`、`SetAnchor`(§19・§20) |
| UI | **Rect**(共通。`RectDemoScene`) | 画面の9か所(`Anchor`)に、印(黄色の小さな四角=`GetAnchoredPosition`)と、印に角や辺を合わせた青い四角(`GetAnchoredCenter`)を置く。ドラッグできる大きさの違う3つの四角(`Image::DrawRect`)。画面の大きさ(`Graphics::width` / `height`)も表示する | 下の3つの四角をドラッグして動かす | `Image::DrawRect`、`Anchor`、`GetAnchoredPosition` / `GetAnchoredCenter`(§20) |
| UI | **Text**(共通。`TextDemoScene`) | 文字の大きさ・色・縁取り・太字・寄せ方(左・中央・右)・フォント(Arialと`LoadFont`で読んだNoto Sans JP)を並べる。キーで入力した文字を、`Text::Measure`で測った板の上に、4つのスタイルで出す。動く物に付いた文字(`TextRenderer`)は、`SetText`で中身を毎フレーム変える | A〜Z・SPACE: 文字を入力 / BackSpace: 消す / 1〜4: 入力した文字のスタイル | `Text::Draw` / `LoadFont` / `Measure`、`TextStyle`、`TextAlign`、`TextRenderer`(`SetText` / `SetOffset`)(§19) |
| UI | **WorldUI**(2D/3D。`WorldUiDemo2DScene` / `WorldUiDemo3DScene`) | 敵3体の頭の上に、HPゲージ(`GaugeRenderer`の`SetFollowTarget`)と、名前・HPの板(`WorldToScreen2D/3D`で画面の位置に直して`DrawRect`+`Text::Draw`)を出す。敵が動いても、UIはついていく。主人公が近づくと板が赤くなる。倒した敵は非表示になり、ゲージも名前も一緒に消える。3Dはカメラの後ろ(`z <= 0`)では描かない。主人公の頭の上の名前(`TextRenderer`)は、Mキーで大きさの決め方を切り替えられる(`Screen`=変わらない / `World`=3Dでは奥行きで変わる。2Dでは変わらない) | 矢印: 主人公を動かす / 1・2・3: 敵にダメージ / SPACE: 全回復 / M: 名前の大きさの決め方を切り替え / 3Dは右ドラッグで視点 | `Image::WorldToScreen2D` / `WorldToScreen3D`、`GaugeRenderer`(`SetFollowTarget` / `SetUse3DCamera`)、`TextRenderer`(`SetUse3DCamera` / `SetSizeMode` / `SetReferenceDistance` / `SetSizeRange`)、`Image::DrawRect`(§14・§19・§20) |
| Render3D | **Shapes**(3Dのみ。`ShapesDemoScene`) | 3列の見本。手前は**形**(立方体・粗い球・細かい球・カプセル・OBJ読み込み。ゆっくり回る)、真ん中は同じオレンジの**球5個**(標準・光沢(鋭い)・光沢(ぼんやり)・発光・縁の光)、奥は**立方体5個**(色・画像・半透明・光を無視・枠線)。球と立方体は、マテリアル(`Material`)だけが違う。各見本の上に、名前とマテリアルの項目名が出る。球の分割数(`rings` / `segments`)が小さいと角ばるのが分かる。(以前 Light デモにあったマテリアルの見本を、ここへ分けました) | 1・2・3: 列の表示オン/オフ / 4: 形を回す / 右ドラッグ: 視点 / Q・E: ズーム | `Mesh::CreateCube` / `CreateSphere(rings, segments)` / `CreateCapsule` / `LoadOBJ`、`MeshRenderer`、`Material`(`specularStrength` `shininess` `emissiveColor` `rimStrength` `lit` `depthWrite` `outlineWidth`)(§4・§30) |
| Render3D | **Light**(3Dのみ。`LightDemoScene`) | 白い球5個と白い立方体5個(全部同じ材質)に、平行光源(太陽)・左右に動くオレンジの点光源・白いスポットライトを当てて、光の違いを見比べる | 1: 平行光源(太陽) / 2: 点光源 / 3: スポットライト のオン・オフ / 矢印キー: 太陽の向きと高さ | `Image::GetLight`、`LightComponent`(`Point` / `Spot` / `SetSpotAngle`)(§31) |
| Render3D | **Shadow**(3Dのみ。`ShadowDemoScene`) | 太陽がゆっくり回り、影が動く。球(影を落とす)・立方体(落とさない)・板と下の箱(受ける)・壁。スポットライト(左右に動く白い玉)も影を落とす | 1: 球の`castShadow` / 2: 箱の`receiveShadow` / 3: スポットの影 / 4: 枠線 / 5: 影の縁のぼかし(`pcfRadius`) / 矢印キー: 太陽を手で動かす(SPACEで自動に戻す) | `Shadow::GetSettings`、`Material`(`castShadow` `receiveShadow` `outlineWidth`)、`LightComponent::SetCastShadow`(§32) |
| Render3D | **Outline**(3Dのみ。`OutlineDemoScene`) | 左: 立方体の外側に、画面上で一定の太さの**枠線**(黄色)を描く(遠くても近くても同じ太さ)。真ん中: 壁の後ろに隠れた球に、**壁越しの枠線**(`seeThrough`。水色)を出す。OFFにすると、壁の後ろの球の枠線は見えなくなる(壁より後に描くよう、球の`drawPriority`を大きくしてある)。右: 暗い球の縁(視線に斜めの所)を内側で光らせる**縁の光**(`SetRimLight`) | 1: 枠線 / 2: 壁越し / 3: 縁の光 のオン・オフ / 上下: 枠線の太さ / 左右: 縁の光の強さ / 右ドラッグ: 視点 | `MeshRenderer::SetOutline(色, 太さ, seeThrough)` / `SetRimLight(色, 強さ)`、`Material`(`outlineWidth` `outlineColor` `outlineSeeThrough` `rimColor` `rimStrength`)(§33) |
| Render3D | **Terrain**(3Dのみ。`TerrainDemoScene`) | 高さの画像(`heightmap.png`)から作った丘。黄色い玉を動かして、その下の地面の高さ(`GetHeightAt`)・向き(`GetNormalAt`)・地形の中か(`Contains`)を表示する | 矢印キー: 玉を動かす | `Terrain::LoadFromImage`、`TerrainRenderer`(`GetHeightAt` / `GetNormalAt` / `Contains`)(§34) |
| Render3D | **Water**(3Dのみ。`WaterDemoScene`) | 丘の谷の池の水面。色・波の高さ・波の速さを変えられる。赤い箱は、波の高さに合わせて浮かぶ | 1・2: 波の高さ / 3・4: 波の速さ / 5: 水の色 | `WaterRenderer`(`WaterSettings` / `GetSettings` / `GetHeightAt`)(§35) |
| Render3D | **Model**(3Dのみ。`ModelDemoScene`) | 同じFBX(`TestParts.fbx`。大きな箱 Base の上に小さな箱 Top が乗った2パーツ)を2体置く。1体は動かし、もう1体は見本。パーツを**名前で探して**(`GetNodeTransform("Top")`)回す・上下・大きさを変えると、そのパーツだけが動く。モデル全体は、置いた物の `Transform` で回す・大きさを変える。読み込んだモデルの情報(パーツ数・親子・位置・半径)も表示する。2体の `Model`(データ)は共有され、パーツの動きは1体ごと | 1: Topを回す / 2: Topを上下 / 3: Topの大きさ / 4: 全体を回す / 5: 全体の大きさ / 右ドラッグ: 視点 | `ModelRenderer`(`GetNodeTransform` / `GetModel`)、`Model`(`GetNodeCount` / `GetNode` / `GetBoundingRadius`)(§27) |
| Render3D | **SkinAnim**(3Dのみ。`SkinAnimDemoScene`) | 左: 箱(`TestAnim.fbx`。クリップ Spin と Bob)。真ん中: 骨2本で曲がる腕(`TestSkin.fbx`。クリップ Bend)をずっと再生。右: 同じ腕を1回だけ再生(終わると `IsFinished` が true になって、最後の形で止まる)。箱のクリップは、すぐ切り替える(`Play`)か、なめらかに切り替える(`CrossFade` 0.6秒)かを見比べる。速さは3つ同時に変わる。枠線は、骨で曲げた形に付く | 1: Spin / 2: Bob / SPACE: CrossFade / 3: 1回だけの腕を再生し直す / 4: 枠線 / 上下: 速さ / 右ドラッグ: 視点 | `ModelAnimationComponent`(`Play` / `CrossFade` / `SetSpeed` / `IsFinished` / `GetClipNames`)、`ModelRenderer`(§27) |
| Render3D | **LOD**(3Dのみ。`LodDemoScene`) | 225本(15x15)の柱が並ぶ場所。カメラから遠い柱は、描かない(3000)・影を落とさない(2000)・枠線を描かない(1500)。LODのオン/オフと距離の倍率を変えて、描画命令の数・三角形の数・距離で省いた数(`RenderStats`。前のフレームの数)が変わるのを見る。柱は三角形が多いカプセル。影・枠線も切り替えられる | 1: LODのオン/オフ / 上下: 距離の倍率 / 2: 影 / 3: 枠線 / 右ドラッグ・Q・E: 視点を変えて数の変化を見る | `Lod::SetEnabled` / `SetGlobalScale`、`MeshRenderer::SetLod`、`RenderStats::GetDrawCallCount` / `GetDrawTriangleCount` / `GetShadowDrawCount` / `GetOutlineDrawCount` / `GetLodCulledCount` ほか(§37) |
| Render3D | **Visibility**(2D/3D。`VisibilityDemo2DScene` / `VisibilityDemo3DScene`) | 広い世界にたくさん置いた物(2Dは箱99個、3Dは球と立方体81個)が、カメラに映っているかを `Visibility` で調べて数える。右の地図(緑=映っている、灰色=映っていない、黄色=カメラの範囲)で、映る範囲が分かる。1キーの `SetFrustumCulling` をONにすると、映っていない物は描かれず、省いた数(`GetCulledCount`。前のフレームの数)が増える(OFFだと0)。**2D**は円(`IsCircleInView2D`)と四角(`IsRectInView2D`)、**3D**は球(`IsSphereInView3D`)と箱(`IsBoxInView3D`)で調べる | 1: 描かない設定のオン/オフ / WASD(矢印): 2Dはカメラ、3Dは見る点を動かす / 3Dは右ドラッグ: 向き、Q・E: ズーム | `Visibility::IsCircleInView2D` / `IsRectInView2D` / `IsSphereInView3D` / `IsBoxInView3D` / `GetCulledCount`、`SpriteRenderer` / `MeshRenderer` の `SetFrustumCulling`(§28) |
| Physics | **Collider**(2D/3D。`ColliderDemo2DScene` / `ColliderDemo3DScene`) | 当たり判定の形(2D: 円・四角・カプセル / 3D: 球・箱・カプセル)を並べる。床の上の固定の見本(`BodyType::Static`)、左右に往復する台(`Kinematic`。`Transform`を動かして、動く物を押す)、クリックで出す動く物(`Dynamic`)。F1で当たり判定の枠線(緑=通常、黄=トリガー)。**2D**は重力が無い上から見た世界なので、壁で囲んで勢いをつけて出す(重力オフ・Z固定)。**3D**はマウスの下の地面の上から落とす。出せる数は30(超えたら古い物から消える) | クリック: 選んだ形を出す / 1・2・3: 形(円・四角・カプセル) / F1: 枠線 / C: 全部消す | `CircleRigidbodyComponent` `SquareRigidbodyComponent` `Capsule2DRigidbodyComponent` / `SphereRigidbodyComponent` `BoxRigidbodyComponent` `CapsuleRigidbodyComponent`、`BodyType`、`RigidbodyComponent::SetDebugDrawEnabled`(§6・§23・§29) |
| Physics | **Force**(2D/3D。`ForceDemo2DScene` / `ForceDemo3DScene`) | 軽い箱(質量100)と重い箱(質量500)を、同じ値で押す。`Force`・`Impulse`は質量で割られるので重い箱は動きにくく、`Acceleration`・`VelocityChange`は質量を無視して同じ速さになる。`Force`・`Acceleration`は押し続ける力(SPACEを押している間)、`Impulse`・`VelocityChange`は一瞬(押すたびに1回)。`SetVelocity`は今の速さを上書きし、`AddTorque`は回す(重い箱ほど回りにくい)。箱は横にだけ動くよう、Y・Zを固定してある(3Dは重力も切ってある) | 1〜4: 力の種類 / SPACE: 力をかける / V: 速さを上書き / T: 回す / R: 元の位置へ | `RigidbodyComponent::AddForce` / `AddTorque` / `SetVelocity` / `GetVelocity` / `SetMass` / `SetDrag`、`ForceMode`(§23) |
| Physics | **RigidbodySettings**(2D/3D。`RigidbodySettingsDemo2DScene` / `RigidbodySettingsDemo3DScene`) | 同じ箱を2つ置く(A=ふつう、B=設定を変えた物)。同じ力(`Impulse`+`AddTorque`)で蹴ると、質量(重いと動きにくい)・空気抵抗(すぐ止まる)・重力(2DはBだけ落ちる、3DはBだけ浮く)・回転固定(回らない)・位置固定(その軸には動かない。もう一度5で軸が変わる。2DはXとY、3DはXとYとZ)の違いが分かる。AとBの今の設定も表示する | 1: 質量 / 2: 抵抗 / 3: 重力 / 4: 回転固定 / 5: 位置固定 / SPACE: 蹴る / R: 元に戻す | `RigidbodyComponent::SetMass` / `SetDrag` / `SetUseGravity` / `SetFreezeRotation` / `SetFreezePositionX` `Y` `Z`、`AddForce`(§23) |
| Physics | **Trigger**(2D/3D。`TriggerDemo2DScene` / `TriggerDemo3DScene`) | 動かせる球を、黄色い丸(トリガー=すり抜ける)と青い箱(ぶつかる)に近づける。トリガーは `OnTriggerEnter`(入った瞬間に1回)・`OnTriggerStay`(中にいる間は毎フレーム)・`OnTriggerExit`(出た瞬間に1回)、箱は `OnCollisionEnter` / `Stay` / `Exit` が呼ばれる。その回数を数えて表示する(数える役は `TouchCounterController`)。トリガーは半透明の黄色(濃さ0.6。暗い背景でも見える濃さ)で、球が中にいる間は0.95に濃くなる。**速く押し付けると跳ね返って `Enter` が何回も呼ばれる**ので、球はゆっくり動かす。**関数の名前は、2D・3D共通で `○○`**(Engineの名前。旧 `○○2D`) | WASD・矢印: 球を動かす / R: 回数を0に戻す | `Component::OnTriggerEnter` / `OnTriggerStay` / `OnTriggerExit` / `OnCollisionEnter` / `OnCollisionStay` / `OnCollisionExit`、`isTrigger`(§6・§23) |
| Physics | **Query**(2D/3D。`QueryDemo2DScene` / `QueryDemo3DScene`) | 物理への問い合わせを目で見る。**Raycast**: 出発点(橙)からマウスへ線を飛ばし、最初に当たった物・距離・当たった場所・面の向き(赤い矢印)を出す。**OverlapSphere / OverlapBox**: マウスの周りの球(2Dは円)・箱に入っている物を探し、見つかった物を光らせる(2Dは黄色い枠、3Dは枠線)。調べられる物は、タグ Target(青・緑)と Decoy(橙・赤)の動かない物と、黄色い半透明のトリガー(タグ Target)。4でタグの絞り込み(なし→Target→Decoy)、5でトリガーも探すか(`Ignore` / `Include`)。3Dは、タグを指定しないと地面も見つかる | マウス: 線の向き・範囲の位置 / 1: Raycast / 2: OverlapSphere / 3: OverlapBox / 4: タグ / 5: トリガー | `Physics::Raycast` / `OverlapSphere` / `OverlapBox`、`RaycastHit`、`QueryTriggers`(§23) |
| Physics | **TerrainCollider**(3Dのみ。`TerrainColliderDemoScene`) | 丘を箱で歩いて登る。丘の見た目と同じデータの当たり判定なので、坂道をそのまま登れる。足元の地面の高さを表示する | WASD: 歩く(R: スタート地点へ。地形の外へ落ちても戻る) | `TerrainRigidbodyComponent`(§34)、`TerrainRenderer::GetHeightAt`、`RigidbodyComponent::SetVelocity` |
| Behavior | **Move**(2D/3D。`MoveDemo2DScene` / `MoveDemo3DScene`) | 動きの部品の基本。`ChaseComponent`(`MoveComponent` の仲間)は、目的地へ一定の速さでまっすぐ向かい、止まる距離(`SetStopDistance`)より近づくと止まる。青は `SetTarget` で、マウスに付いてくる青い目印を追う。橙は `SetTargetPosition` で、クリックした場所(橙の目印)へ向かう。目印の周りの円が止まる距離。途中の壁にぶつかると、まっすぐ進むだけの動きはそこで止まる(避けて歩くのは Path デモ)。動く物は `Dynamic`(`MoveComponent` は、Dynamic なら `SetVelocity` で動かすので壁に当たる)。2Dは `MovePlane::XY`、3Dは `XZ` | マウス: 目標を動かす / クリック: 橙の目的地 / ↑↓: 速さ / ←→: 止まる距離 / R: 戻す | `ChaseComponent`(`SetTarget` / `SetTargetPosition` / `SetSpeed` / `SetStopDistance` / `SetPlane` / `HasArrived`)(§13) |
| Behavior | **Homing**(2D/3D。`HomingDemo2DScene` / `HomingDemo3DScene`) | 同じ目標(マウスに付いてくる黄色い目印)を、3つの追いかけ方で追う。青 `ChaseComponent`: 一定の速さでまっすぐ。橙 `AccelerateChaseComponent`: 加速して(空気抵抗あり)勢いで行き過ぎ、戻ってくる。緑 `HomingComponent`: 1秒に曲がれる角度に限りがあり、最初に向いていた上へ飛び出してから、大回りで追う。通った道を点で残し(古いほど小さい)、白い線が今進んでいる向き。3つは `Kinematic`(`Transform` を直接動かす。お互いにすり抜けるので、動きだけを比べられる) | マウス: 目標 / 1・2・3: 青・橙・緑の入り切り / ↑↓: 緑の曲がる速さ / ←→: 橙の加速 / R: 最初の位置へ | `AccelerateChaseComponent`(`SetAcceleration` / `SetMaxSpeed` / `SetDrag`)、`HomingComponent`(`SetTurnSpeed` / `SetFaceMovement`)、`ChaseComponent`(§13) |
| Behavior | **Vision**(2D/3D。`VisionDemo2DScene` / `VisionDemo3DScene`) | 見張り役(動かない)の視野(扇形)の中にターゲットがいて、壁(タグ "Wall")にさえぎられていなければ「見える」。見えると見張り役が赤くなる。扇形の線(`SetDebugDrawEnabled`)は、見えている時は赤、見えていない時は緑。「扇形の中か(`IsInCone`)」と「見えるか(`CanSee`)」を別々に表示する。壁のタグを外す(`SetObstacleTag("")`)と、壁を考えなくなる。3Dは目の高さ(`SetEyeOffset`)を上げると、低い壁の上から見える | 矢印: ターゲット / A・D: 向き / W・S: 視野の角度 / 1・2: 距離 / SPACE: 自動で回る / 5: 壁のタグ / 3Dは4: 目の高さ / F1: 扇形の線 / R: 戻す | `VisionComponent`(`SetFieldOfView` / `SetRange` / `SetPlane` / `SetFacing` / `SetEyeOffset` / `SetObstacleTag` / `IsInCone` / `CanSee` / `SetDebugDrawEnabled`)(§28) |
| Behavior | **Path**(2D/3D。`PathDemo2DScene` / `PathDemo3DScene`) | 壁で作った通り道(下から伸びる壁と、上から伸びる壁)を、緑は `PathFollowComponent` が格子(`NavGridComponent`)の上で経路を探して、壁を避けて歩く。赤は `ChaseComponent` でまっすぐ向かうだけなので、最初の壁で止まる。クリックで目的地を動かす。格子は壁を置いた後に作り、壁(タグ "Wall" の `Static`)は最初のフレームの後に自動で読まれる(`IsBaked`)。F1で、通れないマスの輪郭(赤)と今の経路(緑)の線を出す。格子のマスの数・焼いた時間・経路の点の数も表示する | クリック: 目的地 / ↑↓: 速さ / ←→: 探し直す間隔 / F1: 格子と経路の線 / R: 戻す | `NavGridComponent`(`IsBaked` / `GetLastBakeMilliseconds` / `SetDebugDrawEnabled`)、`PathFollowComponent`(`SetNavigation` / `SetRepathInterval` / `HasPath` / `GetPath`)(§36) |
| Behavior | **MovingWall**(2D/3D。`MovingWallDemo2DScene` / `MovingWallDemo3DScene`) | 真ん中の壁に、すぐ通れるドア(`Kinematic` の動く壁)と、遠回りの広いすき間がある。ドアが閉じると、緑は遠回りの道を通る。格子は `SetAutoRebakeInterval`(0.25秒ごと)で、動く壁を自動で焼き直す。自動の焼き直しを切ると、`RequestRebake` で焼くまで格子は古いまま(ドアが開いても経路は変わらず、閉まったドアにぶつかる)。物理の無い柵は `NavGrid::SetBlockedBox` で格子にだけ登録する(最大6個)。道を全部ふさぐと、経路が無くなって緑は止まる。緑がドアの通り道にいる間は、ドアを閉められない(挟まるのを防ぐ) | 1: ドアを開け閉め / 2: クリックの意味(目的地・柵) / 3: 自動の焼き直し / 4: 今すぐ焼く / C: 柵を消す / F1: 格子と経路の線 / R: 戻す | `NavGridComponent`(`SetAutoRebakeInterval` / `RequestRebake` / `GetLastBakeMilliseconds`)、`NavGrid`(`SetBlockedBox` / `GetVersion`)、`PathFollowComponent`(§36) |
| Behavior | **TerrainPath**(3Dのみ。`TerrainPathDemoScene`) | 丘の高さ・坂を取り込んだ格子(`NavGridComponent::SetTerrain`)の上を、緑が歩く。登れる坂の角度(`NavGridSettings::maxSlopeDegrees`)で、通れないマス(急な坂)の数と、歩く道が変わる(15度は大回り、20度は少し避ける、30度はほぼまっすぐ。この丘の一番急な所は約32度なので、40度以上だと何も通れなくならない)。角度は格子を作る時に決まるので、変える時は格子を作り直して古い方を `Destroy` し、`SetNavigation` で渡し直す(焼くのに少し時間がかかる)。クリックの位置は、タグ "Terrain" の丘への `Raycast` で調べる | クリック: 目的地 / 1・2・3: 登れる坂(15・20・30度) / ↑↓: 速さ / F1: 格子と経路の線 / R: 戻す / 右ドラッグ: 視点 | `NavGridSettings`(`maxSlopeDegrees`)、`NavGridComponent::SetTerrain`、`PathFollowComponent`、`TerrainRenderer::GetHeightAt`、`Physics::Raycast`(§34・§36) |
| Audio | **Sound**(共通。`SoundDemoScene`) | BGM(`TitleBGM.wav`。グループ`BGM`・ループ・位置を使わない)と、SE(`shot.wav`は`Audio::PlayOneShot`で重ねて鳴らす。`hit.wav`は`AudioSourceComponent`で鳴らし直すと最初から)。音量は**グループごと**(スライダー2本=`AudioGroup::BGM`と`SE`)で、BGM自体の`SetVolume`とかけ算になる。**フェードアウト/インは、Engineに専用の関数が無いので、`TweenValue`で`SetVolume`を動かして作る**(2秒)。BGMの音量の棒が動くのが見える。SEのスライダーは、離すとSEが1回鳴って聞き比べられる。BGMは、デモを出る時に止める(次の画面で鳴り続けない)。**音声デバイスが無くても落ちず**、鳴らせない時は「鳴らせませんでした」と出る | 1: BGM 入/切 / 2: SE(shot) / 3: フェードアウト / 4: フェードイン / 5: SE(hit) / スライダー: 音量(右のボタンも同じ) | `AudioSourceComponent`(`Play` `Stop` `SetVolume` `SetLoop` `SetGroup` `SetSpatial`)、`Audio::PlayOneShot`、`SetGroupVolume` / `GetGroupVolume`、`AudioGroup`、`TweenComponent::TweenValue`、`SliderComponent`(§24) |
| Audio | **Sound3D**(2D/3D。`Sound3DDemo2DScene` / `Sound3DDemo3DScene`) | 位置で聞こえ方が変わる音。橙の玉(音源。`AudioSourceComponent`)が回りながら鳴り、青の玉(聞く側。`AudioListenerComponent`)を動かす。**近いほど大きく、左にあれば左から聞こえる**。緑の輪=ここまでは最大音量、黄の輪=ここで聞こえなくなる(`SetDistance`)。`SetSpatial(false)`にすると、どこにいても同じ音量で真ん中から聞こえる。SPACEは、その場所に`Audio::PlayOneShotAt`で1回だけ鳴らす(鳴らした後は動かない)。下の棒は、音源が聞く側の左右どちらにあるかの目安。**2D版も作れた**(`AudioListenerComponent`は2Dでも使え、向きは画面の上が正面で固定)。**3D版**は`SetUse3DCamera(true)`で3Dカメラの向きで聞く(右ドラッグで視点を回すと、左右が変わる)。**ヘッドホンで聞くと分かりやすい**。音源の音は、このデモのために自動で作った`Assets/Demo/DemoTone.wav`(下の「音とデータのデモの補足」) | WASD: 聞く側を動かす(3Dはカメラの向きが前) / 1: 鳴/止 / 2: 位置の効き(ON/OFF) / ↑↓: 聞こえなくなる距離 / ←→: 回る速さ / SPACE: その場所に1回 / (3D)右ドラッグ: 視点 | `AudioSourceComponent`(`SetSpatial` `SetDistance` `SetLoop`)、`AudioListenerComponent`(`SetUse3DCamera`)、`Audio::PlayOneShotAt`(§24) |
| Data | **Csv**(共通。`CsvDemoScene`) | デモ専用の表(`Assets/Demo/DemoTable.csv`。Name・Hp・Speed・Memoの3行)を読んで、画面に表にする。列の名前で取り出す(`GetString` / `GetInt` / `GetFloat`。`Speed`の`85.5`は`GetFloat`だから小数で出る)。`FindRow`で名前から行を探し、`HasColumn`で列の有無を調べる(列が消えると、見出しが赤くなって「列なし」)。**ホットリロード**: 0.5秒ごとに`IsFileUpdated`で調べ、ファイルが保存し直されたら`Reload`する。読み直すと表の枠が1秒緑に光る。読めない時は赤い文字で知らせる。**ファイルには何も書かない**(読むだけ)。サンプルの`Enemies.csv` / `Waves.csv`は読まない。**編集のしかたは、下の「音とデータのデモの補足」** | メモ帳などで画面に出ているファイルを編集して保存(ゲームは止めない) / R: 今すぐ読み直す | `Csv`(`Load` `IsValid` `GetRowCount` `GetColumnCount` `HasColumn` `GetString` `GetInt` `GetFloat` `FindRow` `IsFileUpdated` `Reload`)、`Timer`(§19) |
| Data | **Save**(共通。`SaveDemoScene`) | `SaveData`に、整数(`Demo_Count`)・小数(`Demo_Ratio`)・日本語の文字(`Demo_Name`)を書いて、ファイルに保存・読み込みする。**保存してアプリを閉じても、開き直してこの画面に来ると同じ値が出る**。「未保存の変更があります」の印、`DeleteKey`で3つを消す、スロット1〜3(別のファイル)の切り替えもある。保存先(`%LOCALAPPDATA%\3D_FreamWork\save1.txt` など)は画面に出る。**サンプルのセーブは壊さない**: スロット0(サンプルのハイスコアなど)は使わず、キーも`Demo_`で始まる物だけ(下の「音とデータのデモの補足」) | Z: 整数+1 / X: 小数+0.1 / C: 名前を変える / S: 保存 / L: 読込 / D: キーを消す / 1・2・3: スロット(右のボタンも同じ) | `SaveData`(`SetInt` `GetInt` `SetFloat` `GetFloat` `SetString` `GetString` `HasKey` `DeleteKey` `Save` `Load`)(§18) |

(音のデモの`Voice`(再生口を直接使う`StartVoice`など)は作っていません。理由は下の「音とデータのデモの補足」。)

### 音とデータのデモの補足(Audio / Data)

**Csv: 表の編集のしかた(ホットリロードを試す)**

1. Csvデモを開くと、画面の上の「編集するファイル」に、読んでいるファイルの場所が出る(ふつうは `3D_FreamWork\x64\Debug\Assets\Demo\DemoTable.csv`。Releaseなら `x64\Release\...`)。これは**ビルドの時に `3D_FreamWork\Assets\Demo\DemoTable.csv` からコピーされたファイル**(Engineの `CopyRuntimeFiles` が `Assets` ごとコピーするので、`Assets\Demo\` も届く)。
2. そのファイルをメモ帳などで開き、`Hp` の数字や `Memo` を書き換えて**保存**する。ゲームは止めなくてよい。0.5秒以内に表が変わり、枠が緑に光る(すぐ見たい時は R)。行を足す・`Hp` の列を消す(見出しが赤くなる)・ファイルを消す(表は残り、消えている間は読み直さない)も試せる。
3. 元の `3D_FreamWork\Assets\Demo\DemoTable.csv` を編集した時は、**次のビルド(F5)で出力先へコピーされる**(`xcopy /d` は、元のファイルの方が新しい時だけコピーする)。出力先を直接編集した後に、元のファイルを触らずにビルドしても、編集は上書きされない。元の内容に戻したい時は、出力先の `DemoTable.csv` を消してからビルドする。
4. Excelで保存する時は「**CSV UTF-8(コンマ区切り)**」を選ぶ(日本語が文字化けするため)。列の名前(1行目)は半角英数字。

このデモは**読むだけで、ファイルを書き換えません**。サンプルの `Assets/Data/Enemies.csv` / `Waves.csv` は読まず、デモ専用の `DemoTable.csv` だけを使います。

**Save: サンプルのセーブを壊さない決まり**

- `SaveData` は、起動時に自動でスロット0を読み、終了時に「書き換えていれば、最後に読み書きに成功したスロット」へ自動で保存します。サンプルのハイスコアなどはスロット0(`save0.txt`)にあります。
- Saveデモは、**スロット1〜3だけ**を使います(最初にスロット1を読み込む)。スロット0は読み書きしないので、サンプルのセーブは変わりません(サンプルのキーはメモリから消えますが、`save0.txt` には触りません)。デモを開いただけで、何も書き換えなければ、ファイルは作られません。
- キーは `Demo_Count` / `Demo_Ratio` / `Demo_Name`(`SaveData` のキーに使える文字は英字・数字・`_` だけなので、`demo.` ではなく `Demo_` にしました)。スロットのファイルは `%LOCALAPPDATA%\3D_FreamWork\save1.txt`〜`save3.txt`(画面にも出る)。消したい時は、そのファイルを消すか、`D` でキーを消してから `S` で保存する。
- スロットを読めなかった時(ほかのアプリが開いている、など)は、そのスロットを書き換えない(サンプルのデータを別のスロットに写さないため)。

**Audio: 素材と決めたこと**

- BGMは、サンプルのタイトルと同じ `Assets/TitleBGM.wav`、SEは `Assets/Sounds/shot.wav` と `hit.wav` を使います(新しい素材は足していません)。
- Sound3Dだけ、`Assets/Demo/DemoTone.wav`(2秒・44100Hz・16bit・**モノラル**。220Hzの音を約0.4秒ごとに強弱をつけたもの。最初と最後が0でつながるので、ループしてもプチッと鳴らない)を、このデモのために**自動で作りました**(外のサイトからのダウンロードや、既存の曲の流用はしていません)。位置で聞こえ方を変える音は、モノラルの音でないと左右が出ないため、既存のステレオの音は使えませんでした。同じ名前・同じ形式のwavに差し替えてもかまいません。
- 音の確認は耳で行う必要があります(ヘッドホン推奨)。音声デバイスが無い環境でも、`Play` が何もしないだけで落ちません(Soundデモは「鳴らせませんでした」と表示)。
- **`Voice`(再生口を直接使う `LoadWave` / `StartVoice` / `UpdateVoice` / `StopVoice`)のデモは作りません**。これらは `Audio.h` に「Engine内部用(ゲーム側からは呼ばない)」と書かれた関数で、ゲームが使う入口は `AudioSourceComponent` と `PlayOneShot` / `PlayOneShotAt` です(Soundデモ・Sound3Dデモで見られる)。`Resources::LoadSound` の「同じ音は1回だけ読み込む」は、Coreの `Resources` デモで見られます。同時に鳴らせる数は32個まで(`Audio::maxVoices`)、という説明は、`Audio.h` の説明のとおりです。
### 仕組み(`DemoRegistry`)

```
Demo/main.cpp          ウィンドウの作成とEngineの初期化 → 最初のシーン(DemoMenuScene)
Demo/Game/Scenes/      Common: DemoRegistry(デモの一覧と、今の状態), DemoMenuScene(カテゴリ一覧), DemoCategoryScene(機能一覧)
                       Core / Render / UI / Render3D / Physics / Behavior / Audio / Data: 各デモのシーン
Demo/Game/Objects/     Common: 全デモで使う部品(DemoGuide, DemoButton, DemoModeButton, DemoMenu, DemoCategoryMenu,
                               DemoBackground, DemoSkybox, DemoGround, DemoCamera, DemoLights, DemoWalker,
                               DemoStage3D=3Dの舞台(空・地面・カメラ)をまとめて作る,
                               DemoBody=物理の剛体付きの物を作る(CreateDemoBody2D / CreateDemoBody3D), DemoWall=壁を作る)
                       Core / Render / UI / Render3D / Physics / Behavior / Audio / Data: 各デモの部品
                       (Behavior の DemoAgent=動く物・目印・壁(タグ "Wall")・競技場・経路探索の格子を作る)
Demo/Game/Components/  Common: DemoGuideController, DemoModeButtonController, DemoCategoryMenuController,
                               DemoCameraController, DemoWalkerController, BackgroundController, GroundController, SkyboxFollowComponent,
                               DemoTextStyle(デモの文字の見た目を作る関数), DemoMarker(点・線・文字の印を描く関数),
                               DemoMouseGround(マウスの下の地面を求める関数), DemoScreenFixController(2Dカメラが動いても、ボタンを画面の同じ場所に出す),
                               DemoStatus(説明の板の下に状態の板を出す関数), DemoBodyReset(動く剛体を元の位置へ戻す関数),
                               DemoPlane(動く平面の道具: 2D=XY・3D=XZ の座標の変換、マウスの下の位置、標準の競技場)
                       Core / Render / UI / Render3D / Physics / Behavior / Audio / Data: 各デモの動き
```

- **`DemoRegistry.cpp` の `GetDemoList()` が、機能の一覧を1か所に持っています**。1機能につき1行で、`DemoInfo` は `{ カテゴリ, ボタンの名前, 一言説明, 2D版を開く関数, 3D版を開く関数 }`。**`nullptr` かどうかが「2Dあり/3Dあり」の印**になるので、印を別に持ちません(書き間違えない)。2Dと3Dの区別が無い機能は、両方に同じ関数を入れます。
- 開く関数は `&OpenScene<○○DemoScene>`(`SceneManager::ChangeScene<T>(FadeTransition())`を呼ぶ小さなテンプレート関数)です。`ChangeScene`はテンプレートなので、シーンの型ごとに関数を作って、関数ポインタで一覧に持たせています。
- 今の状態(2Dか3Dか・今のカテゴリ・最後に開いたデモ)は、`DemoRegistry.cpp` の中の名前の無い名前空間の `static` 変数に持ち、`GetDemoMode()` / `SetDemoMode()` などで読み書きします(Engineの`Image`などと同じ書き方。`Get()`のシングルトンは作らない)。
- 画面の出入り口も同じファイルにあります: `OpenDemoMenu()`(カテゴリ一覧へ)、`OpenCategory(カテゴリ)`、`OpenCurrentCategory()`(デモの「Back」)、`OpenDemo(DemoInfo)`、`SwitchDemoMode()`(Tabとボタン)、`QuitDemo()`(アプリを終了)。
- 各画面の説明の板・Back(Quit)ボタン・2D/3Dボタン・Esc/Tabは、`CreateDemoGuide(scene, { 名前, 何のデモか, 操作, API })` の1行で付きます(デモ本体。カテゴリ一覧は第3引数に `DemoScreen::Top`、機能一覧は `DemoScreen::List`)。戻る先は画面で決まります。
- 2Dと3Dは、**別のシーンのクラス**(`○○Demo2DScene` / `○○Demo3DScene`)にしています。1つのシーンの中で出し入れするより、シーンを切り替える方が、古いシーンの`Uninit`で確実に後片付けできるためです。部品と動きは、できるだけ共有して(`CreateEaseRunner(..., DemoMode)` など)、違いは引数で出します。

### 新しいデモの追加方法

1. `Game/Scenes/<カテゴリ>/○○DemoScene.h/.cpp` を作る(2Dと3Dの両方があるなら `○○Demo2DScene` と `○○Demo3DScene`。`TweenDemo2DScene` などをまねる)。`Init()` には、`CreateDemoGuide(*this, { 名前, 何のデモか, 操作, API });` と、置きたい物の `Create○○(*this)` を並べるだけにする(Sceneには「何を置くか」だけ書く。§22)。2Dのシーンは `CreateDemoBackground` から、3Dのシーンは `CreateDemoStage3D(*this, 見る点, 向き, 距離)`(空・地面・カメラをまとめて置く)から始めるとよい。
   - **`CreateDemoGuide` は、設定を変えるオブジェクトより先に置く**(3Dカメラ・時間の倍率・光・影などを変えるComponentより前)。ガイドは`Init`でこれらの全体設定を既定へ戻すので、後に置くと、デモが決めた設定まで戻されてしまう。`CreateDemoStage3D`(カメラを含む)も、ガイドの後に呼ぶ。
2. 部品が必要なら、`Game/Objects/<カテゴリ>/`(工場関数)と `Game/Components/<カテゴリ>/`(動き)に作る。**ファイル名はプロジェクトの中で重複させない**(同じ名前の`.cpp`があると`.obj`が衝突する)。ボタンの文字はArial固定で日本語が出ないので、ボタンは英語、日本語の説明は`TextRenderer`で出す。
3. **`DemoRegistry.cpp` に、`#include "../<カテゴリ>/○○DemoScene.h"` と、`GetDemoList()` に1行**(`{ DemoCategory::Render, L"ボタンの名前", L"一言説明(30文字くらいまで)", &OpenScene<○○Demo2DScene>, &OpenScene<○○Demo3DScene> },`。片方だけの機能は、無い側を `nullptr` にする)を足す。これで機能一覧にボタンが出る。カテゴリのフォルダが無い時は、`Scenes/` `Objects/` `Components/` の下に作って、フィルターも足す。
4. 新しいファイルを、`Demo.vcxproj` と `Demo.vcxproj.filters` の両方に登録する(フィルターはフォルダと同じ名前: `Game\Scenes\Render` など)。
5. デモが**全体の設定**を新しく変える時(下の表に無い設定)は、`DemoGuideController::ResetSettings()` に「戻す処理」を足す(1か所に集めているので、足し忘れると次のデモに設定が残る)。`EventBus` を使う時は、そのシーンの`Uninit()`で`EventBus::Get().Clear()`する。
6. **2Dカメラ(`Image::GetCamera()`)を動かすデモ**(CameraやCoord)でも、画面のボタンは同じ場所に見えます(`CreateDemoButton` が、ボタンに`DemoScreenFixController`を付けるため)。文字や`Image::DrawRect`は画面の座標なので、元から動きません。2Dのデモの背景(`DemoBackground`)は、2Dモード(z=+2000、いちばん奥)で描いています。UIで描くと、2D・3Dモードのスプライトを後から上書きして隠してしまうためです(2D・3Dのスプライトを使うデモは、背景より手前のzに置く)。点や線の印を描きたい時は、`DemoMarker.h` の `DrawDemoPoint` / `DrawDemoLine` / `DrawDemoLabel` が使えます(2Dと3Dの両方に対応)。3Dで「マウスの下の地面」を求めたい時は `DemoMouseGround.h` の `GetMouseGroundPoint`。
7. **物理(PhysX)のデモ**は、剛体付きの物を `DemoBody.h` の `CreateDemoBody2D(scene, 設計図)` / `CreateDemoBody3D` で作り、壁は `DemoWall.h`(`CreateDemoArena2D` など)で作る。**2Dの動く物(`Dynamic`)は、`CreateDemoBody2D` が重力オフ・Z固定(奥に動かさない)を必ず付ける**(回転も既定は固定。`canRotate` で解除できる)(付け忘れると、2Dの物が下へ落ちていく。2Dの世界は、3Dと同じ物理の世界で、重力を切って作っている)。作った直後の剛体は、最初の`Update`で位置を合わせ直して**速さが0になる**ので、`SetVelocity`は2フレーム待ってから呼ぶ(`ColliderDemoController`)。動く剛体は`Transform`を書き換えず、`SetVelocity` / `AddForce`で動かす。状態の板は `DemoStatus.h` の `DrawDemoStatus` で出せる。
8. **動きとAI(Behavior)のデモ**は、2Dと3Dで同じ工場関数・Componentを共有する(`CreateMoveDemo(scene, mode)` など)。動く物・目印・壁・競技場・格子は `Objects/Behavior/DemoAgent.h` の `CreateDemoAgent` / `CreateDemoGoal` / `CreateDemoObstacle` / `CreateDemoField` / `CreateDemoNavigation` で作り、**位置は「平面の点」(2Dは x,y、3Dは x,z)**で渡す(`Components/Common/DemoPlane.h` の `ConvertPlaneToWorld` / `ConvertWorldToPlane` / `GetDemoMousePoint` / `GetDemoField` が、2Dと3Dの違いを吸収する)。壁には**タグ "Wall"** が付く(`VisionComponent` の視線と `NavGrid` の障害物は、このタグの物)。次の決まりがある。
   - **動く物の `BodyType`**: 壁にぶつけたい物(Move・Path・MovingWall の動く物、Vision のターゲット)は **`Dynamic`** にして、`MoveComponent` に `SetVelocity` で動かさせる。**`Kinematic` の物は、動かない箱(`Static`)とはぶつからず、すり抜ける**(Batch 4 の気付き)。Homing の3つは、お互いにすり抜けて動きだけを比べるため、わざと `Kinematic`(`Transform` を直接動かす)。
   - **格子(`NavGridComponent`)は、壁を置いた後に作る**。壁は最初のフレームの後(2回目の `Update`)に自動で読まれる。動く壁(ドア)は `Kinematic` で、`SetAutoRebakeInterval` か `RequestRebake` で焼き直す。格子の範囲の外に歩く物がいると止まるので、競技場(壁)の内側を格子にする。
   - **壁のすき間は、通れる広さにする**: 歩く物は、`agentRadius` と、マスの半分ずつ(両側で1マス)の分だけ、壁から離れないと通れない(マスの一部でも壁に重なると、そのマスは通れない)。すき間が `(agentRadius + cellSize/2) x 2 + cellSize` より狭いと、経路が見つからなくなる(2Dの Path のすき間は110、MovingWall は90と100)。
   - 全体の設定を変えるデモは、そのComponentの `Uninit` と `ResetSettings` の両方で戻す(`VisionComponent` / `NavGridComponent` / `PathFollowComponent` の `SetDebugDrawEnabled` は、`ResetSettings` に入っている)。

### 全体の設定のリセット

影・光・カメラ・時間の倍率などは、シーンをまたいで残ります。デモに出入りしても、次の画面に設定が残らないように、**`DemoGuideController::ResetSettings()` が、画面に入る時(`Init`)と出る時(`Uninit`)の両方で、次をすべて既定へ戻します**(2D版・3D版で分けず、1か所にまとめています。カテゴリ一覧・機能一覧も`CreateDemoGuide`を使うので、戻ってきた時も戻ります)。

| 設定 | 戻し方 |
|---|---|
| 2Dカメラ | `Image::GetCamera().position` を原点へ |
| 3Dカメラ | `Image::GetCamera3D() = Camera()` と `Image::SetCamera3DAutoSync(true)` |
| 時間の倍率 | `Time::SetTimeScale(1.f)` |
| 平行光源 | `Image::GetLight() = Light()` |
| 影 | `Shadow::SetSettings(Shadow::Settings())` |
| LOD | `Lod::SetEnabled(true)` / `Lod::SetGlobalScale(1.f)` |
| 当たり判定・視野・経路の枠線 | `RigidbodyComponent` / `VisionComponent` / `NavGridComponent` / `PathFollowComponent` の `SetDebugDrawEnabled(false)` |
| 画面エフェクト・カメラの揺れ | `ScreenEffect::Stop()` / `CameraShake::Stop()` |
| 音量 | `Audio::SetGroupVolume(AudioGroup::BGM, 1)` / `(AudioGroup::SE, 1)` |
| カーソルの固定 | `Input::SetCursorLocked(false)` |

さらに、デモの`Uninit`でも、自分が変えた物は戻しています(`DemoCameraController`は3Dカメラの自動追従、`ShadowDemoController`は影の設定、`LodDemoController`はLODと影の設定、`VisibilityDemoController`は2Dカメラの位置、`TimeDemoController`は時間の倍率、`InputDemoController`はカーソルの固定、`ResourcesDemoController`は借りた音とモデル、`SoundDemoController`はBGMを止める(借りた音のデータは`AudioSourceComponent`が返す)。`EventBusDemoScene`は`EventBus`の登録。`ScreenEffectDemoController`は暗転の濃さ(`Fade`はシーンをまたいで残るため、すぐ0へ戻す)、`CameraDemoController`は2Dカメラの位置・3Dカメラの自動追従・揺れ、`CoordDemoController`は2Dカメラの位置、`ColliderDemoController`は当たり判定の枠線(`SetDebugDrawEnabled`)。`ForceDemoController`など他の物理デモは、全体の設定を変えない。Behavior では、`VisionDemoController` は視野の線(`VisionComponent::SetDebugDrawEnabled`)、`PathDemoController` / `MovingWallDemoController` / `TerrainPathDemoController` は格子と経路の線(`NavGridComponent` / `PathFollowComponent` の `SetDebugDrawEnabled`)を、デモに入った時に出して、出る時に消す)。`ParticleRenderer`や`LightComponent`のような、オブジェクトに付く物は、Componentが自分の`Uninit`で後片付けするので、ここには入れません。

### 出入りの順序と、後片付け

シーンを切り替える時は「古いシーンの`Uninit` → 新しいシーンの`Init`」の順です(§16)。デモは、画像(`SpriteRenderer` / `MeshRenderer` / `BackgroundController` / `GroundController`)・パーティクルの画像・PhysXの剛体(3Dの歩く箱と地形)・光(`LightComponent`)の登録を、すべてComponentが自分の`Uninit`で返すので、「カテゴリ一覧 → 機能一覧 → デモ → Tabで2D/3D → 戻る」を繰り返してもリソースは増えません。`Resources::UnloadUnused()`が、古いシーンだけが使っていた画像を、切り替えの後で解放します。

### ビルドの設定(`Demo.vcxproj`)

`Sample2D` / `Sample3D` と同じ作りです(`Engine`を`ProjectReference`、`PhysX.props`をImport、x64のDebug/Release、出力先は共有の`x64\Debug`・`x64\Release`)。違いは次の2つです。

- Releaseの `WholeProgramOptimization`(全体最適化)を**切っています**。デモは`.cpp`が多く、全体最適化だとリンクが遅くなるため(サンプルは`true`のまま)。
- コンパイルのオプションに `/utf-8` を付けています(ソースはBOM付きのUTF-8なので、付けなくても動きますが、念のため)。

`Assets\` とシェーダーを共有の出力先へコピーするのは、今までどおりEngineの`CopyRuntimeFiles`だけです(Demo側には要りません)。
