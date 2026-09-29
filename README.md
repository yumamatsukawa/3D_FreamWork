# 3D_FreamWork

DirectX11製の、Unity風のコンポーネント指向ゲームフレームワーク。2D(スプライト)と3D(メッシュ)の両方に対応しています。

このドキュメントは、これから機能を追加していくチームメンバー向けの使い方ガイドです。

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

**内部実装メモ**: シェーダー・サンプラー・ブレンド/深度/ラスタライザステート・定数バッファは、`Mesh`クラス全体で1つだけ作成して全インスタンスで共有しています(Cube/Sphere/Skyboxを何体作っても、シェーダーのコンパイルなどは初回の1回だけ)。頂点バッファ(形状データ)とテクスチャだけがインスタンスごとに別です。特に意識する必要はありませんが、`Engine/Mesh.h/.cpp`を触る時は「インスタンスごとの値」と「共有する値(static)」を混同しないよう注意してください。

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

`position.y`は2D・3D共通で**上方向が+**です(標準的な数学と同じ、Mesh/Sprite/当たり判定すべて統一)。マウス座標(`Input::GetMousePosition()`)だけはOS標準のY+=下方向のままなので、ワールド座標と比較する時は符号を反転させてください(`PlayerController.cpp`のクリック判定が実例です)。

**2D/3Dのスクロール速度について**: 2Dは疑似的な正射影(距離に関係なく一定速度でスクロール)、3D(Mesh)は本物の透視投影(近いものほど速く、遠いものほどゆっくり動く)なので、根本的に仕組みが違います。`Camera.focalLength`(既定500)の距離にある3Dオブジェクトだけは、2Dと同じ速度で動くように`fovY`を自動調整していますが、それ以外の距離にあるオブジェクトは2Dとズレます(これは正しい遠近感なので、バグではありません)。

### CameraController(三人称/一人称カメラ)

`Game/Components/CameraController.h/.cpp`は、Ownerを追いかける3D専用のカメラです。`Player.cpp`で実例としてアタッチしています。

```cpp
player->AddComponent<CameraController>();
```

- `TAB`キー: 三人称 ⇔ 一人称を切り替え
- 右クリックを押しながらマウスを動かす: 視点を回転

`Image::GetCamera3D()`を直接操作するため、有効な間は`Image::BeginFrame()`による2Dカメラへの自動追従(前述)を自動で止めます(`Image::SetCamera3DAutoSync(false)`。Component破棄時に自動でtrueへ戻ります)。MeshRendererだけでなく、Worldモードの`SpriteRenderer`(ビルボード)もこのcamera3Dを使うので、両方とも同じカメラの動きに正しく追従します。**UIモードの`SpriteRenderer`(HUDなど)は画面に固定されたままで、影響を受けません。**

---

## 6. Collider(当たり判定)

**当たり判定は、2D/3D問わず全てPhysX(`RigidbodyComponent`)で計算しています。**以前は自作の2D数式(AABB/円のSAT判定)を使っていましたが、Player/Enemy/Bulletを含めて`BoxRigidbodyComponent`/`SphereRigidbodyComponent`(§11参照)に統一しました。Player/Enemyのような「3D空間にいる2Dオブジェクト」は、Z軸方向に厚みを持たせた3D形状(薄い箱/球)として扱っています。

```cpp
// Playerの例(実際にすり抜けない円形。Dynamic+速度指定で動かし、衝突応答はPhysXに任せる。§11参照)
player->AddComponent<SphereRigidbodyComponent>(BodyType::Dynamic, 50.0f, 1.0f);

// Bulletの例(すり抜ける円形。Enemyに当たったことだけ分かればよい)
bullet->AddComponent<SphereRigidbodyComponent>(BodyType::Kinematic, 8.0f, 1.0f,
    /*isTrigger*/ true);
```

反応する側は、Componentに以下をoverrideします(この部分はPhysX移行前と同じAPIのままです):

