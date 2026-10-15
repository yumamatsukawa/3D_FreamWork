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
├── Assets\             全プロジェクト共有の画像・音声・モデル
├── Shader.hlsl / Shader3D.hlsl   全プロジェクト共有のシェーダー
├── Engine\             フレームワーク本体(静的ライブラリ、Engine.vcxproj)
├── Sample2D\           2Dのサンプルゲーム(実行ファイル、Sample2D.vcxproj)
└── Sample3D\           3Dのサンプルゲーム(実行ファイル、Sample3D.vcxproj)
```

- **Engine**: `GameObject`/`Component`/`RigidbodyComponent`/描画/音声などフレームワーク本体。それ単体では実行できない静的ライブラリ(`Engine.lib`)で、基本的にみんなで触る場所ではありません。
- **Sample2D** / **Sample3D**: それぞれ独立した実行ファイル。どちらも同じ`Engine.lib`をリンクしているので、**Engineを直した内容は、両方のサンプルを再ビルドするだけで反映されます**。中身の`Game\Objects\`/`Game\Components\`/`Game\Scenes\`は、後述の「2. オブジェクトの作り方」と同じパターンです。
- ビルド後、`Engine.lib`/`Sample2D.exe`/`Sample3D.exe`は全て同じ`x64\Debug\`(`Release\`)フォルダに出力されます。`Assets\`とシェーダーは、Engineのビルド後処理(PostBuildEvent)でそこに自動コピーされるので、`.exe`をダブルクリックするだけでも動きます。
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

画像そのものは、同じ画像を使うオブジェクトみんなで共有しています(§11)。コマ番号(`SetSpriteIndex`)と色(`SetColor`)は**1体ごと**なので、同じ`player.png`を使うPlayerと敵で、色やコマが連動することはありません。

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

**内部実装メモ**: シェーダー・サンプラー・ブレンド/深度/ラスタライザステート・定数バッファは、`Mesh`クラス全体で1つだけ作成して全インスタンスで共有しています(Cube/Sphere/Skyboxを何体作っても、シェーダーのコンパイルなどは初回の1回だけ)。頂点バッファ(形状データ)だけがインスタンスごとに別です(テクスチャは`Resources`で同じ画像どうし共有しています。§11)。特に意識する必要はありませんが、`Engine/Mesh.h/.cpp`を触る時は「インスタンスごとの値」と「共有する値(static)」を混同しないよう注意してください。

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
meshRenderer->SetUnlit(true);
```

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
| `Input::GetMouseWorldPosition()` | ワールド座標(2D用カメラの位置を考慮済み、Y+=上方向) | クリック判定・マウスの方を向くなど、2Dオブジェクトと比較する時(Sample2Dの`PlayerController`/`ButtonController`が実例) |
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

**当たり判定は、2D/3D問わず全てPhysX(`RigidbodyComponent`、§19参照)で計算しています。**2Dのゲームでは、2D向けに使いやすくした`SquareRigidbodyComponent`/`CircleRigidbodyComponent`を使い、`SetFreezePositionZ(true)`で奥行き方向に動かないようにします(Sample2Dが実例)。

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

2D向けのSquare/Circleは、PhysX上は奥行きのある立方体/球ですが(§19)、2Dゲームでは奥行き方向に動かないので、平面で見た時の形(断面)だけを表示しています。

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
// 一定間隔でくり返す(Sample2D/3D EnemyManager)
Timer spawnTimer{ 2.5f, true };                      // 2.5秒ごと
spawnTimer.elapsed = 2.5f - 1.0f;                    // 最初の1回だけ1.0秒で来るようにする(Initで)
if (spawnTimer.Update(dt)) Spawn(...);               // 時間が来たフレームだけ出す

// きっかけがあってから○秒の間(Sample2D/3D PlayerControllerの無敵時間)
Timer invincibleTimer{ 1.0f };
invincibleTimer.Complete();                          // Init: 最初は無敵ではない(終わった状態)
invincibleTimer.Update(dt);                          // Update: 毎フレーム進める
if (!invincibleTimer.IsFinished()) return;           // 無敵中はダメージを受けない
invincibleTimer.Reset();                             // ダメージを受けたら数え始める

