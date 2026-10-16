# 3D_FreamWork

DirectX11製の、Unity風のコンポーネント指向ゲームフレームワーク。2D(スプライト)と3D(メッシュ)の両方に対応しています。

このドキュメントは、これから機能を追加していくチームメンバー向けの使い方ガイドです。

今後のEngine機能追加の計画(関数の設計案・作る順番)は [`docs/EngineRoadmap.md`](docs/EngineRoadmap.md) を参照してください。

---

## 0. プロジェクト構成

このソリューション(`3D_FreamWork.sln`)には3つのプロジェクトがあります。

```
3D_FreamWork\
├── 3D_FreamWork.sln
├── PhysX.props        共有MSBuildプロパティシート(PhysXのinclude/lib設定を1箇所にまとめたもの)
├── Assets\             全プロジェクト共有の画像・音声・モデル・フォント(Fonts\。§19)・表(Data\。§25)
├── Shader.hlsl / Shader3D.hlsl / Shader3DSkin.hlsl / Shader3DCommon.hlsli   全プロジェクト共有のシェーダー(Shader3DSkinは骨で曲げるFBX用。§27。Shader3DCommon.hlsliは3Dの2つのシェーダーに共通の見た目の計算で、マテリアルもここ。§30)
├── Engine\             フレームワーク本体(静的ライブラリ、Engine.vcxproj)
├── Sample2D\           2Dのサンプルゲーム(実行ファイル、Sample2D.vcxproj)
└── Sample3D\           3Dのサンプルゲーム(実行ファイル、Sample3D.vcxproj)
```

- **Engine**: `GameObject`/`Component`/`RigidbodyComponent`/描画/音声などフレームワーク本体。それ単体では実行できない静的ライブラリ(`Engine.lib`)で、基本的にみんなで触る場所ではありません。
- **Sample2D** / **Sample3D**: それぞれ独立した実行ファイル。どちらも同じ`Engine.lib`をリンクしているので、**Engineを直した内容は、両方のサンプルを再ビルドするだけで反映されます**。中身の`Game\Objects\`/`Game\Components\`/`Game\Scenes\`は、後述の「2. オブジェクトの作り方」と同じパターンです。
- 外部ライブラリは、リポジトリ直下(`3D_FreamWork\`の1つ上)の`ThirdParty\`にあります: `ThirdParty/PhysX/`(物理演算。§23)と `ThirdParty/ufbx/`(FBXの読み込み。§27。MITかパブリックドメインのどちらかを選べるライセンスで、条文は同じフォルダの`LICENSE`)。
- ビルド後、`Engine.lib`/`Sample2D.exe`/`Sample3D.exe`は全て同じ`x64\Debug\`(`Release\`)フォルダに出力されます。`Assets\`とシェーダーは、Engineのビルドのたびに(`Engine.vcxproj`の`CopyRuntimeFiles`)、新しいファイルだけがそこへ自動コピーされます。Engineのコードが変わっていなくても動き、`Assets/`とシェーダーを高速最新チェックの入力に登録してあるので(`UpToDateCheckInput`)、`Assets/`だけ編集して実行(F5)しても反映されます。ビルド済みの`.exe`をダブルクリックするだけでも動きます。
- ソリューションを開いてF5で実行する時は、実行したい方(Sample2DかSample3D)をソリューションエクスプローラーで右クリック→「スタートアッププロジェクトに設定」してから実行してください(複数のスタートアッププロジェクトを同時に起動する設定も可能です)。

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

新しいComponentを作る時は`Component`(`Engine/Component.h`)を継承し、必要な関数だけoverrideします:

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

**UIモード / Worldモード**を選べます:

```cpp
sprite->SetWorldSpace(true);
```

- `false`(既定): UIのように常に手前に描画される(深度を無視、画面に固定されたスクリーン座標)
- `true`: **本当に3Dワールドの中にある板(ビルボード)** として描画される。3Dメッシュ(Mesh)と全く同じcamera3DのView/Projection行列を使い、常にカメラの方を向く。3Dオブジェクトと同じ深度バッファで前後関係が決まり(position.zで前後に出たり隠れたりする)、`CameraController`でカメラを動かした時もMeshと同じように正しく追従する

テクスチャの透明な部分は、Worldモードなら後ろのオブジェクトが透けて見えます(`clip()`で深度を書き込まないようにしているため)。Worldモードはカリングも無効になっているので、`SetBillboardLock`で軸を固定した板をカメラが裏側から見ても、消えずにちゃんと描画されます(UIモードは今まで通り裏面カリングあり)。

**注意**: Worldモードの`scale`は(UIモードのピクセル単位とは違い)`MeshRenderer`と同じ**ワールド単位**として扱われます。`rotate.z`は、ビルボードの板の中での見た目の回転(画面に対する回転)として機能します。

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

これら以外の形状を追加したい場合は、同じ形式(`std::vector<MeshVertex>`を返す静的関数)で頂点データを作る関数を`Engine/Mesh.h/.cpp`に足してください(`Game/Objects/Cube.cpp`や`Sphere.cpp`が実例)。

**スカイボックス**: `Game/Objects/Skybox.cpp`が実例です。カメラを中心に追従する(回転はしない)巨大なメッシュを使った、遠景の作り方のパターンとして参考にしてください。

「必ず一番奥に描画される背景」を作る時は、深度テストで他のオブジェクトと競合させるのではなく、次の2つを組み合わせてください(スカイボックスの実装と同じ):

```cpp
skybox->SetDrawPriority(INT_MIN);      // 必ず一番最初に描画する(小さいほど先)
meshRenderer->SetDepthWrite(false);    // 深度バッファに書き込まない(後の描画を一切邪魔しない)
```

これにより、深度バッファの精度やカメラの位置・向きに一切依存しない、Z-fightingが原理的に起こらない背景になります(以前は深度テストに任せていたため、特定のカメラ角度でスカイボックスに隙間ができる不具合がありました)。

一つのGameObjectには、基本的に`SpriteRenderer`か`MeshRenderer`の**どちらか片方**を付けます。

**見た目の設定(マテリアル)**: 色・画像・光沢・自発光・半透明などは `Material` にまとめて持っています(`meshRenderer->GetMaterial()`で手直しできます。`SetColor`/`SetTexture`/`SetUnlit`/`SetDepthWrite` はその項目を書き換える短い書き方です)。詳しくは §30 を見てください。

**内部実装メモ**: シェーダー・サンプラー・ブレンド/深度/ラスタライザステート・定数バッファは、`Mesh`クラス全体で1つだけ作成して全インスタンスで共有しています(Cube/Sphere/Skyboxを何体作っても、シェーダーのコンパイルなどは初回の1回だけ)。頂点バッファ(形状データ)は作る時に1回だけ送る変更不可のバッファで、インスタンスごとに別です(描く度の頂点のコピーはしません。§30)(テクスチャは`Resources`で同じ画像どうし共有しています。§11)。特に意識する必要はありませんが、`Engine/Mesh.h/.cpp`を触る時は「インスタンスごとの値」と「共有する値(static)」を混同しないよう注意してください。

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

光る点(ハイライト)や自分で光る色は、マテリアルで付けられます(§30)。

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

`GetMouseWorldPosition()`は、マウス座標(Yを反転して上方向に合わせたもの)を`Image::ScreenToWorld2D`(§14)でZ=0の平面のワールド座標に直しています。カメラが既定の奥行き(z=-500)なら「カメラ位置 + マウス座標」と同じで、Z=0にある2Dオブジェクトとぴったり一致します。カメラを動かす処理(`CameraFollowComponent`など)がある場合は、カメラを動かした**後**で呼ぶと、そのフレームのカメラ位置で計算されます。画面の揺れ(`CameraShake`)は入りません。

**2D/3Dのスクロール速度について**: 2Dは疑似的な正射影(距離に関係なく一定速度でスクロール)、3D(Mesh)は本物の透視投影(近いものほど速く、遠いものほどゆっくり動く)なので、根本的に仕組みが違います。`Camera.focalLength`(既定500)の距離にある3Dオブジェクトだけは、2Dと同じ速度で動くように`fovY`を自動調整していますが、それ以外の距離にあるオブジェクトは2Dとズレます(これは正しい遠近感なので、バグではありません)。

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

`Image::GetCamera3D()`を直接操作するため、有効な間は`Image::BeginFrame()`による2Dカメラへの自動追従(前述)を自動で止めます(`Image::SetCamera3DAutoSync(false)`。Component破棄時に自動でtrueへ戻ります)。MeshRendererだけでなく、Worldモードの`SpriteRenderer`(ビルボード)もこのcamera3Dを使うので、両方とも同じカメラの動きに正しく追従します。**UIモードの`SpriteRenderer`(HUDなど)は画面に固定されたままで、影響を受けません。**

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
void OnTriggerEnter2D(Collider2D* other) override;  // 入った瞬間
void OnTriggerStay2D(Collider2D* other) override;   // 入ったフレームの次から、出るまで毎フレーム
void OnTriggerExit2D(Collider2D* other) override;   // 出た瞬間

// isTrigger = false の時(押し返しも自動で行われる)
void OnCollisionEnter2D(CollisionInfo info) override;  // info.otherで相手のColliderが取れる
void OnCollisionStay2D(CollisionInfo info) override;
void OnCollisionExit2D(CollisionInfo info) override;
```