```cpp
// isTrigger = true の時
void OnTriggerEnter2D(Collider2D* other) override;
void OnTriggerExit2D(Collider2D* other) override;
// ※ OnTriggerStay2Dは今のところ呼ばれません(PhysXのトリガーにはStay相当の通知が無いため)

// isTrigger = false の時(押し返しも自動で行われる)
void OnCollisionEnter2D(CollisionInfo info) override;  // info.otherで相手のColliderが取れる
void OnCollisionStay2D(CollisionInfo info) override;
void OnCollisionExit2D(CollisionInfo info) override;
```

相手のタグは `other->GetTag()` (Trigger) / `info.other->GetTag()` (Collision) で判定します(`Collider2D`は当たり判定の計算自体は持たない、コールバック用の軽いハンドルになりました)。

**内部実装メモ**: `Engine/Physics.cpp`のシミュレーションコールバック(`onContact`/`onTrigger`)が、PhysXの結果をこれらのコールバックに変換しています。Kinematicなオブジェクト同士がぶつかった時の「押し戻し」は、PhysXの自動計算に任せられない(キネマティックは力の影響を受けないため)ので、接触の法線・めり込み量から手動で`Transform.position`をずらしています(`isStaticForPush=true`側は動かさない)。Dynamicなオブジェクト(Sphereなど)が絡む場合は、PhysX自身が正しく押し返すので、この手動処理はスキップされます。

**CCD(連続衝突判定)について**: `SquareRigidbodyComponent`のような奥行きの薄い箱同士が角(コーナー)でかすめるように当たると、通常の当たり判定(1フレームごとの位置だけを見る離散判定)では押し返しが弱く、貫通してしまうことがあります。これを防ぐため、Dynamicなアクター全てでCCD(移動の軌跡をスイープして判定するモード)を有効にしています(`Physics::Init()`でシーン全体のCCDを有効化、`RigidbodyComponent.cpp`の`CreateActor()`でDynamicアクターごとに`PxRigidBodyFlag::eENABLE_CCD`を設定)。

### Colliderの見た目(デバッグ表示)

実際にPhysXが使っている当たり判定の形・大きさを、ワイヤーフレームで可視化できます(見た目のスプライト/メッシュのサイズとズレていないか確認する時に便利です)。

```cpp
RigidbodyComponent::SetDebugDrawEnabled(true);
```

`Scene::Init()`などで1回呼ぶだけで、以後その回のプレイ中は全ての`RigidbodyComponent`(Box/Sphereどちらも)が緑(通常の当たり判定)または黄(`isTrigger=true`のすり抜ける判定)のワイヤーフレームを、実際のPhysX形状のサイズで描画します。不要になったら`false`を渡すか、呼び出し自体を消してください。

**内部実装メモ**: `RigidbodyComponent::Draw()`が、`PxShape`から実際のジオメトリ(`PxBoxGeometry`/`PxSphereGeometry`)を取得し、Ownerの`Transform`の`scale`だけを一時的に差し替えて`Mesh::Draw(..., wireframe=true)`で描画しています(位置・回転は普段どおりOwnerのTransformをそのまま使う)。ワイヤーフレーム用の単位立方体/単位球メッシュは、全`RigidbodyComponent`で1つずつ共有しています。

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

弾やエフェクトなど、頻繁に生成・消滅するものに使います(`Game/Objects/BulletManager.cpp`が実例)。

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

**ObjectPoolは、必ずそれを使うSceneのメンバとして持たせてください**(`GameScene::bulletManager`を参照)。ファイルのstatic変数のように、シーンをまたいで生かし続けると、シーンが破棄された後もプール内に「もう存在しないGameObjectへのポインタ」が残り、クラッシュします。

---

## 9. 開発時の注意点

### 文字コード(重要)

日本語コメントを含む`.h`/`.cpp`ファイルは、**UTF-8 BOM付き**で保存してください。BOM無しだと、日本語ロケールの環境でコンパイラが文字化けを起こし、意味不明な構文エラーが大量に出ます。Visual Studioで保存する分には自動的にBOM付きになりますが、他のエディタを使う場合は明示的に選んでください。

### 新しいファイルを追加したら