// 寿命(Sample2D/3D BulletController)
if (lifeTimer.Update(dt)) pool->Return(GetOwner());  // 2秒たったらプールに返す

// コマ送りのアニメーション(Sample3D EnemyController)
if (frameTimer.Update(dt)) frameIndex = (frameIndex + 1) % walkFrameCount;   // 0.3秒ごとに次のコマへ
```

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
- 音(`Audio`)とモデルの使い回しは、まだありません(音は「データ」と「再生口」を分ける時に一緒に作る予定。`docs/EngineRoadmap.md` の 5.13 / 5.16)。

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

**実例**: 両サンプルのタイトル画面のSTARTボタン(`Game/Objects/StartButton.cpp`)は、大きさ0から本来の大きさへ `Ease::OutBack` で0.4秒かけて大きくなって出ます。ボタンのクリック判定(`ButtonController`)も `transform.scale` を使っているので、大きくなる途中でも見た目と判定が合っています(ボタンの文字の大きさも、ボタンの高さに合わせて一緒に大きくなります)。

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
- `AccelerateChaseComponent` は、Dynamicなら `AddForce(..., ForceMode::Acceleration)`(§19)で加速します(ぶつかって跳ね返った勢いも残る)。1フレーム分の加速だけ、最高速度を少し超えることがあります。

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

**実例**: 両サンプルの`ButtonController::Draw()`が、ボタンの文字の位置を`Image::WorldToScreen2D`で求めています。

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

## 17. 開発時の注意点

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

`Audio::Load()`(`Engine/Audio.cpp`)は`.wav`ファイルの`fmt `/`data`チャンクを名前で探して読みます。以前は「`fmt `の直後は必ず`data`」という決め打ちの44バイト固定ヘッダーで読んでいたため、ffmpeg/Lavfなど一部のツールで書き出したWAV(`fmt `と`data`の間に`LIST`メタデータチャンクが挟まる)を読み込むと、メタデータの文字列を音声データと誤認識し、音が鳴らない(鳴ってもごく短いノイズだけ)という不具合がありました。現在は汎用的なチャンク読み取りに直してあるので、`LIST`/`fact`など余計なチャンクが挟まっていても正しく読めます。**それでも音が鳴らない場合は、まず`.wav`ファイル自体が壊れていないか(別プレイヤーで再生できるか)を確認してください**。

---

## 18. サンプル構成(現状)

プロジェクト構成自体は`0. プロジェクト構成`を参照してください。ここでは各サンプルの中身(`Game/`)を説明します。

```
Sample2D/Game/Objects/     Title, Button, StartButton, Background, Player, GameSystem, HUD,
                            EnemySpawner, BulletSpawner
Sample2D/Game/Components/  TitleController, ButtonController, BackgroundController, PlayerController,
                            BulletController, EnemyController, GameManager, HUDController,
                            EnemyManager, BulletManager
Sample2D/Game/Scenes/      TitleScene, GameScene

Sample3D/Game/Objects/     Title, Button, StartButton, Skybox, Ground, Player, GameSystem, HUD,
                            EnemySpawner, BulletSpawner, Cube(未使用), Sphere(未使用)
Sample3D/Game/Components/  TitleController, ButtonController, GroundController, PlayerController,
                            CameraController, BulletController, EnemyController, GameManager,
                            HUDController, EnemyManager, BulletManager, SkyboxFollowComponent,
                            SpinComponent(未使用)