相手のタグは `other->GetTag()` (Trigger) / `info.other->GetTag()` (Collision) で判定します(`Collider2D`は当たり判定の計算自体は持たない、コールバック用の軽いハンドルです)。

**`OnTriggerStay2D` が呼ばれるタイミング**
- 入ったフレームは `OnTriggerEnter2D` だけで、**次のフレームから**、出る(`OnTriggerExit2D`)まで毎フレーム呼ばれます(Unityと同じ順番)。トリガー側と入ってきた側の両方に呼ばれます
- ポーズ中(`Time::SetTimeScale(0.0f)` で、PhysXの計算が止まっている間)は呼ばれません
- どちらかが非アクティブ(`ObjectPool` に戻った)・`Destroy()` 済みの間は呼ばれません。プールに戻した物をまた使った時は、重なっていれば `OnTriggerEnter2D` からやり直しになります
- `OnTriggerStay2D` の中で、相手をプールに戻す・`Destroy()` する・`AddForce` するのは大丈夫です(`AddForce` はその場で効きます)
- PhysXはトリガーの「重なっている間」の知らせを出さないので、`Engine/Physics.cpp` が Enter で組み合わせを覚えて、Exit で忘れ、`Physics::Update()` の最後に毎回呼んでいます

**注意: トリガー同士が重なっても、トリガーのコールバックは呼ばれません**
- 両方が `isTrigger = true` の物同士が重なっても、`OnTriggerEnter2D` / `OnTriggerStay2D` / `OnTriggerExit2D` は呼ばれません。これはPhysX 4.1の仕様です(`PxShape.h` / `PxSimulationEventCallback.h` に書かれています)。Unityでは、どちらかにRigidbodyがあればトリガー同士でも呼ばれるので、ここはUnityと違います
- トリガーと普通の当たり判定(`isTrigger = false`)の組み合わせは呼ばれます(今のサンプルの「弾(トリガー)と敵」や、ゴールをトリガーにしてPlayerが触れる場合など)
- トリガー同士で重なりを知りたい時の対策:
  1. 片方をトリガーにしない(ただし普通の当たり判定は、ぶつかって止まる/押し返す動きになります)
  2. すり抜けたまま調べたい時は、範囲の側が自分から探す。例: `Physics::OverlapBox(t.position, t.scale, "Bullet", QueryTriggers::Include)`(→「物理の問い合わせと力」の節)

**内部実装メモ**: `Engine/Physics.cpp`のシミュレーションコールバック(`onContact`/`onTrigger`)が、PhysXの結果をこれらのコールバックに変換しています。Kinematicなオブジェクト同士がぶつかった時の「押し戻し」は、PhysXの自動計算に任せられない(キネマティックは力の影響を受けないため)ので、接触の法線・めり込み量から手動で`Transform.position`をずらしています(`isStaticForPush=true`側は動かさない)。Dynamicなオブジェクト(Sphereなど)が絡む場合は、PhysX自身が正しく押し返すので、この手動処理はスキップされます。

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

方向・距離・角度・乱数の計算は、`Engine/MathUtil.h` の `namespace Math` にまとめてあります。各Componentで `atan2f`/`sqrtf`/`sinf`/`cosf`/`rand()` を直接書かずに、こちらを使ってください(0で割る・角度の向きを間違える、といったミスを防ぐため)。

```cpp
#include "../../../Engine/MathUtil.h"
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

「1フレームの時間」「時間の倍率(スロー・ポーズ)」は `Engine/Time.h` の `namespace Time`、「○秒たったら」を数えるのは `Engine/Timer.h` の `Timer` でできます。各Componentで `timer -= dt; if (timer <= 0) ...` のような処理を手書きせずに、こちらを使ってください。

### Time(1フレームの時間・スロー・ポーズ)

```cpp
#include "../../../Engine/Time.h"
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
#include "../../../Engine/Timer.h"
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

同じ画像を何度も読み込まないための仕組みです(`Engine/Resources.h` の `namespace Resources`)。同じパスの画像は1回だけ読み込み、「何人が使っているか」(参照カウント)を数えて管理します。敵を100体出しても、`player.png` を読み込むのは1回だけです。

**`SpriteRenderer` / `MeshRenderer` は中で自動で使っているので、ゲーム側は普段意識しなくてかまいません**(`AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2)` の書き方はそのまま)。自分で画像を読んで描くComponent(Sample2D `BackgroundController`、Sample3D `GroundController` など)だけが、直接使います。

```cpp
#include "../../../Engine/Resources.h"

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

- `Engine/Ease.h` の `namespace Ease`: 動きの「速さの変化」を決める関数(イージング関数)
- `Engine/TweenComponent.h`: オブジェクトの位置・大きさ・回転・色(と好きな値)を、イージングを付けて目標値まで動かすComponent

```cpp
#include "../../../Engine/TweenComponent.h"   // Ease.h も一緒に読み込まれる
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
#include "../../../Engine/ChaseComponent.h"   // 使うComponentのヘッダーを読む(MoveComponent.h も一緒に読み込まれる)