追加したファイルが属するプロジェクト(Engineに追加するなら`Engine.vcxproj`、Sample2D用のGameコードなら`Sample2D.vcxproj`、Sample3D用なら`Sample3D.vcxproj`)の、`.vcxproj`と`.vcxproj.filters`の両方に新規ファイルを追加してください(Visual Studioでソリューションエクスプローラーから該当プロジェクトを右クリック→「追加」すれば自動でやってくれます)。

### Update中にオブジェクトを生成してよい

`Scene::CreateObject()`はUpdate処理中に呼んでも安全です(内部で一時リストに貯めてから、ループの外でまとめて追加しています)。

### 音が鳴らない時は(WAVファイルの中身を疑う)

`Audio::Load()`(`Engine/Audio.cpp`)は`.wav`ファイルの`fmt `/`data`チャンクを名前で探して読みます。以前は「`fmt `の直後は必ず`data`」という決め打ちの44バイト固定ヘッダーで読んでいたため、ffmpeg/Lavfなど一部のツールで書き出したWAV(`fmt `と`data`の間に`LIST`メタデータチャンクが挟まる)を読み込むと、メタデータの文字列を音声データと誤認識し、音が鳴らない(鳴ってもごく短いノイズだけ)という不具合がありました。現在は汎用的なチャンク読み取りに直してあるので、`LIST`/`fact`など余計なチャンクが挟まっていても正しく読めます。**それでも音が鳴らない場合は、まず`.wav`ファイル自体が壊れていないか(別プレイヤーで再生できるか)を確認してください**。

---

## 10. サンプル構成(現状)

プロジェクト構成自体は`0. プロジェクト構成`を参照してください。ここでは各サンプルの中身(`Game/`)を説明します。

```
Sample2D/Game/Objects/     Title, Player, EnemyManager, BulletManager
Sample2D/Game/Components/  TitleController, PlayerController, BulletController,
                            EnemyController, GameManager
Sample2D/Game/Scenes/      TitleScene, GameScene

Sample3D/Game/Objects/     Title, Player, Cube, Sphere, Ground, Skybox
Sample3D/Game/Components/  TitleController, PlayerController, CameraController,
                            SpinComponent(未アタッチ), SkyboxFollowComponent
Sample3D/Game/Scenes/      TitleScene, GameScene
```

- **Sample2D**: トップダウン視点のアリーナシューター。WASDで移動、SPACEで弾を発射。敵(`EnemyManager`が一定間隔でPlayerの周囲にランダムスポーンさせる)がゆっくり追いかけてきて、弾を当てると倒せる(`EventBus`で`"EnemyDefeated"`イベントを発行しスコア加算)。敵に触れるとHPが減り(`"PlayerHit"`イベント、1秒間の無敵時間つき)、0になるとゲームオーバー画面が出てSPACEでタイトルに戻る。スコア・HPは`GameManager`(専用のGameObjectにAddComponentしたもの)がHUDとして`Text::Draw`で描画している。重力・3Dカメラは使わない(`SetUseGravity(false)`、`CameraController`なし)。
- **Sample3D**: 3人称/1人称カメラで動き回れる3D空間のショーケース。WASDで移動、TABでカメラ切り替え、右クリックドラッグで視点回転。Ground/Cube/Skyboxを配置し、Sphereが重力で落下してGroundの上に着地する。

**Sample2Dの実装メモ**: `EnemyManager`/`BulletManager`はどちらも`ObjectPool`を使った使い回し方式(実例は`Engine/ObjectPool.h`)。プールで使い回すオブジェクトは`Scene::CreateObject("Pooled")`で名前だけ付けて作られるため、`GameObject::SetTag()`で組み立て時にタグ("Enemy"/"Bullet")を後付けしている。敵を倒す/Playerがダメージを受ける判定は、それぞれ`EnemyController::OnTriggerEnter2D`(相手のタグが"Bullet")と`PlayerController::OnCollisionStay2D`(相手のタグが"Enemy")が担当し、実際にスコア・HPを変化させるかどうかの判断(無敵時間中かどうかなど)は`EventBus`経由で`GameManager`に一任している(お互いを直接知らなくて済むようにするため)。