Sample3D/Game/Scenes/      TitleScene, GameScene
```

- **Sample2D**: トップダウン視点のアリーナシューター。タイトル画面のSTARTボタン(出る時に`TweenComponent`でぴょこっと大きくなる)をクリックすると、画面が暗転してからゲーム開始(ゲーム画面に入ると明転する。`FadeTransition`。§16)(ボタンは`CreateButton(scene, 文字, 位置, クリックされた時の処理)`で作れる汎用部品なので、他の画面のボタンにも使い回せる)。WASDで移動、Playerは常にマウスの方を向き、左クリックでその方向へ弾を発射。背景はタイル状に敷き詰めた床(`BackgroundController`)で、移動するとスクロールする(カメラはPlayerに付けた`CameraFollowComponent`が追いかける。§14)。敵(`EnemyManager`が一定間隔でPlayerの周囲にランダムスポーンさせる)がゆっくり追いかけてきて(`ChaseComponent`。§13)、弾を当てると倒せる(`EventBus`で`"EnemyDefeated"`イベントを発行しスコア加算)。敵に触れるとHPが減り(1秒間の無敵時間つき)、敵から離れる向きに弾かれる(ノックバック。`AddForce`で衝撃を加え、0.2秒間は移動キーが効かない。§19)、画面が少し揺れて(`CameraShake`。§14)、画面の端が赤く点滅する(`ScreenEffect::Vignette`。§15)。HPが0になるとゲームオーバー画面が出てSPACEでタイトルに戻る(黒い丸が閉じて開く`SpriteTransition`。§16)。当たり判定・物理はPhysXを使い(Player=`CircleRigidbodyComponent`、敵=`SquareRigidbodyComponent`、弾=`CircleRigidbodyComponent`)、Player・敵はDynamic(`SetVelocity()`で速度を渡して動かす。重力なし・回転なし・`SetFreezePositionZ(true)`で奥行き方向に固定)、弾はKinematicのトリガー。3Dカメラは使わない(`CameraController`なし)。
- **Sample3D**: Sample2Dと同じアリーナシューターを3D空間で遊べるようにしたもの。タイトル画面のボタン(Sample2Dと同じく、出る時に`TweenComponent`でぴょこっと大きくなる)をクリックすると、Sample2Dと同じく暗転してからゲーム開始(Sample3Dの`CreateButton()`は、位置と大きさをまとめて`Transform`で受け取る形になっている: `CreateButton(scene, 文字, transform, クリックされた時の処理)`)。Player(立方体)は三人称カメラ(`CameraController`。マウス移動で視点回転。カーソルは画面中心に固定され、ESCで解除・左クリックで再固定。TABで一人称に切り替え)と同じ向きを向き、WASDもカメラ基準(W=カメラの前、D=カメラの右)で移動する。左クリックでPlayerの向いている方向へ、地面と水平に弾(球)を発射。敵は2Dの画像(Worldモードの`SpriteRenderer`。Y軸だけカメラの方を向く立て看板)で、地面の上をPlayerに向かって歩いてくる(`ChaseComponent`を`SetPlane(MovePlane::XZ)`で使う。§13)。歩く間は、`EnemyController`がタイマーでスプライトシートのコマ(4番と5番)を0.3秒ごとに切り替えて、歩きアニメーションにしている。スコア・HP・無敵時間・被弾時に画面の端が赤く点滅すること・ゲームオーバー(SPACEでタイトルへ。Sample3Dは、黒い帯が左から覆って右へ抜けていく`SpriteTransition(...).SetSwitchFrame(7)`。§16)・`EventBus`のイベント名はSample2Dと同じ。Player・敵はDynamicの`BoxRigidbodyComponent`(重力あり・回転なし。Y速度は`GetVelocity().y`を維持したまま`SetVelocity()`で動かす。敵は`ChaseComponent`がこれをしている)、弾はKinematicのトリガー(`SphereRigidbodyComponent`)。地面は、1000四方のタイルを3Dカメラの描画距離(farZ)の範囲だけ敷き詰めて描く`GroundController`(見た目)と、見えない大きなStaticの板(当たり判定)の組み合わせ。遠景はスカイボックス。

**Sample3Dの実装メモ**: 向きはすべて`transform.rotate.y`(度)で表し、`rotate.y`の時の正面(ローカルの+Z)は`(sin, 0, cos)`の方向になる(`CameraController::GetYaw()`の式と同じ。`Math::Forward3D()`で求められる)。Worldモードの`SpriteRenderer`は`rotate.y`を使わない(ビルボードの向きとZ軸回転だけを使う)ので、敵の画像の向きは`SetBillboardLock(true, false, true)`でカメラの方へ向けている。Playerは`SetFreezeRotation(true)`で、`RigidbodyComponent`は位置しか同期しないため、`PlayerController`が`rotate.y`が変わった時に`GetActor()`の`setGlobalPose`で当たり判定の箱の回転も合わせている(デバッグ表示の枠と実際の判定が同じ向きになる)。一人称の間は、カメラが立方体の内側に入るのでPlayerの`MeshRenderer`を非表示にしている。

**Sample2Dの実装メモ**: `EnemyManager`/`BulletManager`はどちらも`ObjectPool`を使った使い回し方式(実例は`Engine/ObjectPool.h`)。プールで使い回すオブジェクトは`Scene::CreateObject("Pooled")`で名前だけ付けて作られるため、`GameObject::SetTag()`で組み立て時にタグ("Enemy"/"Bullet")を後付けしている。敵を倒す/Playerがダメージを受ける判定は、それぞれ`EnemyController::OnTriggerEnter2D`(相手のタグが"Bullet")と`PlayerController::OnCollisionStay2D`(相手のタグが"Enemy")が担当する。

役割分担は「値の持ち主は1か所、他は読むだけ」にしている:

| Component | 持っている値・担当 |
|---|---|
| `PlayerController` | HP・無敵時間・ノックバック(HPが0になったら`EventBus`で`"PlayerDied"`を発行し、以後は移動・向き・発射を受け付けない。ポーズ中・シーン切り替え中も`Time::IsPaused()`で受け付けない) |
| `GameManager` | スコア(`"EnemyDefeated"`で加算)・ゲームオーバー判定(`"PlayerDied"`を受けて)・タイトルへの遷移。表示は一切しない |
| `EnemyManager` | 敵のプールと出現タイマー。一定間隔でPlayerの周囲に敵を出し(追いかける動きは敵に付けた`ChaseComponent`に、相手と速さを渡す)、`"PlayerDied"`を受けたら出現を止める |
| `BulletManager` | 弾のプール。`"FireBullet"`を受けて、撃った本人の向きへ弾を出す |
| `HUDController` | 画面表示(Score/HP/GAME OVER)だけ。値は自分で持たず、`GameManager::GetScore()`/`PlayerController::GetHp()`などで毎フレーム読む |

表示用に値のコピーを別に持つと、本物の値とズレる原因になるため、HUDは読むだけにしている。新しいUI要素を足す時は`HUDController::Draw()`に追加し、表示したい値は持ち主側にゲッターを生やす形にすること。

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
| 一定間隔で敵を出す | `EnemyManager::Update()` |
| 敵がPlayerを追いかける | 敵に付けた`ChaseComponent`(Engine。§13)。相手と速さは`EnemyManager::Spawn()`が渡す |
| 敵に当たったら弾かれる(Sample2D) | `PlayerController::OnCollisionStay2D()`(`AddForce`。§19) |
| 敵に当たったら画面が揺れる(Sample2D) | `PlayerController::OnCollisionStay2D()`(`CameraShake::Shake`。§14) |
| 敵に当たったら画面の端が赤く点滅する | `PlayerController::OnCollisionStay2D()`(`ScreenEffect::Vignette`。§15) |
| カメラがPlayerを追いかける(Sample2D) | `Objects/Player.cpp`で付けた`CameraFollowComponent`(Engine。§14) |
| コライダー枠の表示ON | `GameManager::Init()` |

Sceneに残してよいのは、オブジェクトの配置と、シーン全体の後片付け(`GameScene::Uninit()`の`EventBus::Get().Clear()`)くらい。新しい処理を足したくなったら、まず「どのオブジェクトの仕事か」を考えて、そのComponentに書くこと。

どちらも現時点では「動く土台」で、ゲームとしての作り込み(敵の挙動、演出、レベルデザインなど)はこれから追加していく想定です。新しいオブジェクトを追加する時は、それぞれのサンプルの`Game/Objects/`と`Game/Components/`に、既存のPlayer/Enemyと同じ形式でファイルを足していくのが基本の流れです。

---

## 19. PhysX(3D物理演算)

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