auto* chase = enemy->AddComponent<ChaseComponent>();
chase->SetTarget(player);          // 追いかける相手
chase->SetSpeed(60.f);             // 速さ(1秒に進む距離)
chase->SetPlane(MovePlane::XZ);    // 3Dの地面の上を動く時(2Dなら不要。既定はXY)
```

| Component | 動き |
|---|---|
| `ChaseComponent` | 一定の速さでまっすぐ向かう(両サンプルの敵) |
| `AccelerateChaseComponent` | 加速しながら向かう。勢いがあるので行き過ぎて戻ってくる |
| `HomingComponent` | 曲がれる角度に上限がある追尾(ミサイル・車・魚向け)。急に向きを変えない |

3つとも、親クラス `MoveComponent` の次の関数が使えます(`MoveComponent` 単体ではAddComponentしません)。

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

**実例**: 両サンプルの `EnemyManager` が、敵に `ChaseComponent` を付けて(`SetupEnemy`)、出す時に追いかける相手(Player)と速さを渡しています(`Spawn`)。Sample3Dは `SetPlane(MovePlane::XZ)` で、地面の上を重力で落ちながら追いかけます。

---

## 14. カメラ(CameraFollowComponent / CameraShake / 座標の変換)

カメラの追従・画面の揺れ・座標の変換を、毎回手書きしなくて済むようにしたものです。カメラそのもの(2D用`Image::GetCamera()`と3D用`Image::GetCamera3D()`の違い)は§5を参照してください。

```cpp
#include "../../../Engine/CameraFollowComponent.h"   // 追従
#include "../../../Engine/CameraShake.h"             // 揺れ
#include "../../../Engine/Image.h"                   // 座標の変換
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

**実例**: Sample2Dの`PlayerController::OnCollisionStay2D()`で、敵に当たってダメージを受けた時に揺らしています(強さ・時間は`PlayerController.h`の`damageShakeStrength`/`damageShakeDuration`)。

### 座標の変換(ワールド ⇔ 画面)

**画面座標は「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」**(`Text::Draw`と同じ)なので、結果をそのまま`Text::Draw`に渡せます。

| 関数 | カメラ | 内容 |
|---|---|---|
| `Image::WorldToScreen2D(ワールド座標)` | 2D用 | ワールド座標 → 画面座標(`XMFLOAT2`)。UIモードの`SpriteRenderer`と同じ計算(奥(Z+)にあるほど小さく、中心に寄って見える。Z=0なら「ワールド座標 - カメラ位置」) |
| `Image::ScreenToWorld2D(画面座標)` | 2D用 | 画面座標 → Z=0の平面の上のワールド座標(`XMFLOAT3`、zは0)。`WorldToScreen2D`の逆。`Input::GetMouseWorldPosition()`の中身 |
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
- 2D用カメラ(疑似的な正射影)と3D用カメラ(透視投影)は計算の仕方が違うので、関数の名前で`2D`/`3D`を分けています。

**実例**: 両サンプルの`ButtonComponent::Draw()`(Engine)が、ボタンの文字の位置を`Image::WorldToScreen2D`で求めています。

---

## 15. 画面エフェクト(ScreenEffect)

被弾・回復・シーン切り替えなどで、画面全体に色の演出を重ねます。

```cpp
#include "../../../Engine/ScreenEffect.h"
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
- 両サンプルの`PlayerController::OnCollisionStay2D()`: ダメージを受けた時に`Vignette`(色・時間・回数は`PlayerController.h`の`damageVignetteColor`/`damageVignetteDuration`/`damageVignetteBlinkCount`)
- STARTボタンの暗転・明転は、§16の`FadeTransition`に置き換えました(`ScreenEffect::Fade`はシーン切り替え以外の演出用に残しています)

---

## 16. シーン切り替え(SceneTransition)

シーンを切り替える時に、暗転や画像のアニメーションで画面を隠します。どの切り替え方も「**画面を覆う → シーンを切り替える → 開く**」の順に進みます。

```cpp
#include "../../../Engine/SceneManager.h"   // これだけで3種類とも使える
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
#include "../../../Engine/SpriteAnimationComponent.h"

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
#include "../../../Engine/SaveData.h"

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

画面に文字を描く仕組みです(`Engine/Text.h`、`Engine/TextRenderer.h`)。座標は **「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」** です(§14の座標の変換と同じ)。`Text::Draw` は Component の `Draw()` の中で呼びます。

**今までの書き方も、そのまま使えます**(見た目も今までと同じ。文字の中心が (x, y) に来ます):
```cpp
#include "../../../Engine/Text.h"

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
#include "../../../Engine/TextRenderer.h"

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

HPバー・音量スライダー・ボタンを、どのゲームでも使える部品にしたものです(`Engine/Anchor.h`、`Engine/GaugeRenderer.h`、`Engine/SliderComponent.h`、`Engine/ButtonComponent.h`、`Engine/Image.h` の `DrawRect`)。画面サイズが変わっても、隅や中央からの距離でずれずに置けます。

### 画面座標の決まり

- 座標は **「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」** です(§14の座標の変換、§19の `Text::Draw` と同じ)。下へずらす時はYをマイナスにします。
- **`Input::GetMousePosition()` はY+=下**なので、マウスと比べる時はYの符号を反転します(`SliderComponent` は中でこれをやっています)。
- UIの四角(`DrawRect`、ゲージ、スライダー)は、`Text::Draw` と同じ Direct2D で描きます。だから**カメラの影響を受けず(スクロールしない)、画像や3Dの描画が全部終わった後に重なります**。`Text::Draw` とは**呼んだ順**に重なります(ゲージを描いてから数字を描けば、ゲージの上に数字が出ます)。
- 画面エフェクト(§15)とシーン切り替え(§16)は、UIよりさらに上に出ます。

### Anchor(画面のどこを基準にするか)

```cpp
#include "../../../Engine/Anchor.h"

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
#include "../../../Engine/GaugeRenderer.h"

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
#include "../../../Engine/SliderComponent.h"

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
#include "../../../Engine/ButtonComponent.h"

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

`Audio::Load()` / `Audio::PlayOneShot()` / `AudioSourceComponent`(`Engine/Audio.cpp`)は`.wav`ファイルの`fmt `/`data`チャンクを名前で探して読みます。以前は「`fmt `の直後は必ず`data`」という決め打ちの44バイト固定ヘッダーで読んでいたため、ffmpeg/Lavfなど一部のツールで書き出したWAV(`fmt `と`data`の間に`LIST`メタデータチャンクが挟まる)を読み込むと、メタデータの文字列を音声データと誤認識し、音が鳴らない(鳴ってもごく短いノイズだけ)という不具合がありました。現在は汎用的なチャンク読み取りに直してあるので、`LIST`/`fact`など余計なチャンクが挟まっていても正しく読めます。**それでも音が鳴らない場合は、まず`.wav`ファイル自体が壊れていないか(別プレイヤーで再生できるか)を確認してください**。

---

## 22. サンプル構成(現状)

プロジェクト構成自体は`0. プロジェクト構成`を参照してください。ここでは各サンプルの中身(`Game/`)を説明します。