どちらも現時点では「動く土台」で、ゲームとしての作り込み(敵の挙動、演出、レベルデザインなど)はこれから追加していく想定です。新しいオブジェクトを追加する時は、それぞれのサンプルの`Game/Objects/`と`Game/Components/`に、既存のPlayer/Enemyと同じ形式でファイルを足していくのが基本の流れです。

---

## 11. PhysX(3D物理演算)

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

### RigidbodyComponent(3Dの物理演算でオブジェクトを動かす)

`Engine/RigidbodyComponent.h/.cpp`に、`BoxRigidbodyComponent`/`SphereRigidbodyComponent`(3D向け)と`SquareRigidbodyComponent`/`CircleRigidbodyComponent`(2D向け)があります。見た目(`MeshRenderer`/`SpriteRenderer`)とは別に、動き方だけを担当するComponentです。4つとも共通の基底クラス`RigidbodyComponent`を継承しているので、他のComponentから形状を意識せず`GetComponent<RigidbodyComponent>()`で探せます(`PlayerController`が実例。これにより、Playerの形状を変えても`Player.cpp`の1行を変えるだけで済みます)。

- `Square`/`CircleRigidbodyComponent`は、中身は`Box`/`SphereRigidbodyComponent`と全く同じ(PhysXに2D専用の形状は無いため、奥行きの薄い箱・普通の球で代用している)。トップダウン2Dゲームで使う時に、`XMFLOAT3`のサイズではなく一辺の長さ/半径を1つ渡すだけで済む、より簡単なAPIになっている
- `SquareRigidbodyComponent`の奥行き(Z)は1.0固定です。厚みを自分で決めたい場合は`BoxRigidbodyComponent`を使ってください

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
- 実例: `Game/Objects/Sphere.cpp`(Dynamic)、`Game/Objects/Ground.cpp`(Static)、`Game/Objects/Enemy.cpp`/`BulletManager.cpp`(Kinematic)、`Game/Objects/Player.cpp`(Dynamic)

**内部実装メモ**:
- Dynamic: 毎フレーム、PhysXのシミュレーション結果(`PxRigidActor`の姿勢)を`Transform`に書き戻す。ただし`SetFreezeRotation(true)`で回転をロックしている間は、位置だけを反映する(回転は自分のコードに任せる。詳細は次項)
- Kinematic: 毎フレーム、`Transform`の現在値(PlayerControllerなどが書き換えた位置)を`setKinematicTarget()`でPhysXへ渡す。次の`Physics::Update()`でそこまで動き、他のDynamicな物体を正しく押せるようになる
- DirectX(左手系)とPhysX(右手系)は回転の向きが逆になるため、変換をかけている(位置はそのままでよい)。この変換は理屈の上では正しいはずですが、**回転がPhysXと逆向きに見えないか、実際に転がるオブジェクト(直方体など)で目視確認してください**。おかしければ`ToPxQuat`/`SyncTransformFromActor`のX/Y反転の箇所を疑ってください

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
- `SetFreezeRotation(true)`にしてあるので、PhysXの回転はTransform.rotateへ反映されない。そのため見た目の向き(`rotate.z`、Q/Eキーで操作・弾の発射方向にも使う)は、物理側に上書きされることなく`PlayerController`が直接書き換えたまま維持される(`SetFreezeRotation`は「ぶつかっても回転しない」かどうかと、「物理の回転をTransformへ反映するか」を同時に切り替える。詳しくは`RigidbodyComponent.h`のコメント参照)
- **コンポーネントの追加順に注意**: `SphereRigidbodyComponent`は`PlayerController`より先に`AddComponent`すること。Componentは追加順に`Update()`されるため、逆にすると`PlayerController`のカメラ追従処理が1フレーム古い位置を読んでしまう
- `SetVelocity()`/`SetUseGravity()`/`SetFreezeRotation()`/`SetFreezePositionY()`は全て`Kinematic`/`Static`では何もしません(`Dynamic`専用)。PhysXの生のAPIをもっと細かく触りたい時だけ`GetActor()`を使ってください
- `ObjectPool`で使い回されるオブジェクト(Bulletなど)は、`SetActive()`が呼ばれると`OnActiveChanged()`経由でPhysXのシーンから着脱される。再度有効化された直後は、前回位置からの掃引(スイープ)で誤ったヒット判定が出ないよう、次の`Update()`で`setKinematicTarget()`ではなく直接`setGlobalPose()`でテレポートするようにしている