```
Sample2D/Game/Objects/     Title, Button, StartButton, VolumeSlider, Background, Player, GameSystem, HUD,
                            EnemySpawner, BulletSpawner, ScorePopup, Explosion, Guard, Wall
Sample2D/Game/Components/  TitleController, BackgroundController, PlayerController,
                            BulletController, EnemyController, GameManager, HUDController,
                            EnemyManager, BulletManager, GuardController
Sample2D/Game/Scenes/      TitleScene, GameScene

Sample3D/Game/Objects/     Title, Button, StartButton, VolumeSlider, Skybox, Ground, Player, GameSystem, HUD,
                            EnemySpawner, BulletSpawner, Explosion, ModelDisplay, SkinDisplay, Guard, Wall, GlassPanel, Cube(未使用), Sphere(未使用)
Sample3D/Game/Components/  TitleController, GroundController, PlayerController,
                            CameraController, BulletController, EnemyController, GameManager,
                            HUDController, EnemyManager, BulletManager, SkyboxFollowComponent,
                            ModelDisplayController, GuardController, SpinComponent(未使用)
Sample3D/Game/Scenes/      TitleScene, GameScene
```

- **Sample2D**: トップダウン視点のアリーナシューター。タイトル画面のSTARTボタン(出る時に`TweenComponent`でぴょこっと大きくなる)をクリックすると、画面が暗転してからゲーム開始(ゲーム画面に入ると明転する。`FadeTransition`。§16)(ボタンは`CreateButton(scene, 文字, 位置, クリックされた時の処理)`で作れる汎用部品で、動きはEngineの`ButtonComponent`。他の画面のボタンにも使い回せる。§20)。WASDで移動、Playerは常にマウスの方を向き、左クリックでその方向へ弾を発射。背景はタイル状に敷き詰めた床(`BackgroundController`)で、移動するとスクロールする(カメラはPlayerに付けた`CameraFollowComponent`が追いかける。§14)。敵(`EnemyManager`が一定間隔でPlayerの周囲にランダムスポーンさせる)がゆっくり追いかけてきて(`ChaseComponent`。§13)、弾を当てると倒せる(`EventBus`で`"EnemyDefeated"`イベントを発行しスコア加算。倒した位置に「+10」が出て、上に動きながら消える=`CreateScorePopup`。§19。爆発のパーティクルも出る=`CreateExplosion`。§26)。敵に触れるとHPが減り(1秒間の無敵時間つき)、敵から離れる向きに弾かれる(ノックバック。`AddForce`で衝撃を加え、0.2秒間は移動キーが効かない。§23)、画面が少し揺れて(`CameraShake`。§14)、画面の端が赤く点滅する(`ScreenEffect::Vignette`。§15)。HPが0になるとゲームオーバー画面が出てSPACEでタイトルに戻る(黒い丸が閉じて開く`SpriteTransition`。§16)。スコアがハイスコアを超えていたら、ゲームオーバーの時に保存され(`SaveData`。キーは`"Sample2DHighScore"`で、Sample3Dの記録とは別。§18)、画面左上のスコアの下に出る(次に起動しても残る)。タイトル画面の下にはBGMの音量スライダー(`CreateVolumeSlider`。つまみをドラッグして0〜1で変えられ、離すと保存されて次に起動した時も残る。§20)がある。画面左上のHPの文字の下にはHPゲージ(減った分が少し遅れて縮む)、Playerの頭上にも小さなHPバーが出る(`GaugeRenderer`。§20)。当たり判定・物理はPhysXを使い(Player=`CircleRigidbodyComponent`、敵=`SquareRigidbodyComponent`、弾=`CircleRigidbodyComponent`)、Player・敵はDynamic(`SetVelocity()`で速度を渡して動かす。重力なし・回転なし・`SetFreezePositionZ(true)`で奥行き方向に固定)、弾はKinematicのトリガー。3Dカメラは使わない(`CameraController`なし)。画面の左上のHPゲージの下には、画面の外なので描かずに省いた数「Culled」が出る(敵の `SpriteRenderer` は `SetFrustumCulling(true)`。§28)。マップには見張り役の `Guard`(青い `Player` 画像がゆっくり回り、扇形の視野に `Player` が入って壁にさえぎられていなければ赤くなる。視野は線で表示)と、視線をさえぎる壁 `Wall` が2枚置いてある(`VisionComponent`。§28)。
- **Sample3D**: Sample2Dと同じアリーナシューターを3D空間で遊べるようにしたもの。タイトル画面のボタン(Sample2Dと同じく、出る時に`TweenComponent`でぴょこっと大きくなる)をクリックすると、Sample2Dと同じく暗転してからゲーム開始(Sample3Dの`CreateButton()`は、位置と大きさをまとめて`Transform`で受け取る形になっている: `CreateButton(scene, 文字, transform, クリックされた時の処理)`)。Player(立方体)は三人称カメラ(`CameraController`。マウス移動で視点回転。カーソルは画面中心に固定され、ESCで解除・左クリックで再固定。TABで一人称に切り替え)と同じ向きを向き、WASDもカメラ基準(W=カメラの前、D=カメラの右)で移動する。左クリックでPlayerの向いている方向へ、地面と水平に弾(球)を発射。敵を倒すと、その位置に爆発のパーティクルが出る(`CreateExplosion`。§26)。敵は2Dの画像(Worldモードの`SpriteRenderer`。Y軸だけカメラの方を向く立て看板)で、地面の上をPlayerに向かって歩いてくる(`ChaseComponent`を`SetPlane(MovePlane::XZ)`で使う。§13)。歩く間は、敵に付けた`SpriteAnimationComponent`がスプライトシートのコマ(4番と5番)を0.3秒ごとに切り替えて、歩きアニメーションにしている(始まるコマはランダムなので、何体いても足踏みがそろわない。§17)。スコア・ハイスコア(保存先のファイルはSample2Dと同じだが、キーは`"Sample3DHighScore"`で、記録はSample2Dとは別。§18)・HP・無敵時間・被弾時に画面の端が赤く点滅すること・ゲームオーバー(SPACEでタイトルへ。Sample3Dは、黒い帯が左から覆って右へ抜けていく`SpriteTransition(...).SetSwitchFrame(7)`。§16)・`EventBus`のイベント名はSample2Dと同じ。タイトル画面の下のBGMの音量スライダーと、画面左上のHPゲージ(Sample2Dと同じ)、三人称の間だけ出るPlayerの頭上のHPバー(`SetUse3DCamera(true)`の`GaugeRenderer`)もある(§20)。Playerは立方体の見た目とDynamicの`BoxRigidbodyComponent`、敵はDynamicの`CapsuleRigidbodyComponent`(半径40・高さ100の立ったカプセル。§29)(どちらも重力あり・回転なし。Y速度は`GetVelocity().y`を維持したまま`SetVelocity()`で動かす。敵は`ChaseComponent`がこれをしている)、弾はKinematicのトリガー(`SphereRigidbodyComponent`)。地面は、1000四方のタイルを3Dカメラの描画距離(farZ)の範囲だけ敷き詰めて描く`GroundController`(見た目)と、見えない大きなStaticの板(当たり判定)の組み合わせ。遠景はスカイボックス。地面の端(Playerから斜めに離れた所)には、FBXモデルの見本(`CreateModelDisplay`。`Assets/Models/TestAnim.fbx` の立方体2つ=親と子のパーツ。子の「Top」が、「Spin」(回る)と「Bob」(上下)のアニメーションを3秒ごとに切り替えて再生する。§27)が置いてある(FBXが無い時は何も置かない)。そのとなり(位置 `(1000, -50, 800)`)には、骨2本で曲がる細長い箱(`CreateSkinDisplay`。`Assets/Models/TestSkin.fbx` のアニメーション「Bend」を再生。§27)も置いてある。地面のタイルは、カメラに映る物だけ描く(`Visibility::IsSphereInView3D`)。敵の画像は `SetFrustumCulling(true)` で、画面の外にいる間は描かない。画面の左上のHPゲージの下には、描かずに省いた数「Culled」が出る。`Player` の近く(位置 `(-800, 0, 600)`)には見張り役 `Guard`(青い立方体がゆっくり回り、扇形の視野に `Player` が入って壁にさえぎられていなければ赤くなる。視野は線で表示)と、視線をさえぎる壁 `Wall` が2枚置いてある(`VisionComponent`。§28)。そのとなり(位置 `(400, 50, 300)`)には、半透明で光沢のある水色の板 `GlassPanel`(マテリアルの使用例。当たり判定は無いので通り抜けられる。§30)も置いてある。

**Sample3Dの実装メモ**: 向きはすべて`transform.rotate.y`(度)で表し、`rotate.y`の時の正面(ローカルの+Z)は`(sin, 0, cos)`の方向になる(`CameraController::GetYaw()`の式と同じ。`Math::Forward3D()`で求められる)。Worldモードの`SpriteRenderer`は`rotate.y`を使わない(ビルボードの向きとZ軸回転だけを使う)ので、敵の画像の向きは`SetBillboardLock(true, false, true)`でカメラの方へ向けている。Playerは`SetFreezeRotation(true)`で、`RigidbodyComponent`は位置しか同期しないため、`PlayerController`が`rotate.y`が変わった時に`GetActor()`の`setGlobalPose`で当たり判定の箱の回転も合わせている(デバッグ表示の枠と実際の判定が同じ向きになる)。一人称の間は、カメラが立方体の内側に入るのでPlayerの`MeshRenderer`と頭上のHPバー(`GaugeRenderer`)を非表示にしている。

**Sample2Dの実装メモ**: `EnemyManager`/`BulletManager`はどちらも`ObjectPool`を使った使い回し方式(実例は`Engine/ObjectPool.h`)。プールで使い回すオブジェクトは`Scene::CreateObject("Pooled")`で名前だけ付けて作られるため、`GameObject::SetTag()`で組み立て時にタグ("Enemy"/"Bullet")を後付けしている。敵を倒す/Playerがダメージを受ける判定は、それぞれ`EnemyController::OnTriggerEnter2D`(相手のタグが"Bullet")と`PlayerController::OnCollisionStay2D`(相手のタグが"Enemy")が担当する。

役割分担は「値の持ち主は1か所、他は読むだけ」にしている:

| Component | 持っている値・担当 |
|---|---|
| `PlayerController` | HP・無敵時間・ノックバック(HPが0になったら`EventBus`で`"PlayerDied"`を発行し、以後は移動・向き・発射を受け付けない。ポーズ中・シーン切り替え中も`Time::IsPaused()`で受け付けない) |
| `GameManager` | スコア(`"EnemyDefeated"`で加算)・ゲームオーバー判定(`"PlayerDied"`を受けて)・ハイスコアの更新と保存(ゲームオーバーの時。ハイスコアの値そのものは`SaveData`が持ち、`GetHighScore()`はそれを読むだけ。§18)・タイトルへの遷移。表示は一切しない |
| `EnemyManager` | 敵のプールと出現の予定。`Init()`で`Assets/Data/Enemies.csv`(敵の速さ)と`Assets/Data/Waves.csv`(出現ウェーブ)を読み(§25)、表の時間にPlayerの周囲に敵を出し(追いかける動きは敵に付けた`ChaseComponent`に、相手と速さを渡す)、`"PlayerDied"`を受けたら出現を止める |
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
| 敵を倒した位置に「+10」を出す(Sample2D) | `EnemyController::OnTriggerEnter2D()`から`CreateScorePopup()`を呼ぶ(動き・消え方は`Objects/ScorePopup.cpp`で付けた`TweenComponent`。§19) |
| 敵を倒した位置に爆発を出す | `EnemyController::OnTriggerEnter2D()`から`CreateExplosion()`を呼ぶ(両サンプル。粒の設定は各サンプルの`Objects/Explosion.cpp`、粒の動き・描画はEngineの`ParticleRenderer`。§26) |
| FBXモデルの見本を置く、Topパーツのアニメーションを切り替える(Sample3D) | `Objects/ModelDisplay.cpp`(`ModelRenderer`と`ModelAnimationComponent`を付ける)と `ModelDisplayController::Update()`(3秒ごとに`CrossFade`。§27) |
| 骨で曲げるFBXモデルの見本を置いて "Bend" を再生する(Sample3D) | `Objects/SkinDisplay.cpp`(`ModelRenderer`と`ModelAnimationComponent`を付け、`Play("Bend")`。§27) |
| 敵がPlayerを追いかける | 敵に付けた`ChaseComponent`(Engine。§13)。相手と速さは`EnemyManager::Spawn()`が渡す |
| 敵の歩きアニメーション(Sample3D) | 敵に付けた`SpriteAnimationComponent`(Engine。§17)。クリップの登録と再生は`EnemyManager`の`SetupEnemy` |
| 敵に当たったら弾かれる(Sample2D) | `PlayerController::OnCollisionStay2D()`(`AddForce`。§23) |
| 敵に当たったら画面が揺れる(Sample2D) | `PlayerController::OnCollisionStay2D()`(`CameraShake::Shake`。§14) |
| 敵に当たったら画面の端が赤く点滅する | `PlayerController::OnCollisionStay2D()`(`ScreenEffect::Vignette`。§15) |
| タイトルのBGM音量スライダー | `Objects/VolumeSlider.cpp`(`SliderComponent`を付け、動かしたら`TitleController::SetVolume()`、離したら`SaveData::Save()`。§20) |
| 発射音・敵を倒した音 | `BulletManager::Fire()`(`shot.wav`)・`EnemyController::OnTriggerEnter2D()`(`hit.wav`)から`Audio::PlayOneShotAt`(その場所で鳴る。連射しても重なる。§24) |
| 音を聞く位置 | `Objects/Player.cpp`で付けた`AudioListenerComponent`(Engine。Sample3Dは`SetUse3DCamera(true)`も。§24) |
| 画面左上のHPゲージ | `Objects/HUD.cpp`(`GaugeRenderer`を付け、`SetValueSource`で`PlayerController`のHPを読む。§20) |
| Playerの頭上のHPバー | `Objects/Player.cpp`(`GaugeRenderer`の`SetFollowTarget`。Sample3Dは`SetUse3DCamera(true)`も。一人称の間は`PlayerController`が隠す。§20) |
| カメラがPlayerを追いかける(Sample2D) | `Objects/Player.cpp`で付けた`CameraFollowComponent`(Engine。§14) |
| コライダー枠・見張り役の視野(扇形)の表示ON | `GameManager::Init()`(`RigidbodyComponent::SetDebugDrawEnabled` と `VisionComponent::SetDebugDrawEnabled`) |
| 見張り役(Guard)を置く、ゆっくり回して視野に入ったら赤くする | `Objects/Guard.cpp`(`VisionComponent` を付ける)と `GuardController::Update()`(`CanSee(player)`。§28) |
| 敵の当たり判定(立ったカプセル。Sample3D) | 敵の `SetupEnemy`(`CapsuleRigidbodyComponent`+`SetFreezeRotation(true)`。§29) |
| 壁(Wall)を置く(視線をさえぎる) | `Objects/Wall.cpp`(タグ `"Wall"` + Staticの `BoxRigidbodyComponent`。§28) |
| 半透明で光沢のある板を置く(Sample3D) | `Objects/GlassPanel.cpp`(`MeshRenderer` の `GetMaterial()` で色のα・光沢・自発光・`depthWrite = false` を設定し、`SetDrawPriority` で後に描く。§30) |
| 地面のタイルのうち、画面に映る物だけ描く(Sample3D) | `GroundController::Draw()`(`Visibility::IsSphereInView3D`。§28) |
| 画面の外の敵を描かない | 敵の `SetupEnemy`(`SpriteRenderer::SetFrustumCulling(true)`。両サンプル。§28) |
| 画面の外で省いた数の表示(Culled) | `HUDController::Draw()`(`Visibility::GetCulledCount()`。§28) |
| ゲームオーバーの時にハイスコアを保存する | `GameManager::Init()`の`"PlayerDied"`を受けた所(`SaveData::SetInt` + `Save`。§18) |