**重要な注意(ハマりやすいポイント)**: `OnTriggerEnter2D`/`OnCollisionEnter2D`などのコールバックは、PhysXのシミュレーションコールバックの中から呼ばれています。PhysXは「コールバックの中でシーンを書き換えるAPI(`scene->addActor()`/`removeActor()`など)を直接呼んではいけない」という制約を持っているため、これらのコールバック内(や、そこから呼ばれる`ObjectPool::Return()`のような処理)で`SetActive(false)`する場合は要注意です。`RigidbodyComponent`はこれに対応済み(`Physics::QueueSceneChange()`で安全なタイミングまで遅延させている)ですが、**今後もし`OnTriggerEnter2D`などの中から新しくPhysXのシーンを直接触るコードを書く場合は、同じように`Physics::QueueSceneChange()`を使うか、フラグを立てて次のフレームで処理するようにしてください**。直接呼ぶと`Concurrent API write call ... during a callback function are not permitted`というアサーションでクラッシュします。

`GameEngine::Run()`では、`Physics::Update(dt)`を`SceneManager::Get().Update(dt)`より**先に**呼ぶようにしています。逆順にすると、RigidbodyComponentが1フレーム古い物理結果をTransformに反映してしまいます。

### ワールド単位について

このエンジンの3Dワールドは、Cube/Sphereの`scale=100`が実寸1mくらいに相当する感覚で作られてきています。PhysXは実寸メートル基準で調整されたデフォルト値(重力・スリープ閾値など)を持っているため、`Physics::Init()`では重力を`(0, -981, 0)`(標準の9.81 m/s²を100倍したもの)に、`PxTolerancesScale.length`も`100`にしてあります(これを合わせないと、シミュレーションの精度・安定性が悪くなります)。今後PxRigidActorのサイズ等を決める時も、この「100単位=1m」のスケール感を意識してください。

### ビルド環境について(記録用)

このマシンではVisual Studio 2022しか入っておらず、PhysX 4.1の公式プリセットはVS2019(`vc16win64`)までしか無かったため、`vc17win64`プリセットを自作して生成しました(`physx/buildtools/presets/public/vc17win64.xml`と、`cmake_generate_projects.py`への`vc17`対応追加)。これらはPhysXのソース側(このリポジトリの外、`ThirdParty/PhysX/`には含まれていないビルド作業用のフォルダ)の変更なので、もし将来PhysXを更新・再ビルドする必要が出てきたら、同じ手順を踏んでください。

- `ThirdParty/PhysX/lib/x64-debug/`, `x64-release/`: Debug/Release、x64向けのビルド済み成果物(`.pdb`/`.map`や、GPU支援用のDLL(`PhysXGpu`/`PhysXDevice`)は容量削減のため含めていません。今回はCPUベースの物理演算のみを使う想定です)
- PhysXのinclude/lib設定は`PhysX.props`(ソリューションフォルダ直下)に1箇所にまとめてあり、`Engine.vcxproj`/`Sample2D.vcxproj`/`Sample3D.vcxproj`の全てがこれをImportしています。PhysXのバージョンを上げる、リンクするlibを増減する時はこのファイルだけ直せば全プロジェクトに反映されます。`RuntimeLibrary`も同じファイルで`MultiThreaded`/`MultiThreadedDebug`(静的CRT)に設定済みです。PhysXのビルドが`NV_USE_STATIC_WINCRT=True`だったため、CRTを合わせないとリンクエラーになります(新しいプロジェクトを追加する時も、必ず同じ`RuntimeLibrary`にしてください)