Sceneに残してよいのは、オブジェクトの配置と、シーン全体の後片付け(`GameScene::Uninit()`の`EventBus::Get().Clear()`)くらい。新しい処理を足したくなったら、まず「どのオブジェクトの仕事か」を考えて、そのComponentに書くこと。

どちらも現時点では「動く土台」で、ゲームとしての作り込み(敵の挙動、演出、レベルデザインなど)はこれから追加していく想定です。新しいオブジェクトを追加する時は、それぞれのサンプルの`Game/Objects/`と`Game/Components/`に、既存のPlayer/Enemyと同じ形式でファイルを足していくのが基本の流れです。

---

## 23. PhysX(3D物理演算)

3D用の物理エンジンとして[NVIDIA PhysX 4.1](https://github.com/NVIDIA-Omniverse/PhysX)を導入しています。ビルド済みのライブラリ一式(ヘッダー・dll・lib)を`ThirdParty/PhysX/`にコミット済みなので、**チームメンバーは自分でビルドし直す必要はありません**(`git clone`してそのままビルドできます)。

### 現状

`Engine/Physics.h/.cpp`が、PhysXの初期化・毎フレームのシミュレーション更新・終了処理を行う最小限の窓口です(Image/Audio/Textと同じ、名前空間+staticなグローバル状態という流儀)。`GameEngine::Init/Run/Uninit`から自動的に呼ばれるので、現時点では特に何もしなくてもPhysXは起動・更新され続けています。

```cpp
namespace Physics {
    bool Init();      // PxFoundation/PxPhysics/PxSceneを作る(GameEngine::Initから呼ばれる)
    void Update(float dt); // シーンをシミュレーションする(GameEngine::Runから毎フレーム呼ばれる)
    void Uninit();    // 後片付け(GameEngine::Uninitから呼ばれる)
}
```

ゲーム側から使う「問い合わせ」(`Raycast`/`OverlapSphere`/`OverlapBox`)と、剛体に力を加える関数(`AddForce`など)は、下の「物理の問い合わせと力」を参照してください。

### RigidbodyComponent(3Dの物理演算でオブジェクトを動かす)

`Engine/RigidbodyComponent.h/.cpp`に、`BoxRigidbodyComponent`/`SphereRigidbodyComponent`(3D向け)と`SquareRigidbodyComponent`/`CircleRigidbodyComponent`(2D向け)があります。見た目(`MeshRenderer`/`SpriteRenderer`)とは別に、動き方だけを担当するComponentです。4つとも共通の基底クラス`RigidbodyComponent`を継承しているので、他のComponentから形状を意識せず`GetComponent<RigidbodyComponent>()`で探せます(`PlayerController`が実例。これにより、Playerの形状を変えても`Player.cpp`の1行を変えるだけで済みます)。

- 立った人・敵向けの`CapsuleRigidbodyComponent`(3D)/`Capsule2DRigidbodyComponent`(2D)もあります(§29)
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
- `isTrigger`: `true`ですり抜ける当たり判定(`OnTriggerEnter2D`)になる。既定`false`(押し返される、`OnCollisionEnter2D`)
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

**重要な注意(ハマりやすいポイント)**: `OnTriggerEnter2D`/`OnCollisionEnter2D`などのコールバックは、PhysXのシミュレーションコールバックの中から呼ばれています。PhysXは「コールバックの中でシーンを書き換えるAPI(`scene->addActor()`/`removeActor()`など)を直接呼んではいけない」という制約を持っているため、これらのコールバック内(や、そこから呼ばれる`ObjectPool::Return()`のような処理)で`SetActive(false)`する場合は要注意です。`RigidbodyComponent`はこれに対応済み(`Physics::QueueSceneChange()`で安全なタイミングまで遅延させている)ですが、**今後もし`OnTriggerEnter2D`などの中から新しくPhysXのシーンを直接触るコードを書く場合は、同じように`Physics::QueueSceneChange()`を使うか、フラグを立てて次のフレームで処理するようにしてください**。直接呼ぶと`Concurrent API write call ... during a callback function are not permitted`というアサーションでクラッシュします。(`RigidbodyComponent::AddForce()`/`AddTorque()`も対応済みで、コールバックの中で呼ばれた時は、PhysXの計算が終わった直後にまとめて加えます。次の節を参照)

`GameEngine::Run()`では、`Physics::Update(dt)`を`SceneManager::Get().Update(dt)`より**先に**呼ぶようにしています。逆順にすると、RigidbodyComponentが1フレーム古い物理結果をTransformに反映してしまいます。

### 物理の問い合わせと力(Raycast / Overlap / AddForce)

「この線の先に何があるか」「この範囲に何がいるか」を調べる関数(`Engine/Physics.h`)と、剛体に力を加える関数(`RigidbodyComponent`)です。

```cpp
#include "../../../Engine/Physics.h"

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
- 読むだけなので、`OnTriggerEnter2D` などのコールバックの中から呼んでも大丈夫です。

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
- `AddForce`/`AddTorque` は、`OnCollisionStay2D` などのコールバックの中で呼んでも大丈夫です(PhysXの計算が終わった直後にまとめて加わります)。`SetMass`/`SetDrag` はコールバックの中では呼ばず、`Init`/`Update` で呼んでください。
- 毎フレーム `SetVelocity()` で速度を上書きしているオブジェクト(Playerなど)は、上書きした時点で力で付いた勢いが消えます。力で動かしている間は `SetVelocity()` を止めてください(下の実例)。
- `Impulse`/`Force` は重さで割られるので、重いオブジェクトほど動きません。重さに関係なく決まった速さにしたい時は、`VelocityChange`/`Acceleration` を使うか、強さを「速さ × `GetMass()`」にします。

**実例(Sample2Dのノックバック)**: `PlayerController::OnCollisionStay2D` で、敵に当たってダメージを受けた時に、敵から離れる向き(`Math::DirectionTo`、XY平面)へ `AddForce(..., ForceMode::Impulse)` で弾いています。Playerは重い(密度100)ので、強さは「弾かれる速さ(`knockbackSpeed`)× `GetMass()`」にしています。弾かれている0.2秒間(`knockbackTimer`)は、移動キーでの `SetVelocity()` をしないようにして、弾かれた勢いを残しています。

### ワールド単位について

このエンジンの3Dワールドは、Cube/Sphereの`scale=100`が実寸1mくらいに相当する感覚で作られてきています。PhysXは実寸メートル基準で調整されたデフォルト値(重力・スリープ閾値など)を持っているため、`Physics::Init()`では重力を`(0, -981, 0)`(標準の9.81 m/s²を100倍したもの)に、`PxTolerancesScale.length`も`100`にしてあります(これを合わせないと、シミュレーションの精度・安定性が悪くなります)。今後PxRigidActorのサイズ等を決める時も、この「100単位=1m」のスケール感を意識してください。

### ビルド環境について(記録用)

このマシンではVisual Studio 2022しか入っておらず、PhysX 4.1の公式プリセットはVS2019(`vc16win64`)までしか無かったため、`vc17win64`プリセットを自作して生成しました(`physx/buildtools/presets/public/vc17win64.xml`と、`cmake_generate_projects.py`への`vc17`対応追加)。これらはPhysXのソース側(このリポジトリの外、`ThirdParty/PhysX/`には含まれていないビルド作業用のフォルダ)の変更なので、もし将来PhysXを更新・再ビルドする必要が出てきたら、同じ手順を踏んでください。

- `ThirdParty/PhysX/lib/x64-debug/`, `x64-release/`: Debug/Release、x64向けのビルド済み成果物(`.pdb`/`.map`や、GPU支援用のDLL(`PhysXGpu`/`PhysXDevice`)は容量削減のため含めていません。今回はCPUベースの物理演算のみを使う想定です)
- PhysXのinclude/lib設定は`PhysX.props`(ソリューションフォルダ直下)に1箇所にまとめてあり、`Engine.vcxproj`/`Sample2D.vcxproj`/`Sample3D.vcxproj`の全てがこれをImportしています。PhysXのバージョンを上げる、リンクするlibを増減する時はこのファイルだけ直せば全プロジェクトに反映されます。`RuntimeLibrary`も同じファイルで`MultiThreaded`/`MultiThreadedDebug`(静的CRT)に設定済みです。PhysXのビルドが`NV_USE_STATIC_WINCRT=True`だったため、CRTを合わせないとリンクエラーになります(新しいプロジェクトを追加する時も、必ず同じ`RuntimeLibrary`にしてください)

---

## 24. 音(Audio / AudioSourceComponent / AudioListenerComponent / グループ音量)

音を鳴らす仕組みです(`Engine/Audio.h`、`Engine/AudioSourceComponent.h`、`Engine/AudioListenerComponent.h`)。**音が「どこで鳴っているか」で聞こえ方が変わります**(遠い音は小さく、左で鳴った音は左から聞こえる)。同じ効果音を重ねて鳴らせるので、連射しても音が途切れません。BGMと効果音の音量は別々に変えられます。

### 使い方

```cpp
#include "../../../Engine/Audio.h"
#include "../../../Engine/AudioSourceComponent.h"
#include "../../../Engine/AudioListenerComponent.h"

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
- 両サンプルの `EnemyController::OnTriggerEnter2D()` が、敵を倒した時に `hit.wav` を倒された場所で鳴らす。
- 両サンプルの `Objects/Player.cpp` に `AudioListenerComponent`(Sample3Dは `SetUse3DCamera(true)` 付き)。

### 注意

- 位置の単位はワールド座標です(2Dはピクセル、3Dのサンプルは1000で地面のタイル1枚)。ゲームの広さに合わせて `SetDistance` で調整してください。`PlayOneShotAt` は既定の 300 / 2500 です。
- WAV は 16bit PCM(または8bit)を使ってください。フォーマットが違う音は同じ再生口を使い回せないので、種類が増えると再生口を作り直します(動きはしますが、1つのフォーマットにそろえたほうが軽いです)。

---

## 25. CSV(Csv)

敵の強さや出現の順番などを、コードではなく表(Excel)で調整するための仕組みです。表のファイル(`.csv`)を読んで、**列の名前**で値を取り出します。

```cpp
#include "../../../Engine/Csv.h"

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
  - `Enemies.csv`: 敵の速さを取り直し、**すでに出ている敵にも新しい速さを渡します**(`ChaseComponent::SetSpeed`。出ている敵は `ObjectPool::GetObjects()` で調べます)
  - `Waves.csv`: 経過時間はそのままにして、新しい表で「今の経過時間より前に出るはずだった回」を**出した扱い**にし、次に出す時間だけを決め直します。そのため、すでに出した分を二重に出さず、これから出す分も飛ばしません(例: 12.3秒の時に表を変えると、12.3秒より後の回から新しい表になります)。先に出ている敵はそのままです

### 実例: 敵の速さと出現ウェーブ(両サンプルの `EnemyManager`)

`Assets/Data/Enemies.csv`(敵の速さ):
```
Name,Speed
Slime,60
```
`EnemyManager::Init()` が `Name` が `Slime` の行の `Speed` を読み、敵に付けた `ChaseComponent` に渡します。

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

爆発・煙・火花などを、たくさんの小さな粒で出す仕組みです。粒1つ1つはGameObjectではなく、`ParticleRenderer`の中のデータ(位置・速さ・残り時間)で、まとめて描きます(数百個出しても重くなりにくい)。使うヘッダーは `Engine/ParticleRenderer.h`(`ParticleSettings`もここ)と `Engine/Effect.h`(`CreateEffect`)です。

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
| 描き方 | `SpriteRenderer`のUIモードと同じ(2Dカメラでスクロールする。奥行きで小さくなる) | `SpriteRenderer`のWorldモードと同じ。粒は**常にカメラの方を向く板(ビルボード)**で、手前の3Dオブジェクトに隠れる |
| `spreadAngle` | XY平面で向きを±(角度÷2)回す | 向きを軸にした円錐(360なら全方向の球) |
| 粒の位置 | `x, y`(`z`は奥行き) | `x, y, z` |

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

両サンプルの`EnemyController::OnTriggerEnter2D()`が、弾に当たって倒された位置で`CreateExplosion()`(`Game/Objects/Explosion.cpp`。Sample2DとSample3Dに、それぞれ別の設定で置いてある)を呼びます。`CreateExplosion`は、オレンジ色の加算合成の粒を全方向に出す`CreateEffect`+`Burst`です(Sample3Dは`use3D = true`で、重力で少し落ちる)。粒が消えるとエフェクトのオブジェクトは自動で消えます。Sceneには置きません(敵を倒した時に呼びます)。

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
| `Model::Load(パス)` / `GetNodeCount()` / `FindNode(名前)` | モデルのデータそのもの(`Engine/Model.h`)。普通は `ModelRenderer` が使うので、直接使うことは少ない |
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
- **新しいシェーダー**: `Shader3DSkin.hlsl`(`Shader3D.hlsl` と同じ場所。`Engine.vcxproj` の `CopyRuntimeFiles` が出力先へコピーします)と、描画クラス `Engine/SkinMesh.h/.cpp`、頂点形式 `SkinVertex`(位置・法線・色・UV + 骨の番号4つ + 影響度4つ)です。**今の `MeshVertex` / `Shader3D.hlsl` / `Shader.hlsl` は変えていません。** ライティング・色・テクスチャの計算は `Shader3D.hlsl` と同じです(今は両方が共通の `Shader3DCommon.hlsli` を使っています。§30)。シェーダーは実行時にコンパイルされ、エラーは出力ウィンドウに出ます。
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
| `IsCircleInView2D(位置, 半径)` | 2Dカメラ(`Image::GetCamera()`)の画面に、円が少しでも入っているか。位置はワールド座標(`z`は遠近の倍率に使う) |
| `IsRectInView2D(位置, 半分の大きさ)` | 同じく四角(回転しない) |
| `GetCulledCount()` | 前のフレームに、描かずに省いた数(HUDに出して確認する) |
| `SpriteRenderer` / `MeshRenderer` / `ModelRenderer` の `SetFrustumCulling(bool)` | 画面の外なら描かない。**既定は `false`(今までと同じ。全部描く)** |

**2D と 3D の違い**
- 3Dは「視錐台」と「球・箱」で調べます。視錐台は 1フレームに1回、`GameEngine` が `CameraShake::BeginDraw()` の後に作ります(揺れが入ったカメラで判定するので、画面の端で揺れても一瞬消えたりしません)。ゲーム側が準備を呼ぶ必要はありません。
- 2Dは「2Dカメラの画面(画面中央が(0,0)、`Y+`=上、単位はピクセル)」と「円・四角」で調べます。`SpriteRenderer`(UIモード)と同じ遠近の倍率(奥の物は小さく中心に寄って見える)を使います。2Dカメラは毎回その場の値を読むので、準備は要りません。
- どちらも、最初のフレーム(準備の前)は「映っている」を返します(消さない側に倒しています)。

**Rendererの画面外判定(`SetFrustumCulling(true)`)の決まり**
- `SpriteRenderer`: `transform.scale` の幅・高さの対角線の半分を半径にした円(UIモード)か球(`SetWorldSpace(true)` のビルボード)で調べます。`scale` が0以下で画像の大きさを使う時は、大きさが分からないので判定しません(必ず描く)。
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

3Dメッシュの見た目の設定(色・画像・光沢・自発光・半透明)を、1つの `Material`(`Engine/Material.h`)にまとめました(`docs/EngineRoadmap2.md` の 6.3)。`MeshRenderer` と `ModelRenderer` が1体ごとに持ちます。**何も設定しなければ、マテリアルが無かった頃と見た目は同じです**(ハイライトなし・自発光なし)。

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

- **光沢**は、光とカメラの真ん中の向きに法線が近いほど光る計算(ブリン・フォン)です。光の色(`Image::GetLight()` の `color`)も掛けます。`lit = false` の時は出ません。
- **自発光**は、光の計算の後に色を足すだけです。画面全体ににじむ本物の「光る感じ」(ブルーム)ではありません。
- ライト(ポイントライト・スポットライト)・影・縁の光・枠線の項目は、それぞれの機能(`docs/EngineRoadmap2.md` の 6.4〜6.6)を作る時に `Material` へ足します。

### 半透明にする時

`color.a` を1より小さくすれば半透明になります(ブレンドは前から有効です)。ただし次の点に気を付けてください。**自動では何も変えません**(`color.a` を変えるだけで `depthWrite` や `drawPriority` を勝手に変えることはしません)。

```cpp
panel->SetDrawPriority(10);          // 不透明な物より後に描く(大きいほど後)
material.color.w = 0.4f;             // 透明度
material.depthWrite = false;         // 自分より後ろの物を隠さない
```

- 描く順番は `drawPriority` → `z` の小さい順で、**カメラからの距離順ではありません**。半透明の物は、不透明の物より後に描くように `SetDrawPriority` を大きくしてください。
- 半透明の物どうしの前後は、見る角度によってずれることがあります(距離で並べ替える処理が無いため)。
- `clip` で、ほぼ透明なピクセルは今まで通り描きません。

### Mesh の直接の使い方(Engineを触る人向け)

`Mesh::Draw` / `SkinMesh::Draw` に、`Material` を受け取る版があります。今までの `color` を渡す版も、既定のマテリアルで新しい版を呼ぶだけなので、そのまま使えます(`RigidbodyComponent` の枠線表示や `GroundController` は変えていません)。

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

`Shader3D.hlsl`(普通のメッシュ)と `Shader3DSkin.hlsl`(骨で曲げるFBX)で、見た目の計算(光・色・テクスチャ・光沢・自発光)は、共通の `Shader3DCommon.hlsli` の `ShadePixel()` に書いてあります。**見た目の計算を変える時は、ここだけを直せば、2つのシェーダーが同じ結果になります。**

| レジスタ | 中身 |
|---|---|
| `b0` | ワールド・ビュー・射影・平行光源(今まで通り) |
| `b1` | 骨の行列(`Shader3DSkin.hlsl` だけ。色は `b2` に移した) |
| `b2` | マテリアル(色・光沢・自発光・光を使うか) |
| `b3` | カメラの位置(光沢のため。ライトの機能を作る時にここへ足す) |
| `t0` / `s0` | 画像 / サンプラー |

- シェーダーは実行時にコンパイルされるので、`Shader3DCommon.hlsli` も実行ファイルの隣に必要です。`Engine.vcxproj` の `CopyRuntimeFiles`(コピー)と `UpToDateCheckInput` に登録済みです(**新しい `.hlsli` を足したら、この2か所にも足すこと**)。`#include` が使えるよう、`D3DCompileFromFile` には `D3D_COMPILE_STANDARD_FILE_INCLUDE` を渡しています。
- 書き方に間違いがあると、何も描かれなくなり、出力ウィンドウにエラーが出ます。

### Sampleでの使われ方

- **Sample3D の `GlassPanel`**(`Objects/GlassPanel.cpp`): 半透明(`color.a = 0.4`・`depthWrite = false`・`SetDrawPriority(10)`)で、光沢と少しの自発光がある水色の板です。位置 `(400, 50, 300)`。
- **Sample3D の `Sphere`**(`Objects/Sphere.cpp`。今のシーンでは未使用): `specularStrength = 0.8`・`shininess = 64` でつやつやの球になります。
- **`Skybox`** は今まで通り `SetUnlit(true)` / `SetDepthWrite(false)`(中でマテリアルの項目を書き換える)。
- 当たり判定のデバッグ表示(Sample2Dにも出る枠線)や、`GroundController` のタイルは、`Mesh::Draw` の色指定版を通ります。見た目は今まで通りです。

### まだ無いもの

- FBXに書かれたマテリアル(スペキュラ・自発光)の読み込みはしません(今は色と画像だけ)。
- マテリアルをファイルにして使い回す仕組みはありません(1体ごとに持ちます)。
