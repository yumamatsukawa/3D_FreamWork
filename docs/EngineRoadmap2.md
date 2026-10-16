# Engine 機能追加ロードマップ2(見え方・地形・物理の追加)

## 1. はじめに

### この文書の目的
Engine(`3D_FreamWork/Engine/`)に追加したい9個の機能(視界判定・カプセルコライダー・マテリアル・ライト・影・枠線・地形・地形コライダー・水面)について、「どんな関数を作るか」「どの順番で作るか」「Engineのどのファイルを変えるか」「Sampleではどう使えるか」をまとめた設計書です。チーム(C++初心者を含む)で分担して実装する時の資料として使います。

### `docs/EngineRoadmap.md` との関係
- `EngineRoadmap.md`(ロードマップ1)の17機能は**すべて完了**しています。この文書はその**続き**です。書き方(目的 / ファイル案 / 関数 / 設計のポイント / 置き換えるSampleのコード / 依存する機能)は同じにそろえました。
- 番号は `EngineRoadmap.md` の「5.x」の続き(5.18〜)ではなく、この文書の中で **「6.1 視界判定」** のように付けます(1〜5章は説明、6章が機能ごとの設計)。
- ロードマップ1の機能(物理の問い合わせ 5.6、カメラ機能 5.7、FBX・骨アニメーション 5.16 など)は**もう使える前提**で書いています。特に、視界判定は 5.6 の `Physics::Raycast` を使います。

### 状態
**ここに書いてある内容は「設計案」です。6.1 視界判定・6.2 カプセルコライダー・6.3 マテリアルが実装済みです(4章の表の状態を見てください。ほかは「未着手」)。** 関数名・引数は、実装しながら変わってよいものです。この設計書は「実際のEngineのコードを読んで」書いたので、既存のクラス名・関数名・ファイル名は本物です。新しく作る名前だけが案です。

### 進め方の方針(1機能ずつ)
1機能ごとに、次の流れで進めます。

1. **着手前の確認**: 何を・なぜ・両サンプルへの影響(下の「重要」を参照)
2. **implementer(作業役)が実装** + Debug/Release のビルド確認
3. **reviewer(レビュー役)がレビュー**(Engineの変更・新しいファイル・ロジックの変更は必須)
4. **ユーザーが遊んで確認**(キー入力・マウス操作の確認はユーザーにお願いする。合成したキー入力は送らない)
5. **コミット文を考える**(`commit-message` スキル。READMEも同じターンで更新)

> **重要: この文書の機能は、Engineを変える時にユーザーの了承が必要です。**
> `CLAUDE.md` の「docs/EngineRoadmap.md の機能を作る間の例外(了承を待たずに進めてよい)」の対象は、**`docs/EngineRoadmap.md` の機能だけ**です。この `EngineRoadmap2.md` の機能は対象に**入っていません**。そのため、Engine(`3D_FreamWork/Engine/` 以下、`Shader.hlsl` / `Shader3D.hlsl` / `Shader3DSkin.hlsl` などのシェーダー、`PhysX.props`)を変える時は、通常のルールどおり **①何を変えるか ②なぜ必要か ③Sample2D・Sample3D 両方への影響** を説明して、ユーザーの了承をもらってから手を付けます。
>
> 「この設計書を読んで了承してもらうこと」を、各機能の着手前の確認として扱ってよいかは、**ユーザーが決めます**(7章の未確定事項 1番)。決まるまでは、安全側(機能ごとに着手前の確認をする)で進めます。
> また、了承は「その機能1つ分」だけです。前の機能で了承をもらっていても、次の機能では改めて聞きます。

### 名前の付け方・ファイルを追加する時
`EngineRoadmap.md` の「2. 共通ルール」と同じです(`CLAUDE.md` の命名規則に従う / 新しいファイルは `Engine.vcxproj` と `Engine.vcxproj.filters` の両方に登録 / 日本語を含む `.h`・`.cpp` は UTF-8 BOM付き / Debug と Release の両方でビルド確認)。
新しいシェーダーファイルを足す時だけ追加でやることがあります(5章を参照)。

### 作ったら必ずSampleのコードを置き換える(使う)
`EngineRoadmap.md` と同じ約束です。1機能作ったら、Sampleの該当するコードを必ず置き換える(または新しく使う)ようにします。各節の「置き換える・使うSampleのコード」がその対象です。

---

## 2. 作る順番の考え方

一覧表は 4章にあります。この章では、その順番にした理由を説明します。

### この順番にした理由
- **マテリアル → ライト → 影**: ライトも影も、「この面の色を、光でどう変えるか」という計算を増やす機能です。その計算を入れる場所(マテリアルの定数バッファ・カメラ位置の定数バッファ)を、最初にマテリアルで決めておくと、後の機能がシェーダーを何度も書き直さずに済みます。影は「光の向き・位置」(ライト)と「Meshを速く描ける作り」(マテリアル段階2)が前提なので、一番後ろに置きました。
- **地形 → 地形コライダー → 水面**: 地形コライダーは、地形と**同じ高さのデータ**から作ります(見た目と当たり判定がずれないように)。水面は「地形のくぼみに水を張る」使い方が一番分かりやすいので、地形の後にしました。
- **視界判定とカプセルは単独**: ほかの機能に依存せず、依存もされません(カプセルは地形と相性がよいだけ)。担当者が空いている時に、いつでも先に作ってよいです。
- **枠線はマテリアルの後**: 枠線の色・太さ・縁の光り方はマテリアルの項目として持つので、マテリアルが先です。影(5)とは独立しているので、影が難しければ先に作ってもかまいません。

---

## 3. 今のEngineで分かっている事実(コードを読んで確認したこと)

設計に関わる「今のコードの事実」です。各機能の設計は、これを前提に書いています。

### 描画まわり
1. **`ColliderType` という名前のものは、今のコードにありません。** 当たり判定の形は、`RigidbodyComponent`(共通部分)の**派生クラス**で表しています(`BoxRigidbodyComponent` / `SphereRigidbodyComponent` / `CircleRigidbodyComponent`(2D) / `SquareRigidbodyComponent`(2D))。種類は `BodyType`(Dynamic / Kinematic / Static)。形ごとの違いは、派生クラスの `Init()` が `InitWithGeometry(PxGeometry)` を呼ぶ部分だけです。
2. **`RigidbodyComponent::Draw`(当たり判定の枠線表示)は、Box と Sphere の形にしか対応していません。** それ以外の形(`switch` の `default`)では何も描きません。
3. **(6.3 で直した。下は直す前の状態)** **`Mesh::Draw` は、描く度に頂点を全部コピーして、色を上書きしてから送っています**(`Mesh.cpp` の `coloredVerts`)。頂点バッファは `D3D11_USAGE_DYNAMIC` で、`Map` / `memcpy` / `Unmap` を毎回やります。小さな立方体・球なら問題ありませんが、**頂点が数万個ある地形や、1フレームに2回描く影の描画には向きません**。インデックスバッファ(頂点を使い回す仕組み)も未対応です。なお、今ある `Mesh` の頂点の色は**すべて白**(`CreateCube` / `CreateSphere` / `LoadOBJ` / 輪郭の線 / FBXのモデル)なので、「色を頂点にではなく定数バッファで渡す」作りに変えても見た目は変わりません。
4. **`Shader3D.hlsl` は、平行光源1つ + 環境光だけです。** 鏡面反射(ハイライト)も、ワールド座標のピクセルへの受け渡しもありません。`PSInput` に位置が無いので、点光源・スポットライト・影・縁の光には**ワールド座標の受け渡しを足す**必要があります。`clip(texColor.a - 0.01f)` で、ほぼ透明なピクセルは描きません。
5. **`Shader3DSkin.hlsl` は、ピクセルシェーダーの中身が `Shader3D.hlsl` のコピーです。** 光の計算を変える時は2つのファイルを同じように直さないとずれます。
6. **定数バッファの割り当て**: `b0` = `MeshConstantBuffer`(world / view / proj / lightDir / lightColor / ambient。`Mesh.h` で定義)。`Shader3DSkin.hlsl` だけ `b1` = 骨の行列64個 + `tint`。テクスチャは `t0`、サンプラーは `s0`。
7. **シェーダーは実行時にコンパイルしています**(`D3DCompileFromFile(L"Shader3D.hlsl", nullptr, nullptr, ...)`)。`Shader.hlsl` / `Shader3D.hlsl` / `Shader3DSkin.hlsl` は、`Engine.vcxproj` の `CopyRuntimeFiles`(`xcopy`)で実行ファイルの隣にコピーされ、`UpToDateCheckInput` にも登録されています。**新しいシェーダーファイルを足したら、この2か所にも足す必要があります。** また、`#include` を使うには、`D3DCompileFromFile` の第4引数を `nullptr` から `D3D_COMPILE_STANDARD_FILE_INCLUDE` に変える必要があります。
8. **`Mesh` と `SkinMesh` は、シェーダー・ステート・定数バッファを `static` で1組ずつ共有**し、参照カウントで最後の1体が `Uninit` した時に解放します。新しい定数バッファや新しいステートを足す時は、この「共有物」の仕組みに載せます。
9. **カメラ(`Camera.h`)はアスペクト比を持っていません。** 画面の縦横比は `Graphics::width / Graphics::height` から、`Mesh::Init` と `Image.cpp`(無名namespaceの `GetAspectRatio`)がそれぞれ計算しています。3Dカメラの `fovY` は、`Image::BeginFrame` が(`SetCamera3DAutoSync(true)` の間)**毎フレーム上書き**しています。`nearZ = 5`、`farZ = 10000`。
10. **描画の順番**: `Scene::Draw` は `drawPriority` が小さい順、同じなら `position.z` が小さい順です(**カメラからの距離順ではありません**)。そのため、半透明の物をきれいに重ねる並べ替えはありません。
11. **深度バッファは `DXGI_FORMAT_D24_UNORM_S8_UINT`** です(ステンシル付き。ただしシェーダーから読める形(SRV)ではありません)。`Image::BeginFrame` は深度だけをクリアしています(ステンシルはクリアしていない)。レンダーターゲットはスワップチェーンのバックバッファ1枚で、ビューポートは起動時に1回だけ設定しています。
12. **`GameEngine::Run` の描画の流れ**: `Image::BeginFrame()`(画面クリア・カメラ追従) → `CameraShake::BeginDraw()` → `Text::BeginDraw()` → `SceneManager::Get().Draw()` → `ScreenEffect::Draw()` → `SceneManager::Get().DrawTransition()` → `Text::EndDraw()` → `CameraShake::EndDraw()` → `Image::EndFrame()`(Present)。

### 物理まわり
13. **`PhysX.props` は `PhysXCooking_64.lib` をすでにリンクしています**(Debug / Release とも)。地形コライダー(高さフィールド)に必要な「クッキング」のライブラリは**追加不要**で、`PhysX.props` は変えなくて済みます。ただし `Physics.cpp` に `PxCooking` は無いので、作る処理を足す必要があります(`PxPhysicsAPI.h` が `cooking/PxCooking.h` も読み込みます)。
14. **`RigidbodyComponent::InitWithGeometry(const PxGeometry&)`** は、アクターの位置・回転を「オーナーの `Transform`」から作り、**形(ジオメトリ)だけ**を受け取ります。形をアクターの中心からずらしたり、向きを変えたり(shapeのローカルポーズ)する機能は、今はありません。
15. **PhysXのカプセル(`PxCapsuleGeometry`)は、X軸の向きに伸びます**(`PxCapsuleGeometry.h` に書いてある)。さらに `radius > 0` かつ `halfHeight > 0` でないと無効な形です。立って歩くカプセルにするには、形を回す(Z軸まわり90度)必要があります。
16. **`Physics::Raycast(origin, direction, maxDistance, hit, tag = "", triggers = QueryTriggers::Ignore)`** の決まり(`Physics.h`): `tag` を指定すると、**そのタグの物だけ**を対象にして一番近い物を返す / トリガーは無視 / 始点が中に入っている物には当たらない。視界判定の「壁に遮られているか」は、この決まりを使って作れます(6.1)。

### Sampleまわり
17. **Sample3D の敵・Sample2D の敵は、どちらも `SpriteRenderer`(画像)で描かれています**(Sample3D は `SetWorldSpace(true)` のビルボード)。`Shader3D.hlsl` を使う `MeshRenderer` / `ModelRenderer` ではないので、**ライト・影・枠線(6.4〜6.6)は敵の見た目には効きません**。効くのは、Sample3D の Player(立方体)・Sphere・Cube・ModelDisplay・SkinDisplay・Ground のように `Mesh` で描くものです。
18. **Sample2D は `Shader.hlsl`(スプライト)だけで描かれ、`Shader3D.hlsl` は使いません。** ただし `RigidbodyComponent::Draw` の当たり判定の枠線表示は、2Dでも `Mesh`(`Shader3D.hlsl`)で描いています。`Shader3D.hlsl` を変える機能(6.3〜6.6、6.9)は、**このデバッグ表示を通してSample2Dにも影響します**(見た目が変わらないことを確認する)。
19. **Sample3D の `GroundController` は、カメラのまわりのタイル(立方体)を自前で並べて描いています**(描画距離の円の外は描かない、手書きの範囲判定)。地面の当たり判定は、見えない大きな `BoxRigidbodyComponent`(`CreateGround`)です。
20. **Sample3D の Player は、`MeshRenderer`(立方体)+ `BoxRigidbodyComponent`**です。回転ロック中の箱を、`PlayerController` が `rigidbody->GetActor()->setGlobalPose(...)` で `rotate.y` に合わせて回しています。
21. **`Time::GetTime()` がある**(起動してからの時間。倍率の影響を受ける)ので、水面のうねりの時間に使えます。

---

## 4. 作る順番(一覧表)

| 順 | 機能 | 理由・依存 | 担当者 | 状態 |
|---|---|---|---|---|
| 1 | [6.1 視界判定](#61-視界判定実装済み) | 5.6(物理の問い合わせ)・5.1(計算系)を使う。単独 | | 実装済み |
| 2 | [6.2 カプセルコライダー](#62-カプセルコライダー実装済み) | 単独(`RigidbodyComponent` に形を足す) | | 実装済み |
| 3 | [6.3 マテリアル](#63-マテリアル実装済み) | 5.3(`Resources`)を使う。ライト・影・枠線・水面・地形の土台 | | 実装済み |
| 4 | [6.4 ポイントライトとスポットライト](#64-ポイントライト点光源とスポットライト) | 6.3 | | 未着手 |
| 5 | [6.5 影](#65-影シャドウマップ) | 6.3(段階2)・6.4 | | 未着手 |
| 6 | [6.6 枠線を光らせる](#66-枠線を光らせるアウトラインリム発光) | 6.3・6.4 | | 未着手 |
| 7 | [6.7 地形](#67-地形) | 6.3(段階2)。6.1は任意 | | 未着手 |
| 8 | [6.8 地形に合わせたコライダー](#68-地形に合わせたコライダー) | 6.7 | | 未着手 |
| 9 | [6.9 水面](#69-水面) | 6.3・6.4。6.7があると使い方が分かりやすい | | 未着手 |

---

## 5. 全機能で共通する設計(シェーダーの約束)

マテリアル・ライト・影・枠線・水面は、どれも `Shader3D.hlsl` と `Shader3DSkin.hlsl`(水面は専用のシェーダー)に項目を足します。バラバラに足すとレジスタ番号がぶつかるので、**最初に番号を決めておきます**。

### 5.1 レジスタの割り当て表(案)
| レジスタ | 中身 | 作る機能 | 更新の頻度 |
|---|---|---|---|
| `b0` | `MeshConstantBuffer`(world / view / proj / lightDir / lightColor / ambient)。**今のまま変えない** | (今ある) | 描く度 |
| `b1` | 骨の行列(`Shader3DSkin.hlsl` だけ。今のまま) | (今ある) | 描く度 |
| `b2` | `MaterialBuffer`(鏡面・自発光・縁の光・フラグ) | 6.3 | 描く度 |
| `b3` | `LightBuffer`(カメラ位置 + 点光源・スポットライトの配列) | 6.4 | 1フレームに1回 |
| `b4` | `ShadowBuffer`(光から見たview×proj・バイアスなど) | 6.5 | 1フレームに1回 |
| `t0` / `s0` | 画像 / サンプラー(今のまま) | (今ある) | |
| `t1` / `s1` | 影の深度テクスチャ / 比較サンプラー | 6.5 | |
| `b5` | `WaterBuffer`(水面だけ) | 6.9 | 1フレームに1回 |

**`b0` を変えない理由**: `MeshConstantBuffer` は `Mesh` と `SkinMesh` と2つのシェーダーが同じ形で使っていて、平行光源はここに入っています。ここを変えると全部を同時に直す必要があります。**平行光源は今のまま `b0`、増える光(点光源・スポット)は新しい `b3`**にすれば、新しい光を1つも置かない限り、今のゲームの見た目は変わりません。

### 5.2 シェーダーの共通部分を `.hlsli` にまとめる(推奨)
今は `Shader3D.hlsl` と `Shader3DSkin.hlsl` で光の計算が重複しています。光の計算(鏡面・点光源・スポット・影・縁の光)を2か所に書くとずれるので、**共通の関数を `Shader3DCommon.hlsli`(新規)に書き、2つのシェーダーから `#include` する**ことを推奨します。
- `D3DCompileFromFile` の第4引数を `D3D_COMPILE_STANDARD_FILE_INCLUDE` にする(`Mesh.cpp` / `SkinMesh.cpp`)
- `Engine.vcxproj` の `CopyRuntimeFiles` と `UpToDateCheckInput` に `Shader3DCommon.hlsli` を足す
- **落とし穴**: 実行ファイルの隣に `.hlsli` がコピーされていないと、実行時のコンパイルが失敗して何も描かれなくなります(出力ウィンドウにエラーが出る)。コピーの設定の足し忘れに注意
- `#include` を使いたくない場合は、2つのシェーダーに同じ関数を書き写す方法でも作れます(ずれないよう、コメントに「もう片方も直す」と書く)。どちらにするかは 7章の未確定事項。

### 5.3 定数バッファの注意(初心者がつまずく所)
- HLSLの `cbuffer` は **16バイト区切り**で詰められます。C++の構造体は `XMFLOAT4` だけで作って、`static_assert(sizeof(MaterialBuffer) % 16 == 0)` を付けると、ずれに気付けます。`float` を1つだけ置くと次の `float4` との間に隙間ができて、値がずれます(`float3` の後ろに `float` を置くのは詰まるのでOK)。
- 行列は転置して送る(今の `MeshConstantBuffer` と同じ)。

### 5.4 新しいシェーダーファイル・新しいEngineファイルを足す時
- 新しい `.hlsl` / `.hlsli` は、`3D_FreamWork/` の直下(今の `Shader3D.hlsl` と同じ場所)に置き、`Engine.vcxproj` の `UpToDateCheckInput` と `CopyRuntimeFiles` に足す(3章7番)
- 新しい `.h`/`.cpp` は `Engine.vcxproj` と `.filters` の両方に登録する

---

## 6. 機能ごとの設計

各節は「目的 / ファイル案 / 関数 / 設計のポイント / 両サンプルへの影響 / 置き換える・使うSampleのコード / 依存する機能 / 未確定の点」の順に書いています。「両サンプルへの影響」には、**ユーザーの了承が必要になる Engine の変更点**(変えるファイル)をまとめて書いています。

---

### 6.1 視界判定(実装済み)

**目的**: 2つの「見えるか」を調べられるようにする。
- **(a) カメラに映っているか**: 画面の外にあるオブジェクトを描かない・処理を省く(ビューフラスタム判定。2Dはカメラの矩形、3Dは視錐台と、オブジェクトの大きさ(バウンディング球)で調べる)
- **(b) 敵の視野の扇形に入っているか**: 角度・距離で扇形の中にいて、かつ壁などで遮られていないか(5.6 の `Physics::Raycast` を使う)

**ファイル案**:
- `Engine/Visibility.h/.cpp`(中身は `namespace Visibility`。(a)の判定)
- `Engine/VisionComponent.h/.cpp`((b)の視野。敵などに付ける)
- 既存の変更: `Engine/MeshRenderer.h`・`Engine/ModelRenderer.h/.cpp`・`Engine/SpriteRenderer.h/.cpp`(画面外なら描かない)、`Engine/Model.h/.cpp`(大きさの半径)、`Engine/GameEngine.cpp`(毎フレームの準備を呼ぶ)、`Engine/Image.h/.cpp`(縦横比を返す関数を公開)

**関数**:
```cpp
// ── (a) カメラに映っているか ──
namespace Visibility {
    // 3D: 今の3Dカメラ(Image::GetCamera3D())の視錐台に、球・箱が少しでも入っているか
    bool IsSphereInView3D(XMFLOAT3 center, float radius);
    bool IsBoxInView3D(XMFLOAT3 center, XMFLOAT3 halfSize);      // 軸に沿った箱(回転しない)

    // 2D: 今の2Dカメラ(Image::GetCamera())の画面に、円・四角が少しでも入っているか
    // (SpriteRenderer(UIモード)と同じ遠近の倍率を使う。画面の大きさは Graphics::width / height)
    bool IsCircleInView2D(XMFLOAT3 position, float radius);
    bool IsRectInView2D(XMFLOAT3 position, XMFLOAT2 halfSize);

    // 描かずに省いた数(確認用。HUDに出して、ちゃんと省けているか見る)
    int  GetCulledCount();

    // ── Engine内部用(GameEngineだけが呼ぶ) ──
    void BeginFrame();    // CameraShake::BeginDraw() の後に1回。視錐台をそのフレームのカメラから作り直す
}

// SpriteRenderer / MeshRenderer / ModelRenderer に追加
void SetFrustumCulling(bool enabled);   // 画面の外なら描かない(既定は7章の未確定事項)

// ── (b) 敵の視野 ──
class VisionComponent : public Component {
public:
    void SetFieldOfView(float degrees);          // 視野の角度(全体。90なら左右45度ずつ)
    void SetRange(float distance);               // 見える距離
    void SetPlane(MovePlane plane);              // 見る平面(2D=XY、3Dの地面=XZ。MoveComponentと同じ MovePlane を使う)
    void SetEyeOffset(XMFLOAT3 offset);          // 目の位置(足元ではなく胸の高さなど。オーナーの位置からの相対)
    void SetObstacleTag(const std::string& tag); // このタグの物は、視線をさえぎる壁(既定 "Wall")
    void SetFacing(XMFLOAT3 direction);          // 向きを手で指定する(長さ0に戻すと、transform の向きを使う)
    void SetCheckInterval(float seconds);        // 壁の判定(レイキャスト)をやり直す間隔(既定 0.1秒。毎フレームは重い)

    bool IsInCone(XMFLOAT3 point) const;         // 扇形(角度と距離)の中か。壁は考えない
    bool CanSee(XMFLOAT3 point) const;           // 扇形の中で、壁にさえぎられていないか(レイキャストする)
    bool CanSee(GameObject* target) const;       // 上と同じ(相手の位置を使う)

    static void SetDebugDrawEnabled(bool enabled);   // 扇形を線で描く(RigidbodyComponent::SetDebugDrawEnabled と同じ使い方)
};
```

**設計のポイント**:

*(a) カメラに映っているか*
- **3Dは `DirectXCollision.h` の `BoundingFrustum`(DirectXMathに付いている)を使います。** `BoundingFrustum(camera.GetProjectionMatrix(aspect))` で視錐台を作り、`Transform` でカメラのワールド行列(`GetViewMatrix` の逆行列)を掛けて、ワールド座標の視錐台にします。`Intersects(BoundingSphere)` で球の判定ができます。エンジンはLH(左手系)なので、`BoundingFrustum` の既定(LH)のままで合います。
- **視錐台を作るのは1フレームに1回**(`Visibility::BeginFrame`)。呼ぶ場所は `CameraShake::BeginDraw()` の**後**です(揺れが入ったカメラで判定しないと、画面の端で揺れた時に一瞬描かれない物が出ます)。`Update` の中で呼ぶと「1つ前のフレームの描画に使ったカメラ」で判定します(敵のAIを省く程度なら問題ない。コメントに書く)。
- **縦横比**: 今、`Image.cpp` の無名namespaceの中にある `GetAspectRatio` と同じ式です。`Image.h` に公開するか、`Visibility.cpp` に同じ式を書くかを実装時に決めます(同じ式が3か所になるので公開を推奨)。
- **2Dは、`SpriteRenderer`(UIモード)と同じ遠近の倍率で画面座標に直して、画面の矩形(`±Graphics::width/2`, `±Graphics::height/2`)と比べます。** `Image::WorldToScreen2D(worldPosition)` が使えます(`z` が奥にあるほど `focalLength / (z - camera.z)` 倍で小さくなる)。画面上の半径は「半径 × 同じ倍率」です。カメラより手前(`depth <= 0`)は倍率1のまま扱う(`Image.cpp` の `GetDepthScale2D` と同じ)。
- **バウンディング球の半径**:
  - 立方体(`Mesh::CreateCube`、一辺1): 半径 = `0.5 × √3 × max(scale)` ≒ `0.87 × max(scale)`
  - 球(`Mesh::CreateSphere`、半径0.5): 半径 = `0.5 × max(scale)`
  - スプライト(2D): `Sprite.cpp` の大きさの決め方と同じ(`scale.x > 0` ならそれ、0なら画像の幅)の大きい方 × 0.71(正方形の対角の半分)
  - モデル(FBX): `Model` が読み込んだ時に、全パーツの頂点から半径を求めて持つ(`Model::GetBoundingRadius()` を足す案)。**骨で曲がるモデル・アニメーションで動くパーツは、静止の姿より大きくはみ出す**ので、余裕(1.5倍など)を持たせる
  - 親子関係のあるオブジェクトは `transform.GetWorldMatrix()` の位置を使う(`position` は親からの相対なので、そのまま使うとずれる)
- **落とし穴(消えてしまう物)**:
  - スカイボックス(`SetDepthWrite(false)`)・地面のように「画面全体」を覆う物は、球では表せません。**`depthWrite == false` の `MeshRenderer` は自動で判定をしない**ことにし、ほかは `SetFrustumCulling(false)` で切れるようにします。
  - Sample2D の `BackgroundController`・Sample3D の `GroundController` は `SpriteRenderer` / `MeshRenderer` を使わず、自前で描くので、この変更の影響を受けません(`GroundController` の範囲判定は、`Visibility::IsSphereInView3D` に置き換えられます。置き換えるSampleのコードを参照)。
  - 影の描画(6.5)では、**カメラの外にある物も影を落とす**ので、影のパスでは画面外判定を使わない(光から見た範囲で判定する)。
  - 画面外の判定が間違っていると、**物が突然消えて見える**不具合になります(描画しか省かないので、物理や当たり判定は動き続ける)。`Visibility::GetCulledCount()` をHUDに出して確認します。
- 物理(PhysX)の当たり判定は、この機能では省きません(見えない敵も壁にぶつかる。省くと位置がずれる)。

*(b) 敵の視野*
- **角度の判定**: 向きベクトル(`facing`)と、相手への向きベクトルを、動く平面(XZ または XY)の上に直して、`Math::Dot` で比べます(`Dot ≥ cos(視野/2)` なら角度内)。長さは `Math::Distance` / `Math::DistanceSq`(2乗で比べると `sqrt` を省ける)。向きは `Math::Normalize`。
- **向き(正面)の決め方**: 手で指定(`SetFacing`)が最優先。指定が無ければ、3D(XZ)は `Math::Forward3D(transform)`(`rotate.y`)、2D(XY)は `Math::Forward2D(transform)`(`rotate.z`)を使います。
  - **落とし穴**: Sample3D の敵は `rotate = {0,0,0}` のまま(`SpriteRenderer` のビルボードは `rotate.y` を使わない)で、向きの情報を持っていません。追いかける敵に付けるなら、`SetFacing` を毎フレーム呼ぶ(`EnemyController::Update` で進む向きを渡す)必要があります。
- **壁にさえぎられているか**: 目の位置(`transform.position + eyeOffset`)から相手の位置へ、`Physics::Raycast(origin, direction, distance, hit, obstacleTag)` を呼びます。**`obstacleTag` を指定すると、そのタグの物だけが対象**になり(5.6 の決まり)、`hit.distance < 相手までの距離` なら壁にさえぎられています。
  - この作り方なら、Physics に手を入れずに済みます。ほかの敵・Player・弾は壁の扱いにならないので、「手前に別の敵がいたから見えない」にもなりません。
  - **落とし穴1**: 壁のオブジェクトにタグ(`"Wall"`)を付け忘れると、素通しになります。
  - **落とし穴2**: `Physics::Raycast` は「始点が中に入っている物には当たらない」ので、**壁の中に目があると見えてしまう**(普通は起こらない)。
  - **落とし穴3**: 地面(`Ground`)は壁のタグではないので、斜め下を見る視線は地面にさえぎられません(地面が出っ張る地形(6.7)で視線がめり込んでも、見えるまま)。地形で視線をさえぎりたい時は、地形に壁のタグを付けるか、`obstacleTag` を地形のタグにする(地形コライダー(6.8)もレイキャストで当たる)。
  - **落とし穴4**: 相手の「1点(位置)」しか見ないので、相手の体が壁から少しはみ出していても、中心が壁の陰なら見えません。気になる時は、目の高さを変えて2回判定する案があります(7章の未確定事項)。
  - **落とし穴5**: レイキャストは**1つ前の `Physics::Update` の位置**を使います(`Physics.h` のコメントのとおり)。動いたばかりの壁には1フレーム遅れて反映されます。
- **重さ**: 敵が100体いて毎フレーム100回レイキャストするのは無駄が多いので、`SetCheckInterval`(既定0.1秒)で間隔をあけ、結果を覚えておきます(角度と距離の判定は毎フレームでも軽い)。敵ごとに最初の判定をずらして(`Math::RandomRange`)、同じフレームに集中しないようにします。
- **デバッグ表示**: 扇形の外側の弧と2本の辺を、線(`D3D11_PRIMITIVE_TOPOLOGY_LINELIST`)で描きます。`Mesh::Draw` は `vertices` を毎回そのまま送る作りで、頂点を後から変える関数が無いので、**距離1の扇形を作っておき、`Transform` の `scale`(距離)と `rotate`(向き)で大きさと向きを変える**やり方にします(`RigidbodyComponent::Draw` が輪郭のメッシュを `scale` で伸ばしているのと同じ)。視野の角度を変えた時だけ、`Mesh` を作り直します。見えている時は赤、見えていない時は緑。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 新規: `Engine/Visibility.h/.cpp`、`Engine/VisionComponent.h/.cpp`、`Engine.vcxproj(.filters)`
- 既存の変更: `Engine/MeshRenderer.h`・`Engine/ModelRenderer.h/.cpp`(`Draw` の最初に判定を足す)、`Engine/SpriteRenderer.h/.cpp`、`Engine/Model.h/.cpp`(半径)、`Engine/GameEngine.cpp`(`Visibility::BeginFrame()` を1行足す)、`Engine/Image.h/.cpp`(縦横比を返す関数。任意)
- `Shader*.hlsl`・`PhysX.props`・`Physics.h/.cpp` は**変えない**
- Sample2D: `SpriteRenderer` が画面外なら描かなくなる(見た目は変わらない。**画面外にいる敵・弾のスプライトを描かなくなるだけ**)。Sample3D: `MeshRenderer` / `ModelRenderer` / 敵の `SpriteRenderer`(ビルボード)も同じ。**見た目は変わらない**はずですが、半径の見積もりが小さいと画面の端で消えるので、遊んで確認してもらう。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D GroundController::Draw: 描画距離の円の手書き判定を置き換える
// 今:  dx*dx + dz*dz > range*range なら continue
// 後:  if (!Visibility::IsSphereInView3D(tileCenter, tileSize * 0.8f)) continue;   // タイルが画面に映らなければ描かない

// Sample3D EnemyManager / Sample2D EnemyManager の SetupEnemy: 画面の外の敵のアニメーションを止める
//   (EnemyController::Update 等で)
if (!Visibility::IsSphereInView3D(GetOwner()->transform.position, 100.f)) anim->SetEnabled(false);

// 見張り役(新しく置く。下の「未確定の点」で確認)
// Sample3D/Objects/Guard.cpp: CreateGuard(scene, player)
auto* vision = guard->AddComponent<VisionComponent>();
vision->SetFieldOfView(90.f);
vision->SetRange(1500.f);
vision->SetPlane(MovePlane::XZ);
vision->SetEyeOffset({ 0.f, 50.f, 0.f });
// GuardController::Update
bool seen = vision->CanSee(player);     // 見えたら赤くする・追いかけ始める
sprite->SetColor(seen ? 1.f : 0.f, 0.f, seen ? 0.f : 1.f, 1.f);
// 壁: Sample3D/Objects/Wall.cpp(新規。BoxRigidbodyComponent の Static + タグ "Wall") を数枚置く
```
対象: Sample3D の `GroundController`(円の判定の置き換え)、敵のアニメーション停止(両サンプル)。新しいオブジェクト `Guard` と `Wall` は、Sample3D(と、Sample2D にも同じ物を置くか)を**ユーザーに確認してから**作る。

**実装して決めたこと(実装済み)**:
- `SetFrustumCulling` の既定は **OFF**(おすすめの記載が無かったので、今までの見た目を変えない安全側)。Sampleでは、両サンプルの敵(`SpriteRenderer`)だけ `SetFrustumCulling(true)` にした。`depthWrite == false` の `MeshRenderer` は、ONでも判定しない。
- `Guard` と `Wall`(タグ `"Wall"`)を **Sample2D・Sample3D の両方**に置いた。相手の判定は **1点だけ**(位置)。
- `Visibility` に Engine内部用の `CountCulled()` と `IsTransformedSphereInView3D(world, localRadius)` を足した(設計書の関数に加えた分)。`GetCulledCount()` は「前のフレームの数」を返す(HUDが描く時点で今のフレームがまだ途中でも、数が揺れないように)。
- 半径は、`MeshRenderer` は頂点の最大距離(立方体なら約0.87)、`ModelRenderer` は `Model::GetBoundingRadius()`(止まっている姿の最大距離)×1.5、`SpriteRenderer` は `scale` の幅・高さの対角線の半分。`scale` が0以下のスプライトは判定しない。縦横比は `Image::GetAspectRatio()` を公開して使った(Image.h/.cpp)。
- `VisionComponent::SetPlane` の既定は `XY`(`MoveComponent` と同じ)。`CanSee(GameObject*)` だけ壁の判定を `SetCheckInterval`(既定0.1秒、±20%ばらす)ごとに覚える。`CanSee(点)` は毎回Raycastする。`SetObstacleTag("")` は壁なし。
- 「画面外の敵のアニメーションを止める」(`anim->SetEnabled(false)`)は、動きが変わるので **まだやっていない**。

**依存する機能**: 5.1 計算系、5.6 物理の問い合わせ(`Physics::Raycast`)、5.7 カメラ機能(`Image::WorldToScreen2D` と同じ遠近の考え方)

**未確定の点(ユーザーに聞きたいこと)**:
- (a) `SetFrustumCulling` の既定を **ON** にするか **OFF** にするか(ONだと何もしなくても速くなるが、半径が合わない物が消える危険がある)
- (b) 見張り役 `Guard`(ゆっくり回って、視野に Player が入ったら赤くなる)を、Sample3D(と Sample2D)に**新しく置いてよいか**。今の敵(いつも追いかけてくる)の動きは変えない案です
- (b) 壁のタグ名は `"Wall"` でよいか。Sample に壁を置いてよいか
- (b) 相手の判定を「1点だけ」にするか「頭と足元の2点」にするか

---

### 6.2 カプセルコライダー(実装済み)

**目的**: 立った人や敵のような、細長い形(円柱の両端が半球)の当たり判定を作れるようにする。箱の角で段差に引っかからない、坂道を滑らかに歩ける、という利点がある(特に地形(6.7〜6.8)の上を歩く時)。

**ファイル案**: 既存の `Engine/RigidbodyComponent.h/.cpp` に追加(新しい派生クラス)。見た目用に `Engine/Mesh.h/.cpp` に `CreateCapsule` を追加。

**関数**:
```cpp
// カプセルの長い向き(PhysXは元々X軸。立って歩く形になるよう、既定はYにする)
enum class CapsuleAxis { X, Y, Z };

// 3D用のカプセル形状の剛体。詳しい説明はRigidbodyComponentのコメントを参照
// radius  : 半径。既定(0)ならOwnerの transform.scale.x の半分
// height  : 全体の高さ(両端の半球を含む。Unityと同じ)。既定(0)ならOwnerの transform.scale.y
//           (2×半径より小さい時は、2×半径に少し足した値にする = PhysXは円柱の部分が0だと無効な形のため)
// axis    : 長い向き(既定 Y)
class CapsuleRigidbodyComponent : public RigidbodyComponent {
public:
    explicit CapsuleRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        float radius = 0.f, float height = 0.f, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false,
        CapsuleAxis axis = CapsuleAxis::Y);
    void Init() override;
};

// 2D用(XY平面で使う)。中身はカプセルと同じだが、デバッグ表示は断面(角の丸い四角)だけを描く。
// CircleRigidbodyComponent / SquareRigidbodyComponent と同じく、2Dでは SetFreezePositionZ(true) で使う
class Capsule2DRigidbodyComponent : public RigidbodyComponent {
public:
    explicit Capsule2DRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        float radius = 0.f, float height = 0.f, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false,
        CapsuleAxis axis = CapsuleAxis::Y);    // Y=縦長、X=横長(Zは2Dでは使わない)
    void Init() override;
};

// 見た目(MeshRenderer に渡す)。半径radius・全体の高さheightの、実際の大きさで作る
// (立方体や球のような「大きさ1の形を scale で伸ばす」やり方だと、両端の半球が潰れて楕円になるため、scale は 1 のまま使う)
static std::vector<MeshVertex> Mesh::CreateCapsule(float radius, float height, int rings = 8, int segments = 24);
```

**設計のポイント**:
- **今のコードに `ColliderType` はありません**(3章1番)。形は派生クラスで表す今の設計に合わせて、`CapsuleRigidbodyComponent`(3D)と `Capsule2DRigidbodyComponent`(2D)を足します。`Box` / `Sphere` と `Square` / `Circle` の関係(2Dの方が `flat2D = true` でデバッグ表示だけ違う)と同じです。2D用の名前は `Capsule2D` でよいか、7章で確認します。
- **向き(落とし穴)**: PhysXの `PxCapsuleGeometry` は**X軸に伸びる**ので、そのままだと寝た状態になります。形(shape)のローカルポーズを回して、Y向き(X→Y: Z軸まわりに +90度)・Z向き(X→Z: Y軸まわりに −90度)にします。`InitWithGeometry` が今は「ジオメトリだけ」を受け取る作りなので、**`InitWithGeometry(geometry, 形の向き)` のように、形の向き(`PxTransform` か、軸の種類)を渡せるように広げる**必要があります(`PxShape::setLocalPose` を、アクターを作った後で呼ぶ)。
- **アクター自身の回転は別**: Player のように `SetFreezeRotation(true)` + `setGlobalPose` で `rotate.y` に合わせて回す使い方は、カプセルが縦のままY軸まわりに回るだけなので問題ありません(`PlayerController` の `colliderRotateY` の処理はそのまま使える)。**立ったカプセルを、回転ロックなしの Dynamic にすると、ぶつかって倒れて転がります**。人・敵に使う時は `SetFreezeRotation(true)` を付けるのが基本です(サンプルのコメントに書く)。
- **サイズの決め方**: PhysX は円柱の半分の長さ(`halfHeight`)を使い、`radius > 0` かつ `halfHeight > 0` が必須(`PxCapsuleGeometry.h`)。`halfHeight = (height − 2×radius) / 2` を計算し、0以下になる時は小さな正の値にする(球に近い形)。
- **質量**: 他の形と同じく `PxCreateDynamic` が密度から計算するので、`density` の意味は同じです。
- **デバッグ表示**: `RigidbodyComponent::Draw` の `switch` に `PxGeometryType::eCAPSULE` を足します。ただし、カプセルは**大きさ(半径と高さ)で形が変わる**ので、立方体・球のように「大きさ1の輪郭を `scale` で伸ばす」やり方が使えません。`Mesh::CreateCapsuleOutline(radius, halfHeight, axis, segments)` で、実際の大きさの輪郭の線(両端の半円2つ + 側面の線)を作り、コンポーネントが自分用の `Mesh` を持つ作りにします(`Init` で作り、`Uninit` で解放)。2D用は断面の「角の丸い四角」(スタジアム形)だけにします。
- **質問が出そうな点**: `Physics::OverlapSphere` / `OverlapBox` は、カプセルの形にも当たる(相手の形は何でもよい)ので、変更は不要です。カプセルで調べる `Physics::OverlapCapsule` は、今回は作りません(必要になったら足す)。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 変更: `Engine/RigidbodyComponent.h/.cpp`(派生クラス2つ、`InitWithGeometry` に形の向きを渡せるようにする、`Draw` の `eCAPSULE`)、`Engine/Mesh.h/.cpp`(`CreateCapsule`、`CreateCapsuleOutline`)、`Engine.vcxproj`(ファイルは増えないので不要。増やす場合のみ)
- `Shader*.hlsl`・`PhysX.props`・`Physics.h/.cpp` は**変えない**
- 既存の `Box` / `Sphere` / `Circle` / `Square` の動きは変わらない(`InitWithGeometry` の引数は既定値つきで追加するので、呼び出し側は直さなくて済む)。Sample2D・Sample3D とも、**既存のオブジェクトを置き換えるまでは何も変わりません**。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D Player.cpp: 立方体の見た目と箱の当たり判定を、カプセルにする
// 今:  player->AddComponent<MeshRenderer>(Mesh::CreateCube());
//      auto* rigidbody = player->AddComponent<BoxRigidbodyComponent>(BodyType::Dynamic, player->transform.scale, 100.f);
// 後:
player->transform.scale = { 1.f, 1.f, 1.f };    // カプセルは大きさそのままで作るので、scale は 1
player->AddComponent<MeshRenderer>(Mesh::CreateCapsule(50.f, 180.f));
auto* rigidbody = player->AddComponent<CapsuleRigidbodyComponent>(BodyType::Dynamic, 50.f, 180.f, 100.f);
// 以下の SetUseGravity / SetFreezeRotation / PlayerController の setGlobalPose の回転はそのまま

// Sample3D EnemyManager.cpp SetupEnemy: 敵(立て看板の画像)の箱を、カプセルにして段差・坂道(地形)で引っかからなくする
auto* rigidbody = obj->AddComponent<CapsuleRigidbodyComponent>(BodyType::Dynamic, 40.f, 100.f, 1.0f);
```
対象: Sample3D の Player・敵(**見た目と当たり判定の大きさがそろうように、`GaugeRenderer` の位置・`CameraController` の目の高さ・`transform.scale` を使っている所を確認**)。Sample2D は今回は置き換えず(円と四角で足りる)、`Capsule2DRigidbodyComponent` を試すために**細長い障害物**を新しく置く案(7章で確認)。

**実装して決めたこと(実装済み)**:
- 2D用のクラス名は設計書どおり `Capsule2DRigidbodyComponent`。`CapsuleAxis`(X/Y/Z)は `Mesh.h` に置いた(`Mesh::CreateCapsuleOutline` も使うため。`RigidbodyComponent.h` が `Mesh.h` を include している)。
- `InitWithGeometry(geometry, axis)` に形の向きを足した。`axis` が Y/Z の時だけ `PxShape::setLocalPose` で形を回し(Y: Z軸まわり+90度、Z: Y軸まわり-90度)、質量・慣性は `updateMassAndInertia` で計算し直す。呼び出し側(Box/Sphere/Circle/Square)は変更なし。
- `halfHeight` が小さい時の下限は `radius × 0.01`(`radius` が0.001未満なら0.001に直す)。
- デバッグ表示の枠は、`RigidbodyComponent` が `Mesh capsuleOutline` を持ち、初めて描く時に作る(`Uninit` で解放)。Capsule2D は断面だけ(`axis=Z` の2Dは丸)。
- Sample3D は **敵だけ** `CapsuleRigidbodyComponent`(半径40・高さ100)に置き換えた。Player は見た目が立方体(100)で `GaugeRenderer` やカメラの高さに影響するため、安全側で **今のまま**(設計書の例の `CreateCapsule(50, 180)` は未適用)。Sample2D も置き換えず、細長い障害物も置いていない。
- `Physics::OverlapCapsule` は作っていない。
**依存する機能**: なし(単独)。

**未確定の点**:
- 2D用のクラス名は `Capsule2DRigidbodyComponent` でよいか(`Circle` / `Square` は「2D」を名前に入れていない)。別案: 3D用1つだけにして、2Dは `SetFreezePositionZ(true)` で使う
- Player の見た目を立方体からカプセルに変えてよいか(`GaugeRenderer` / カメラの高さがずれるのを直す)。当たり判定だけカプセルにして、見た目は立方体のまま、という案もある

---

### 6.3 マテリアル(実装済み)

**目的**: 色・画像・光沢(スペキュラ)・透明度・自発光(縁の光や枠線の項目を含む)を、1つの「マテリアル」にまとめる。今は `MeshRenderer::SetColor` / `SetTexture` / `SetUnlit` / `SetDepthWrite` がバラバラにある。ライト・影・枠線・水面の項目を足す入れ物になる。

**ファイル案**: `Engine/Material.h`(構造体だけ)。既存の変更: `Engine/Mesh.h/.cpp`・`Engine/SkinMesh.h/.cpp`・`Engine/MeshRenderer.h`・`Engine/ModelRenderer.h/.cpp`・`Shader3D.hlsl`・`Shader3DSkin.hlsl`(・5章で決める `Shader3DCommon.hlsli`)。

**関数**:
```cpp
// 見た目の設定をまとめた構造体
struct Material {
    // ── 今ある項目(MeshRenderer::SetColor / SetTexture / SetUnlit / SetDepthWrite と同じ意味) ──
    XMFLOAT4     color{ 1.f, 1.f, 1.f, 1.f };   // 色(aが透明度。1=不透明)
    std::wstring texture;                        // 画像(空なら白)
    bool         lit = true;                     // false=光の影響を受けない(スカイボックス)
    bool         depthWrite = true;              // false=自分より後ろの物を隠さない(半透明・スカイボックス)

    // ── 新しい項目(既定値は「今と同じ見た目」になる値) ──
    XMFLOAT3 specularColor{ 1.f, 1.f, 1.f };     // 光沢(ハイライト)の色
    float    specularStrength = 0.f;             // 光沢の強さ。0=ハイライト無し(既定。今と同じ)
    float    shininess = 32.f;                   // 光沢の鋭さ(大きいほどハイライトが小さく鋭い)
    XMFLOAT3 emissiveColor{ 1.f, 1.f, 1.f };    // 自分で光る色
    float    emissiveStrength = 0.f;             // 自発光の強さ。0=光らない(既定)
    bool     receiveShadow = true;               // 影を受けるか(6.5。影が無い間は意味なし)
    bool     castShadow = true;                  // 影を落とすか(6.5)
    XMFLOAT3 rimColor{ 1.f, 1.f, 1.f };         // 縁の光の色(6.6)
    float    rimStrength = 0.f;                  // 縁の光の強さ。0=無し(既定)
    float    rimPower = 3.f;                     // 縁の光が縁に集まる鋭さ(6.6)
    XMFLOAT4 outlineColor{ 1.f, 1.f, 0.f, 1.f };// 枠線の色(wは強さ)(6.6)
    float    outlineWidth = 0.f;                 // 枠線の太さ(ピクセル)。0=枠線無し(既定)(6.6)
};

// MeshRenderer / ModelRenderer に追加(今の SetColor などは残し、中でマテリアルの項目を書き換える)
void SetMaterial(const Material& material);   // まとめて設定(画像は Resources で読む)
Material& GetMaterial();                      // 手直し用: renderer->GetMaterial().emissiveStrength = 2.f;
```

**設計のポイント**:
- **既存のAPIは全部残します。** `SetColor` / `SetTexture` / `SetUnlit` / `SetDepthWrite` は、`Material` の項目を書き換える薄い関数にします(呼び出し側は直さなくてよい)。
- **持ち方**: マテリアルは `MeshRenderer` / `ModelRenderer` が**1体ごとに持つ**(`color` を1体ごとに持っているのと同じ)。画像だけは今まで通り `Resources` で共有する。Unityのように「マテリアルをファイルにして共有する」ことは今回しない。
- **定数バッファ `b2`(`MaterialBuffer`)**: 鏡面・自発光・縁の光・フラグを `float4` 4本程度にまとめて送ります。
  ```
  float4 specular;   // rgb=色, w=強さ        float4 emissive;   // rgb=色, w=強さ
  float4 rim;        // rgb=色, w=強さ        float4 params;     // x=shininess, y=rimPower, z=影を受ける(1/0), w=光を使う(1=lit)
  ```
  `Mesh::Draw` / `SkinMesh::Draw` に `Material` を受ける版を足し、今の引数版(`color`, `light`, `writeDepth`)は、既定のマテリアルで新しい版を呼ぶだけにします(`RigidbodyComponent::Draw` / `GroundController` などの既存の呼び出しは直さない)。
- **鏡面反射(ブリン・フォン)**: `H = normalize(L + V)`、`spec = pow(saturate(dot(N, H)), shininess) × specularStrength`。`V`(ピクセルからカメラへの向き)には**カメラの位置とワールド座標**が要ります。→ `PSInput` にワールド座標を足す(`Shader3D.hlsl` / `Shader3DSkin.hlsl` の両方。スキンメッシュは骨で動かした後の位置を使う)。カメラ位置は 6.4 の `b3`(`eyePos`)を使います。**6.3 を先に作る時は、`eyePos` だけを先に `b3` に入れておく**(6.4 で配列を足す)。
- **自発光**: 最後に `emissiveColor × emissiveStrength` を足すだけです(光の計算の後)。**本物の「にじむ光」(ブルーム)は別の機能**(画面全体を後から処理する仕組み)で、このマテリアルでは作りません。自発光は「暗い所でもその色で見える」までです。
- **透明度**: ブレンドは今も有効(`Mesh::CreateBlendState` が `SRC_ALPHA` / `INV_SRC_ALPHA`)で、`color.w` が効きます。**落とし穴**:
  - 描く順番が `drawPriority` → `z` の小さい順で、**カメラからの距離順ではない**(3章10番)。半透明の物は、不透明の物より後に描くよう `GameObject::SetDrawPriority` を大きくし、`depthWrite = false` にします。`MeshRenderer::SetMaterial` で `color.w < 1` の時に自動でそうするかは、7章で確認。
  - 半透明どうしの前後はずれることがある(カメラからの距離で並べ替える処理が `Scene::Draw` に無いため)。直す場合は `Scene::Draw` の変更(2Dにも影響)になるので、別の機能として扱う。
  - `clip(texColor.a - 0.01f)` は今のまま残す(ほぼ透明なピクセルを捨てる)。
- **`ModelRenderer` との関係**: FBXのパーツごとの画像は今まで通り各パーツの画像を使い、`SetTexture` で全パーツに同じ画像を貼れます。`Material` は全パーツ共通の設定(色・光沢・自発光・縁・枠線)として渡します。`SkinMesh` も同じ `b2` を使います。FBXに書かれたマテリアル(ufbxが読めるスペキュラ・自発光)の読み込みは、今回は**しません**(将来)。

#### 段階2: `Mesh` の作り直し(地形・影の前提。強く推奨)
3章3番のとおり、`Mesh::Draw` は毎回頂点をコピーする作りで、数万頂点の地形や、1フレームに2回描く影には使えません。マテリアルと同時に次を直します。
- **頂点バッファを一度だけ作る(`D3D11_USAGE_IMMUTABLE`)**。色は頂点にではなく、定数バッファ(`b2` の `color` として、頂点色に掛ける)で渡す。頂点の色がすべて白なので、結果は変わらない(3章3番)。`SkinMesh` が既にこの作り(`tint`)になっている。
- **インデックスバッファ対応**: `Mesh::InitIndexed(vertices, indices)` を足し、`Draw` が `DrawIndexed` を呼ぶ。頂点が使い回せるので、地形の頂点数が6分の1になる。
- **輪郭の線(`LINELIST`)・`CreateCube` など、今ある作り方は変えずに動く**。
- **落とし穴**: `Mesh` を `static` で共有しているものがある(`RigidbodyComponent.cpp` の `GetOutlineMesh`)。`Init` の呼び方は変えないこと。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 新規: `Engine/Material.h`、(任意)`Shader3DCommon.hlsli`
- 変更: `Shader3D.hlsl`・`Shader3DSkin.hlsl`(`b2`、ワールド座標の受け渡し、鏡面・自発光)、`Engine/Mesh.h/.cpp`・`Engine/SkinMesh.h/.cpp`(`MaterialBuffer`、`Draw` の新しい版、段階2)、`Engine/MeshRenderer.h`・`Engine/ModelRenderer.h/.cpp`、`Engine.vcxproj`(`.hlsli` を足す場合)
- `PhysX.props` は変えない
- **Sample3D**: 既定値は「今の見た目と同じ」になる(ハイライトなし・自発光なし)ので、**見た目は変わらないはず**。ただし `Shader3D.hlsl` を書き換えるので、全部の3Dオブジェクトの見た目が変わっていないか確認してもらう。
- **Sample2D**: 当たり判定のデバッグ表示(`Mesh`)を通して間接的に影響する。`Shader3D.hlsl` の変更が表示を壊していないか確認する(`RigidbodyComponent::SetDebugDrawEnabled(true)` で見る)。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D Sphere.cpp: 光沢のある球にする
auto* meshRenderer = sphere->AddComponent<MeshRenderer>(Mesh::CreateSphere());
meshRenderer->SetTexture(L"Assets/grid.png");
meshRenderer->GetMaterial().specularStrength = 0.8f;    // つやつや
meshRenderer->GetMaterial().shininess = 64.f;

// Sample3D Player.cpp: ぼんやり光るPlayer(自発光)
meshRenderer->GetMaterial().emissiveColor = { 0.2f, 0.6f, 1.f };
meshRenderer->GetMaterial().emissiveStrength = 0.3f;

// Sample3D Skybox.cpp: 今の SetUnlit(true) / SetDepthWrite(false) は、中でマテリアルの項目を書き換える(変更なし)
// Sample3D の半透明の板(新しく置く): color.w = 0.5f にして、SetDrawPriority で後に描く
```
対象: Sample3D の `Sphere`・`Player`・`Cube`。Sample2D は使わない(スプライトは `Shader.hlsl`)。

**依存する機能**: 5.3 リソースの使い回し(画像の共有)。

**実装して決めたこと(実装済み。段階1・段階2とも)**:
- **新しいファイル**: `Engine/Material.h`(`Material` と、シェーダーへ送る `MaterialConstantBuffer`)、`Shader3DCommon.hlsli`(`Engine.vcxproj` の `CopyRuntimeFiles` と `UpToDateCheckInput` に登録)。`Mesh.cpp` / `SkinMesh.cpp` の `D3DCompileFromFile` は `D3D_COMPILE_STANDARD_FILE_INCLUDE` に変えた。
- **`Material` の項目**: `color`・`texture`・`lit`・`depthWrite`・`specularColor`・`specularStrength`・`shininess`・`emissiveColor`・`emissiveStrength` の9個。**設計書の `receiveShadow` / `castShadow` / `rim*` / `outline*` は、まだ足していない**(使う機能 6.5・6.6 を作る時に足す。定数バッファ `params` の `y`/`z` を予約してある)。
- **シェーダー**: `Shader3DCommon.hlsli` に `b0`(今まで通り)・`b2`(`MaterialBuffer`: `color` / `specular` / `emissive` / `params`)・`b3`(`CameraBuffer`: `eyePos` だけ)・`PSInput`(`worldPos` を追加)・`ShadePixel()` を置き、`Shader3D.hlsl` と `Shader3DSkin.hlsl` は頂点の動かし方と `ShadePixel()` の呼び出しだけにした。`b1`(骨)の `tint` は `b2` の `color` に移して削除した。
- **鏡面**は `rgb += specularColor × lightColor × pow(saturate(dot(N, H)), shininess) × specularStrength`(設計書の式に、平行光源の色を掛けた。`lit = false` の時と、光が当たらない面(`dot(N, L) <= 0`)では出さない)。**自発光**は光の計算の後に足すだけ。
- **持ち方**: `MeshRenderer` / `ModelRenderer` が `Material` を1体ごとに持つ。`material.texture`(パス)は、描く時(と `SetMaterial` / `SetTexture` の時)に `Resources` で借り直す。`SetColor` / `SetTexture` / `SetUnlit` / `SetDepthWrite` は残し、`Material` の項目を書き換えるだけにした。`ModelRenderer::SetTexture` は `material.texture` を書き換える。`Mesh::Draw` / `SkinMesh::Draw` に `Material` 版を足し、色指定の版は既定のマテリアルで呼ぶだけ(`RigidbodyComponent` / `VisionComponent` / `GroundController` は変更なし)。`MeshRenderer` の `unlit` は、`fullBright` の `Light` を渡すのをやめて、`material.lit = false` で伝える(結果は同じ)。
- **段階2(`Mesh` の作り直し)**: 頂点バッファは `D3D11_USAGE_IMMUTABLE` で `Init` の時に1回だけ作る(`vertices` の保持と毎回のコピーはやめた)。色は `b2` の `color` として頂点の色に掛ける(今ある形は頂点の色がすべて白なので同じ見た目)。`Mesh::InitIndexed(頂点, インデックス, topology)` を追加(32ビットのインデックス。`Draw` は `DrawIndexed`)。`Init(頂点, topology)` の呼び方は変えていない(`RigidbodyComponent.cpp` の `static` なアウトラインメッシュもそのまま動く)。同じ `Mesh` に `Init` を2回呼んだ時は、前のバッファを解放する(参照カウントは増やさない)。
- **透明(7章の確認事項)**: **自動では変えない**。`color.w < 1` にしても `depthWrite` と `drawPriority` は勝手に変えない(`Sample3D` の `GlassPanel` で、使う側が `SetDrawPriority` と `depthWrite = false` を書く)。半透明どうしの前後がずれる問題は、そのまま。
- **Sample**: `Sample3D` に `GlassPanel`(半透明・光沢・自発光の板。`GameScene` に置いた)を追加し、`Sphere.cpp`(今のシーンでは未使用)に光沢を付けた。`Player` の自発光(設計書の例)は、見た目が変わるので付けていない。
- **確認**: WARP(ソフトウェアのD3D11)で、旧実装(同じ描画を再現したコード)と新しい `Mesh` / `SkinMesh` を描き比べて、色指定の描き方・`unlit`・輪郭の線・半透明・スキンメッシュのすべてでピクセルが完全に一致することを確認した。`InitIndexed` の立方体(24頂点)も通常の立方体と一致。光沢・自発光・色・透明度・`lit` を変えると見た目が変わることも確認した。
- **FBXに書かれたマテリアルの読み込みと、`MeshRenderer` / `ModelRenderer` を通した描画は、WARP では確かめていない**(実機で確認してもらう)。

**未確定の点(決まったこと)**:
- 段階2(`Mesh` の作り直し)を、6.3 の中でやるか → **6.3 の中でやった**
- 透明な `Material`(`color.w < 1`)で、`depthWrite` と `drawPriority` を自動で変えてよいか → **自動では変えない**(上の「透明」)
- 光の計算を `Shader3DCommon.hlsli` にまとめるか(5.2) → **まとめた**

---

### 6.4 ポイントライト(点光源)とスポットライト

**目的**: 今の「全体に同じ方向から当たる光(平行光源)」に、ランタンのように**周りを照らす光(ポイントライト)**と、懐中電灯のように**円すい形に照らす光(スポットライト)**を足す。

**ファイル案**: `Engine/LightComponent.h/.cpp`(付けたオブジェクトの位置から光る)、`Engine/Lighting.h/.cpp`(`namespace Lighting`。光の一覧を集めて、GPUへ送る)。既存の変更: `Shader3D.hlsl`・`Shader3DSkin.hlsl`・`Engine/Mesh.cpp`・`Engine/SkinMesh.cpp`・`Engine/GameEngine.cpp`。

**関数**:
```cpp
enum class LightType { Point, Spot };

// GameObjectに付ける光。位置はオーナーの Transform(ワールド座標)。スポットの向きはオーナーの正面(Math::Forward3D)または手で指定
class LightComponent : public Component {
public:
    explicit LightComponent(LightType type = LightType::Point);

    void SetColor(float r, float g, float b);    // 光の色(既定 白)
    void SetIntensity(float intensity);          // 強さ(既定 1。2にすると2倍明るい)
    void SetRange(float range);                  // 光が届く距離(ここで0になる。既定 800)

    // スポットライトだけ
    void SetDirection(XMFLOAT3 direction);       // 向き(長さ0に戻すと、オーナーの正面を使う)
    void SetSpotAngle(float innerDegrees, float outerDegrees);   // 内側(全開)と外側(消える)の角度。内側 < 外側(既定 20 / 35)

    void Init() override;     // Lighting に登録
    void Uninit() override;   // 登録を外す
};

namespace Lighting {
    constexpr int maxLights = 8;          // 同時に使える光の数(超えたらカメラに近い順に8個)

    // ── Engine内部用(GameEngine と Mesh/SkinMesh だけが呼ぶ) ──
    void Register(LightComponent* light);
    void Unregister(LightComponent* light);
    void Update();        // 毎フレーム1回(描画の前)。アクティブな光を集め、カメラに近い順に maxLights 個を選んで b3 に書く
    void Bind();          // Mesh / SkinMesh が描く時に、b3 をシェーダーにセットする
}
```

**設計のポイント**:
- **平行光源は今のまま**(`Image::GetLight()`、`b0`)。点光源・スポットは**新しい仲間**として `b3` に入れます(5.1)。新しい光を1つも置かない限り、見た目は変わりません。
- **Componentにするか、Scene/Engine側の登録にするか → Component(`LightComponent`)を推奨します。** 理由: ①光の位置はオブジェクトに付いて動く(敵やPlayerが持つランタン) ②`SetActive(false)` / `Destroy` で消える、他のComponentと同じ使い方 ③`CLAUDE.md` の「見た目や機能はComponent」の考え方に合う。Engine側には、集めるための**登録簿(`Lighting`)**だけを持ちます(`Init` で登録、`Uninit` で外す。`ObjectPool` の再利用で `SetActive(false)` になった光は、集める時に `GetOwner()->GetIsActive()` で除く)。
- **`b3` の中身(案)**: 1つの光 = `float4` 4本(64バイト)× 8個 = 512バイト + カメラ位置と光の数。
  ```
  struct GpuLight {
      float4 positionRange;   // xyz=位置, w=届く距離
      float4 colorType;       // rgb=色×強さ, w=種類(0=無し, 1=点, 2=スポット)
      float4 directionOuter;  // xyz=向き(スポット), w=cos(外側の角度)
      float4 innerPad;        // x=cos(内側の角度)
  };
  cbuffer LightBuffer : register(b3) { float4 eyePos; int4 counts; GpuLight lights[8]; };
  ```
- **減衰(遠いほど暗くなる式)**: 「届く距離 `range` でちょうど0になる」式を推奨します(`atten = saturate(1 − (d / range)²)²`)。物理的な `1/d²` は、遠くでいつまでも0にならず、`range` の外でも計算が必要になるため、ゲームでは範囲つきの式が扱いやすい。このエンジンは「100単位=1m」なので、`range` の既定は800(8m)にする。
- **スポットの角度**: 向き `S` と「ピクセルへの向き」の内積を `cosθ` と比べて、`smoothstep(cos(外側), cos(内側), dot)` で、内側は全開・外側で0に滑らかにつなぐ(角が出ない)。
- **ピクセルごとの計算**: `for (int i = 0; i < counts.x; i++)` で最大8個を足す。法線 `N` と「ピクセルから光への向き」 `L` の内積(`saturate(dot(N, L))`)に、色・減衰を掛けます。鏡面反射(6.3)も同じループで足します。**頂点ごとではなくピクセルごと**にする(ライトの近くで面が大きいと、頂点ごとでは光が見えなくなるため。そのために `PSInput` にワールド座標を足す=6.3で足済み)。
- **選び方(落とし穴)**: 光が9個以上ある時は、「カメラに近い順の8個」にします。**オブジェクトごとに近い光を選ぶ**方式は、今回しません(フレームに1回だけ選ぶので、遠くの大きなオブジェクトが近くの光を無視することがある)。
- **法線の落とし穴(今もあるが目立つようになる)**: 今のシェーダーは法線に `world` をそのまま掛けています。`scale` が均等(`{100,100,100}`)なら問題ありませんが、**不均等な拡大縮小(地面のタイル `{1000, 20, 1000}`)の法線は、球などでは曲がります**。立方体(法線が軸に沿う)では問題が出ません。点光源は法線の向きが見た目にはっきり出るので、必要なら「法線行列(逆転置行列)」を使う(`b0` にもう1つ行列を足す=`b0` を変える必要があるので、気になった時に別の相談とする)。
- **2D(Sample2D)**: スプライト(`Shader.hlsl`)は光を計算しないので、**光は2Dには効きません**。2Dの明るさは今回の範囲外(必要なら別の機能として相談)。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 新規: `Engine/LightComponent.h/.cpp`、`Engine/Lighting.h/.cpp`、`Engine.vcxproj(.filters)`
- 変更: `Shader3D.hlsl`・`Shader3DSkin.hlsl`(`b3`、ループ)、`Engine/Mesh.h/.cpp`・`Engine/SkinMesh.h/.cpp`(`b3` の作成・`Bind`)、`Engine/GameEngine.cpp`(`Lighting::Update()` を、`Visibility::BeginFrame()` の次あたりに1行)
- `PhysX.props`・`Light.h`(平行光源)は変えない
- Sample3D: 光を置くまで見た目は変わらない。Sample2D: 変わらない(デバッグ表示の `Mesh` が `b3` を使うが、光の数が0なので同じ)。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D Player.cpp: Playerがランタンを持つ(周りだけ明るい。環境光を少し暗くする)
auto* lantern = player->AddComponent<LightComponent>(LightType::Point);
lantern->SetColor(1.0f, 0.8f, 0.5f);
lantern->SetRange(900.f);

// Sample3D のカメラの正面を照らす懐中電灯(スポット): CameraController が毎フレーム向きを渡す
auto* torch = player->AddComponent<LightComponent>(LightType::Spot);
torch->SetSpotAngle(15.f, 30.f);
// CameraController::Update
torch->SetDirection(cameraForward);

// Sample3D の GameScene で環境光を暗くして、光の効果が分かるようにする
Image::GetLight().ambient = { 0.1f, 0.1f, 0.12f };
```
対象: Sample3D の `Player`(光を持たせる)、`CameraController`(スポットの向き)。

**依存する機能**: 6.3(`PSInput` のワールド座標・鏡面反射・`eyePos`)。

**未確定の点**:
- 最大数8でよいか(多いほど重い。このゲームでは足りるはず)
- Sample3D の環境光を暗くして光の効果を見せてよいか(ゲームの見た目が変わる)
- 敵・弾・爆発にも光を付けるか(例: 爆発 `CreateExplosion` が一瞬光る)

---

### 6.5 影(シャドウマップ)

**目的**: 立体のオブジェクトが地面や他のオブジェクトに落とす影を描く。まず平行光源(太陽)の影から作り、余裕があればスポットライトの影も作る。

> **この機能は一番大きく、難しいです。** 描画の流れそのもの(`GameEngine::Run`)に新しい「影のパス」が加わります。下の「難しい・大きい点」と「段階分け」を必ず読んでください。

**ファイル案**: `Engine/Shadow.h/.cpp`(`namespace Shadow`。影の深度テクスチャ・描画の準備・後片付け)。既存の変更: `Shader3D.hlsl`・`Shader3DSkin.hlsl`(影の深度を書くエントリ `VSShadow` を足し、本番のピクセルシェーダーで影を引く)、`Engine/Mesh.h/.cpp`・`SkinMesh.h/.cpp`(`DrawDepth` を足す)、`Engine/Component.h`・`Engine/GameObject.h`・`Engine/Scene.h/.cpp`・`Engine/SceneManager.h/.cpp`(影のパスで「影を落とす物」を呼ぶ口)、`Engine/MeshRenderer.h`・`Engine/ModelRenderer.h/.cpp`、`Engine/GameEngine.cpp`。

**関数**:
```cpp
namespace Shadow {
    // 影の設定(1つだけ。平行光源 Image::GetLight() の向きで作る)
    struct Settings {
        bool  enabled = false;          // 既定はOFF(今の見た目のまま)
        int   resolution = 2048;        // 影のテクスチャの大きさ(正方形。1024/2048/4096)
        float distance = 3000.f;        // カメラの前方、この距離の範囲に影を作る(広いほどぼやける)
        float depthBias = 0.0015f;      // 影のにじみ(影のつぶつぶ=シャドウアクネ)を消すための補正
        float normalBias = 2.0f;        // 法線方向へずらす補正(ワールド単位)
        float strength = 0.6f;          // 影の濃さ(0=影なし、1=真っ黒)
        int   pcfRadius = 1;            // 影の縁のぼかし(0=ぼかさない、1=3x3、2=5x5)
    };
    void SetSettings(const Settings& settings);
    Settings& GetSettings();

    // ── Engine内部用(GameEngineだけが呼ぶ) ──
    bool Init();          // 深度テクスチャ・ステート・サンプラーを作る
    void Render();        // 影のパス: 深度テクスチャに「影を落とす物」を描く → 描画先を元に戻す
    void Bind();          // Mesh/SkinMesh が描く時に、b4 / t1 / s1 をセットする
    void Uninit();
}

// Component に追加(既定は何もしない)。影を落とす物(MeshRenderer / ModelRenderer)が override する
virtual void DrawShadow() {}
```

**設計のポイント(仕組み)**:
1. **影のパス(1回目)**: 太陽の向きから見た「深度」だけを、専用の深度テクスチャ(`R32_TYPELESS`。書く時は `D32_FLOAT` のDSV、読む時は `R32_FLOAT` のSRV)に描く。色は描かない(ピクセルシェーダー無し)。
2. **本番のパス(2回目、今の描画)**: ピクセルごとに、「そのピクセルを光から見た位置」を求め、影の深度と比べる。光から見て手前に別の物があれば影。
- **光から見た行列**: 太陽の向きから `XMMatrixLookAtLH`(カメラの注視点の少し後ろから、光の向きに見る)と、`XMMatrixOrthographicLH`(平行光源なので正射影)を作る。範囲は「カメラの前方 `distance` の範囲を含む箱」。世界全体(Groundは100000四方)を1枚に入れると解像度が足りず、ぼやける。
- **深度バイアス(落とし穴)**: 影の判定は浮動小数点の誤差があるので、そのままだと自分自身に影が出る(シャドウアクネ=縞模様)。①影のパスのラスタライザに `DepthBias` / `SlopeScaledDepthBias` を設定 ②本番側で `depthBias` を引く ③法線方向に少しずらす(`normalBias`)、の組み合わせで調整する。逆に補正が大きすぎると、影が物から離れて浮く(ピーターパン現象)。**数値の調整が必要で、この機能の一番の手間**。Sample3D の「100単位=1m」のスケールに合わせた既定値を見つける。
- **PCF(影の縁をぼかす)**: 比較サンプラー(`D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT`、`D3D11_COMPARISON_LESS_EQUAL`)と `SampleCmpLevelZero` を使うと、2x2の補間がGPUでされる。`pcfRadius = 1`(3x3の9回)で十分なぼかしになる。
- **範囲の外**: 影のテクスチャの外は影なし(明るい)にしたい。サンプラーのアドレスを `D3D11_TEXTURE_ADDRESS_BORDER`、境界色を1.0にする。
- **カメラが動くと影がちらつく(落とし穴)**: 影の範囲がカメラに追従して動くと、テクセル1つ分ずつずれて縁がチラチラする。光の座標系でカメラ位置を**テクセルの大きさの整数倍に丸める**(スナップ)。

**難しい・大きい点(正直に)**:
- **描画の流れが変わる**: `Image::BeginFrame` が画面をクリアして、レンダーターゲットとビューポートを設定したあと、影のパスで**レンダーターゲットとビューポートを一時的に切り替え**(深度テクスチャだけに描く、解像度が違う)、終わったら**必ず元に戻す**。戻し忘れると、画面が真っ黒になる。
- **Direct2D(`Text` / `ScreenEffect` / シーン切り替え)との順番**: 影のパスは、`Text::BeginDraw()` の**前**(`CameraShake::BeginDraw()` の後)に完了させる。Direct2D の描画の途中でレンダーターゲットを切り替えない。
- **すべての「影を落とす物」を2回描く**: `Scene::Draw` は `Component::Draw` を呼ぶ作りなので、影のパスで「深度だけ描く」ために、`Component` に `DrawShadow()` という口を新しく足し、`GameObject` → `Scene` → `SceneManager` まで同じように通す(`Draw` と同じ形)。この変更は**Engineの根幹(`Component.h` / `GameObject.h` / `Scene.h/.cpp` / `SceneManager.h/.cpp`)**で、両サンプルのすべてのオブジェクトに影響する(`DrawShadow` を override しなければ何も起きない)。別案は、`MeshRenderer` / `ModelRenderer` が `Shadow` に自分を登録する方式(`Lighting` と同じ。根幹を触らずに済むが、アクティブ・有効の判定を自分で作る必要がある)。どちらにするかは 7章。
- **`Mesh::Draw` の作りが前提(6.3 段階2)**: 今の `Mesh::Draw` は毎回頂点をコピーする。影のパスで同じ物をもう一度描くと、コピーが2倍になり重い。6.3 の段階2(頂点バッファを一度だけ作る)が**前提**です。
- **スキンメッシュ(骨のモデル)の影**: 骨で曲げた後の位置で深度を書く必要があるので、`Shader3DSkin.hlsl` にも `VSShadow` が要ります。段階分けで後にします。
- **敵(スプライトのビルボード)は影を落とせません**。`Shader.hlsl` の画像(`SpriteRenderer`)は、影のパスに出ない(板の影は不自然)。代わりに、足元に**丸い影(ブロブシャドウ)**を置く案があります(安く見栄えがする。7章で確認)。
- **性能**: テクスチャ 2048x2048 の `D32_FLOAT` = 16MB、4096x4096 = 64MB。影のパスは「影を落とす物の数」だけ描画命令が増える。Sample の規模なら問題ないが、`resolution` を設定で変えられるようにして、重ければ下げる。
- **実機GPUでの見た目**: 影のバイアスは、グラフィックスドライバや解像度で見え方が変わる。必ずユーザーが遊んで確認する。

**段階分け(提案)**:
- **段階1**: 平行光源だけ、`Mesh`(`MeshRenderer`)が影を落とし・受ける。PCF 3x3。解像度は設定。静止した `ModelRenderer` のパーツも落とす。
- **段階2**: スキンメッシュ(`SkinMesh`)が影を落とす。透明部分が多い画像(アルファの切り抜き)の影。
- **段階3**: スポットライトの影(透視投影で、光の位置から見た深度。同時に影を作る光は1つだけ)。
- **将来(今回は作らない)**: カスケード(遠近で影の解像度を変える)、点光源の影(キューブマップ)、柔らかい影(PCSS)。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 新規: `Engine/Shadow.h/.cpp`、`Engine.vcxproj(.filters)`
- 変更: `Shader3D.hlsl`・`Shader3DSkin.hlsl`、`Engine/Mesh.h/.cpp`・`Engine/SkinMesh.h/.cpp`、`Engine/Component.h`・`Engine/GameObject.h`・`Engine/Scene.h/.cpp`・`Engine/SceneManager.h/.cpp`、`Engine/MeshRenderer.h`・`Engine/ModelRenderer.h/.cpp`、`Engine/GameEngine.cpp`(影のパスの呼び出し)、`Engine/Graphics.h/.cpp`(必要なら、元のビューポートを戻す補助)
- `PhysX.props` は変えない
- **`Shadow::GetSettings().enabled = false` の間は影のパスを実行しないので、見た目は今と同じ**(既定OFF)。両サンプルとも、ONにしたシーンだけが変わる。Sample2D は影を使わない。ただし根幹のファイルを触るので、**Sample2D も含めて全体が正しく動くことを必ず確認**する。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D GameScene::Init: 影をONにする(平行光源の向きは Image::GetLight().direction)
Shadow::GetSettings().enabled = true;
Shadow::GetSettings().distance = 2500.f;

// Sample3D Player / Sphere / Cube: 既定で影を落とす・受ける(Material::castShadow / receiveShadow が既定 true)
// Sample3D GroundController のタイル: Mesh::Draw の既定のマテリアル(receiveShadow = true)で影を受ける
// Skybox: meshRenderer->GetMaterial().receiveShadow = false;  castShadow = false;
// (敵は画像のビルボードなので影を落とさない。丸い影の案は7章で確認)
```
対象: Sample3D の Player・Sphere・Cube・ModelDisplay・Ground・Skybox。

**依存する機能**: 6.3(段階2)、6.4(光の向き・位置)。

**未確定の点**:
- 影の範囲(`distance`)と解像度の既定値でよいか(2048、3000)
- `DrawShadow` を `Component` に足す方式にするか、登録簿の方式にするか(根幹のファイルを触るかどうか)
- 敵(画像)の足元に丸い影を置くか
- スポットライトの影(段階3)まで作るか、段階1〜2で止めるか

---

### 6.6 枠線を光らせる(アウトライン・リム発光)

**目的**: 物の縁を色付きで光らせて、選んだ物・取れる物・敵を目立たせる。

**方式の比較**:
| 方式 | 仕組み | 長所 | 短所 |
|---|---|---|---|
| **A. 法線拡大の2パス**(逆さの殻) | 同じ物を、法線方向に少し大きくして、裏面だけ単色で描く(表面は消える)。その上に本来の物を描く | 外側にはっきりした枠線が出る。太さを自由に決められる。骨のモデルにもできる | 法線を平均した頂点データが必要(立方体のように面ごとに法線が違う物は、角に隙間ができる)。描画が1回増える |
| **B. ステンシル** | 物を描いて、ステンシルに印を付ける。少し大きくした物を、印の無い所だけ単色で描く | 法線に頼らない | 深度バッファに「ステンシルがある」(3章11番)が、今は毎フレームのクリアをしていない。中心から拡大するので、凹んだ形・原点がずれた物は枠がずれる。ステンシルの設定が増える |
| **C. リムライト** | ピクセルシェーダーで、「見る向き」と「法線」がなす角が大きい(縁に近い)ほど明るくする | 描画が増えない。どの物にも1行で付く。骨のモデルにも付く | 物の「内側」に光るだけで、外側には出ない。立方体のような平らな面では縁に光が出にくい |

**おすすめ**: 
- **段階1 = C. リムライト**: シェーダーに数行足すだけで、`Material` の項目で操作できて安全。「光る」感じは最小の手間で出せる。
- **段階2 = A. 法線拡大の2パス**: 外側の枠線が必要な時に足す。法線は「同じ位置の頂点の法線を平均した版」を、`Mesh` が初期化の時に別の頂点バッファ(または頂点データの別の枠)で用意する(段階2のMesh作り直しと一緒に作ると楽)。
- B(ステンシル)は、Aで隙間が出る形の保険として残す(今回は作らない)。

**ファイル案**: 新しいファイルは不要。`Material`(6.3)の項目(`rimColor` / `rimStrength` / `rimPower` / `outlineColor` / `outlineWidth`)を使う。変更: `Shader3D.hlsl`・`Shader3DSkin.hlsl`(リム、`VSOutline` / `PSOutline`)、`Engine/Mesh.h/.cpp`・`SkinMesh.h/.cpp`(`DrawOutline`、表面を捨てるラスタライザ)、`Engine/MeshRenderer.h`・`Engine/ModelRenderer.h/.cpp`。

**関数**:
```cpp
// MeshRenderer / ModelRenderer に追加(中でマテリアルの項目を書き換えるだけ)
void SetOutline(XMFLOAT4 color, float width);                        // 外側の枠線(段階2)。width=ピクセル。0で消す
void SetRimLight(XMFLOAT3 color, float strength, float power = 3.f); // 縁の光(段階1)

// Mesh / SkinMesh に追加(MeshRenderer などが、本来の物を描いた直後に呼ぶ)
void DrawOutline(const XMMATRIX& world, const Camera& camera, XMFLOAT4 color, float widthPixels);
```

**設計のポイント**:
- **リムライト(段階1)**: ピクセルシェーダーに、`V`(ピクセルからカメラへ。`b3` の `eyePos` から)と法線 `N` で、`rim = pow(1 − saturate(dot(N, V)), rimPower) × rimStrength` を求め、`rimColor × rim` を最後に足す。`rimPower` が大きいほど縁の細い線だけ光る。6.3 のワールド座標の受け渡し・6.4 の `eyePos` を使う。
- **法線拡大(段階2)の細かい点**:
  - 頂点シェーダーでクリップ空間に出した後、法線のクリップ空間での向きの方へ `width / 画面の高さ × w` だけずらすと、**遠くても近くても同じピクセルの太さ**になる(ワールド単位で膨らませると、遠くの物の枠線が細くなって消える)。
  - 表面を捨てて裏面だけ描く、新しいラスタライザ(`D3D11_CULL_FRONT`)が必要。今の `Mesh` は `CULL_NONE` で描いている。
  - 平均した法線(同じ位置の頂点の法線の合計)を使わないと、立方体の角で枠線が割れる(法線が面ごとに違うため)。
  - **枠線は物の後ろ側にも出るので、他の物にめり込んで隠れる(深度テストが働く)**。`Scene::Draw` の順で本来の物の直後に描くため、前の物には隠される。それで自然。
- **「光る」の限界**: 枠線の色を明るくしても、画面の外にはにじみません(ぼかした光=ブルームは別の仕組み)。**ブルーム(画面全体を暗くぼかして足す)は、今回の範囲外**です(大きな別機能。必要なら 6.x として追加を相談)。
- **加算合成で光らせる案**: 枠線を加算ブレンド(`Mesh` に `blendStateAdditive` を足す)で描くと、背景が明るく透けて「光って」見えやすい。段階2の中で試す。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 変更: `Shader3D.hlsl`・`Shader3DSkin.hlsl`、`Engine/Mesh.h/.cpp`・`SkinMesh.h/.cpp`、`Engine/MeshRenderer.h`・`Engine/ModelRenderer.h/.cpp`。新しいファイルなし。
- `PhysX.props` は変えない
- 枠線・リムは**既定OFF**(`outlineWidth = 0`、`rimStrength = 0`)なので、見た目は変わらない。Sample2D は使わない(デバッグ表示の `Mesh` への影響の確認だけ)。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D Player.cpp: 縁が青く光るPlayer
meshRenderer->SetRimLight({ 0.3f, 0.7f, 1.f }, 1.5f);

// Sample3D: Playerが近づくと光る球(取れるアイテムの合図)
//   Sphere用の SphereController::Update:
auto nearby = Physics::OverlapSphere(sphere->transform.position, 400.f, "Player");
meshRenderer->SetOutline({ 1.f, 0.9f, 0.2f, 1.f }, nearby.empty() ? 0.f : 3.f);
```
対象: Sample3D の Player(リム)、Sphere(枠線)。敵はスプライトなので対象外(スプライトの枠線は別の仕組みが要る)。

**依存する機能**: 6.3(`Material` の項目・ワールド座標)、6.4(`eyePos` の `b3`)。

**未確定の点**:
- 「光る」を、ブルーム(画面全体をにじませる)まで期待するか。しない前提で段階1(リム)+段階2(法線拡大)でよいか
- 敵(画像)にも枠線を付けたいか(必要なら、画像用の別の仕組みを相談)

---

### 6.7 地形

**目的**: 高さの画像(白=高い、黒=低い)または表(CSV)から、起伏のある地面の見た目(メッシュ)を作る。今の地面は、平らなタイルを敷き詰めているだけ(`GroundController`)。

**ファイル案**: `Engine/Terrain.h/.cpp`(高さのデータと、高さ・法線を調べる関数)、`Engine/TerrainRenderer.h/.cpp`(描画する Component)。`Assets/Terrain/`(高さの画像。サンプルを1枚作る)。

**関数**:
```cpp
// 地形の設定
struct TerrainSettings {
    float width = 10000.f;       // X方向の大きさ
    float depth = 10000.f;       // Z方向の大きさ
    float maxHeight = 1000.f;    // 画像が真っ白(CSVなら最大値)の時の高さ。黒=0
    int   cellsX = 128;          // X方向の分割数(マスの数。頂点は +1 個)。画像より大きくはしない
    int   cellsZ = 128;          // Z方向の分割数
    float uvRepeat = 20.f;       // 画像を地形全体に何回くり返して貼るか
    std::wstring texture;        // 地形に貼る画像(空なら白)
};

// 高さのデータ(Resources のようにみんなで共有できるよう shared_ptr で渡す)
class Terrain {
public:
    // 高さの画像(PNG。グレースケールかRGBA。Rの値を使う)から作る。読めなければ nullptr
    static std::shared_ptr<Terrain> LoadFromImage(const std::wstring& path, const TerrainSettings& settings);
    // CSV(1行目は列の名前 "x0,x1,x2,...")から作る。1マス1つの数値(0〜1)
    static std::shared_ptr<Terrain> LoadFromCsv(const std::wstring& path, const TerrainSettings& settings);

    // 地形の左手前の角を原点(0,0)とした座標で調べる(ワールドへの変換は TerrainRenderer がやる)
    float    GetHeightAt(float localX, float localZ) const;    // その場所の高さ(面の上の高さ。範囲外は端の高さ)
    XMFLOAT3 GetNormalAt(float localX, float localZ) const;    // その場所の面の向き(長さ1)
    bool     Contains(float localX, float localZ) const;       // 地形の範囲内か
    const TerrainSettings& GetSettings() const;
    // 高さの配列(行=X方向、列=Z方向。コライダー(6.8)も同じデータを使う)
    const std::vector<float>& GetHeights() const;
    int GetVertexCountX() const;   // cellsX + 1
    int GetVertexCountZ() const;   // cellsZ + 1
};

// 地形を描く Component。オーナーの Transform が地形の左手前の角(X・Zが最小の角)の位置で、Y が高さ0
class TerrainRenderer : public Component {
public:
    explicit TerrainRenderer(const std::shared_ptr<Terrain>& terrain);
    void SetMaterial(const Material& material);   // 6.3 のマテリアル(画像は TerrainSettings::texture が既定)
    // ワールド座標で調べる(オーナーの位置を足し引きして Terrain の関数を呼ぶ)
    float GetHeightAt(float worldX, float worldZ) const;
    XMFLOAT3 GetNormalAt(float worldX, float worldZ) const;
    const std::shared_ptr<Terrain>& GetTerrain() const;
    void Draw() override;
};
```

**設計のポイント**:
- **向きと座標**: `Terrain` の中は「左手前の角が原点、X・Z方向が増える向き」。ワールドに置く位置は `TerrainRenderer`(のオーナー)の `transform.position`。**原点を中心にしない理由**: Unityの Terrain と同じで、PhysXの高さフィールド(6.8)も角が原点だから。ずらす計算が1か所で済む。
- **高さの画像を読む**: 今の `Texture::Load` は GPU 用のテクスチャしか返さず、**画素の値(CPU側)は読めません**。`Terrain` が WIC(`IWICImagingFactory`。`SpriteTransition` などが既に WIC で画像を読んでいる)で画像を読み、`GUID_WICPixelFormat32bppRGBA` に変換して R の値を高さにします。
  - **落とし穴**: 8ビットの画像は 256 段階しかなく、`maxHeight = 1000` だと 4 ごとの階段状になります(法線をなめらかに計算すれば、見た目はあまり目立たない)。高さの精度が欲しければ、16ビットのグレースケールPNG(`GUID_WICPixelFormat16bppGray`)に対応する案(65536段階)。段階1は8ビット + 隣との平均でなめらかにする。
  - 画像のサイズ(例 257x257)より `cellsX` を大きくしたら、画像を双線形補間で引き伸ばす。画像より細かくしても情報は増えない。
- **CSV**: 今の `Csv` クラスは「1行目が列の名前、2行目からデータ」(`GetFloat(row, "列の名前")`)。高さの表では 1行目に `x0,x1,x2,...` を置き、`GetFloat(row, "x" + 番号)` で読む。**CSV の書き方が決まりごとになるので、README に書く**。画像を先に作り、CSV は段階2でもよい(7章で確認)。
- **メッシュの作り方**: 縦横の頂点(`(cellsX+1) × (cellsZ+1)`)を並べ、1マスを2枚の三角形にする。
  - 頂点データは `MeshVertex`(位置・法線・色(白)・UV)。法線は隣の頂点との高さの差から計算(中央差分)。UV は `u = x / width × uvRepeat`、`v = z / depth × uvRepeat`。
  - **インデックスバッファが前提**(6.3 段階2)。128x128 のマスなら頂点 16641 個・インデックス 98304 個。インデックスなしだと頂点が98304個になり、かつ今の `Mesh::Draw` が毎回コピーするので、重くて使えない(3章3番)。分割数の上限は 256(頂点は 66049 個。`uint32` のインデックス)にして、それ以上は警告して丸める。
  - **三角形の対角線の向き**: 1マスを2枚に分ける対角線は、見た目の形が変わります。**PhysXの高さフィールド(6.8)の対角線とそろえる**必要があります(`PxHeightFieldSample` の `tessFlag`)。
- **`GetHeightAt` の計算**: 範囲内のマスを求め、そのマスの2枚の三角形のうち、点が入っている方の面の高さを返す(三角形の上の補間)。**描画のメッシュと同じ三角形**でないと、物を地形の上に置いた時に浮いたり埋まったりする。
- **描画範囲を絞る(任意、段階2)**: 地形を 32x32 マスずつの「チャンク」に分け、それぞれにバウンディング球を持たせて、`Visibility::IsSphereInView3D`(6.1)で見えるチャンクだけ `DrawIndexed` する。128x128 程度なら全部描いても軽いので、まず1回の描画で作り、重ければチャンクにする。
- **テクスチャ**: 1枚の画像をくり返して貼る(サンプラーは `WRAP` なので、`uvRepeat` を大きくするだけでくり返す)。高さで画像を混ぜる(草・岩・雪)のは、今回は作らない。
- **法線の落とし穴**: 地形の `transform.scale` は `{1,1,1}` から変えない(大きさは `TerrainSettings` で決める)。不均等な拡大縮小があると、法線がずれる(6.4の落とし穴)。
- **地形の外**: 地形の範囲の外は何もない(落ちる)。Sample3D は今、見えない大きな床(100000四方)を持っている。地形の外側にも床が必要かどうかは 7章で確認。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 新規: `Engine/Terrain.h/.cpp`、`Engine/TerrainRenderer.h/.cpp`、`Engine.vcxproj(.filters)`
- 変更: 6.3 の段階2で `Mesh` のインデックス対応を作るので、**6.7 では新しい変更は(WIC の画像読み込みを除き)少ない**
- `Shader*.hlsl`・`PhysX.props` は変えない
- Sample2D: 影響なし。Sample3D: 地形を使うまでは何も変わらない。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D Ground.cpp: 平らなタイルと見えない大きな床を、地形に置き換える
auto terrain = Terrain::LoadFromImage(L"Assets/Terrain/heightmap.png", settings);
GameObject* ground = scene.CreateObject("Ground");
ground->transform.position = { -settings.width * 0.5f, -50.f, -settings.depth * 0.5f };   // 原点が地形の中心にくる位置に角を置く
ground->SetDrawPriority(INT_MIN + 1);
auto* renderer = ground->AddComponent<TerrainRenderer>(terrain);
ground->AddComponent<TerrainRigidbodyComponent>(terrain);       // 6.8

// Sample3D EnemyManager::Update: 敵を出す高さを、その場所の地形の高さにする
// 今:  XMFLOAT3 pos = { target.x + offset.x, spawnHeight, target.z + offset.y };
// 後:  float groundY = terrainRenderer->GetHeightAt(x, z);   pos.y = groundY + spawnHeight;
// (terrainRenderer は CreateEnemySpawner(scene, player, terrain) の引数で受け取る)

// Sample3D GroundController.cpp は不要になる(削除。.vcxproj と .filters からも消す)
```
対象: Sample3D の `Ground`(`GroundController` を削除)、`EnemyManager`(出現の高さ)、`Player` の出現位置。

**依存する機能**: 6.3(段階2: インデックス・immutable の頂点バッファ)。6.1(チャンクごとの画面判定。任意)。

**未確定の点**:
- 高さのデータは、画像(PNG)を先にして、CSVは後でよいか
- 高さの精度(8ビット / 16ビット)
- 地形の大きさ・高さ(10000 × 10000、最大高さ 1000、分割 128 でよいか)
- 地形の外側に、落ちないための床(見えない箱)を残すか
- 高さの画像のサンプルを、こちらで作ってよいか(`Assets/Terrain/heightmap.png`)

---

### 6.8 地形に合わせたコライダー

**目的**: 6.7 の地形と**同じ高さのデータ**から、PhysX の当たり判定を作り、物が地形の起伏の上に立てるようにする。

**ファイル案**: 既存の `Engine/RigidbodyComponent.h/.cpp` に `TerrainRigidbodyComponent` を足す。`Engine/Physics.h/.cpp` に PhysX の「クッキング」(`PxCooking`)を足す。

**関数**:
```cpp
// 地形の当たり判定(PhysXの高さフィールド)。いつも Static(動かない)。
// 高さのデータは TerrainRenderer と同じ Terrain を渡す(見た目と当たり判定がずれないように)
// オーナーの Transform は、TerrainRenderer と同じ(地形の左手前の角)
class TerrainRigidbodyComponent : public RigidbodyComponent {
public:
    explicit TerrainRigidbodyComponent(const std::shared_ptr<Terrain>& terrain, bool isTrigger = false);
    void Init() override;
    void Uninit() override;    // アクターを解放した後に、高さフィールドも解放する
};

// Physics に追加(Engine内部用)
namespace physx { class PxCooking; }
physx::PxCooking* Physics::GetCooking();    // 最初に呼ばれた時に作る(PxCreateCooking)
```

**設計のポイント**:
- **PhysX 4.1 の高さフィールド(`PxHeightField`)を使います**(三角形メッシュより軽く、サンプル配列から作れる)。三角形メッシュ(`PxTriangleMesh`)は、洞窟のような「地形の下に空間がある形」には要るが、今回は不要です。
- **作り方**(`ThirdParty/PhysX/include` のヘッダーで確認済みの事実):
  1. `PxHeightFieldSample` の配列を作る。`height` は **`PxI16`(整数)**で、実際の高さ = `height × heightScale`。`maxHeight / 32767` を `heightScale` にして、`round(高さ / heightScale)` を入れる。
  2. `PxHeightFieldDesc` に `nbRows = cellsX + 1`(**行 = X方向**)、`nbColumns = cellsZ + 1`(**列 = Z方向**)、`samples` を渡す。サンプルの並び = `row × nbColumns + column`。
  3. `PxCooking::createHeightField(desc, physics->getPhysicsInsertionCallback())` で `PxHeightField` を作る(`PxCreateCooking` で `PxCooking` を先に作る)。
  4. `PxHeightFieldGeometry(heightField, PxMeshGeometryFlags(), heightScale, rowScale = width / cellsX, columnScale = depth / cellsZ)` を作り、`InitWithGeometry` に渡す(`BodyType::Static`)。
- **クッキングのライブラリは追加不要**: `PhysX.props` に `PhysXCooking_64.lib` が既にある(3章13番)。**`PhysX.props` は変えません**。
- **見た目と当たり判定の対角線をそろえる(落とし穴)**: `PxHeightFieldSample::tessFlag` が立っていると、そのマスの対角線は「そのサンプルから反対側の頂点へ」、立っていなければ「他の2つの頂点を結ぶ向き」(`PxHeightFieldSample.h` の説明)。見た目のメッシュ(6.7)の対角線と同じ向きになるよう、全マスで `tessFlag` を同じにして、6.7 の三角形の分け方をそれに合わせます。**どちらがどちらの向きかは、実装時に確認する**(下の検証を参照)。
- **PhysX の決まり(落とし穴)**:
  - 高さフィールドは **Static(か Kinematic)のアクターにしか付けられません**(Dynamic にはできない)。`TerrainRigidbodyComponent` は常に `BodyType::Static`。
  - アクターの原点は高さフィールドの**角**(0,0)。オーナーの `transform.position` を角の位置にする(中心にしたい時は、シェイプのローカルポーズをずらす。6.2 で作る「形の向き」を渡す口を、位置にも使えるようにする案)。
  - 解放の順番: アクターを `release()` してから、高さフィールドを `release()`(アクターが参照しているため)。`Physics::Uninit` より前に必ず解放する(`Physics::Uninit` が `gPhysics` を解放する)。
  - 地形の**下側から**ぶつかると、そのまま通り抜ける可能性がある(面は上向きの1枚)。保険として、地形の下に薄い見えない床を残す案(7章)。
  - 穴(洞窟の入口)は、サンプルの `materialIndex` に `PxHeightFieldMaterial::eHOLE` を入れると作れるが、今回は作らない。
- **`RigidbodyComponent` との関係**: `InitWithGeometry(geometry)` に `PxHeightFieldGeometry` をそのまま渡せる(`PxCreateStatic` は任意のジオメトリを受け取る)。`userData`(`Collider2D`)も同じなので、**`Physics::Raycast` / `OverlapSphere` / `OverlapBox` が地形にも当たる**(視界判定(6.1)で「地形が壁になる」も、地形に `SetTag("Wall")` すれば作れる)。デバッグ表示(`Draw`)は、高さフィールドには線を描かない(`default` で何も描かない。大きすぎて意味がないため)。
- **Dynamic の箱・カプセルが地形の上に立つ**: 箱は坂の途中で角が引っかかる。**6.2 のカプセルを使う**と、ずっと滑らかに登れる。急な坂(約45度以上)は、`ChaseComponent` や `PlayerController` の速度のコード(`SetVelocity` で水平だけ与える)では登れない・滑り落ちることがあるので、高さの画像は緩やかな起伏にする(7章)。
- **見た目と当たり判定のずれの検証**: `Terrain::GetHeightAt` の値と、地形の上から下へ飛ばした `Physics::Raycast` の `hit.point.y` を、いくつかの場所で比べる(誤差が1単位未満)。この検証用のサンプルを、実装時にScratchで作る。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 変更: `Engine/RigidbodyComponent.h/.cpp`(`TerrainRigidbodyComponent`、必要なら形の位置・向きを渡す口)、`Engine/Physics.h/.cpp`(`GetCooking()` を足し、`Init` / `Uninit` で作成・解放)
- `PhysX.props` は変えない(クッキングは既にリンク済み)。`Shader*.hlsl` は変えない
- Sample2D: 影響なし(既存の物理の動きは変わらない)。Sample3D: 地形を使うまでは何も変わらない。**`Physics::Init` / `Uninit` の変更なので、Sample2D も含めて起動・終了が壊れていないことを確認する**。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D Ground.cpp(6.7と同じ): 見えない大きな箱を、地形の高さフィールドに置き換える
// 今:  ground->AddComponent<BoxRigidbodyComponent>(BodyType::Static, colliderSize);
// 後:  ground->AddComponent<TerrainRigidbodyComponent>(terrain);

// Sample3D: Player・敵が地形の起伏の上を歩く(カプセル 6.2 と組み合わせる)
// Sample3D BulletController: 弾が地形に当たったら消える(今は地面の箱に当たった時の処理)
```
対象: Sample3D の `Ground`(箱コライダー)、弾が地面に当たる処理。

**依存する機能**: 6.7(同じ `Terrain`)。6.2(カプセル。登りやすさのため。必須ではない)。

**未確定の点**:
- 地形の下に、保険の薄い見えない床を残すか
- 地形の外側(高さフィールドの外)の扱い(落ちる / 見えない壁で囲む)
- 地形に `"Wall"` のタグを付けて、視界(6.1)をさえぎる地形にするか

---

### 6.9 水面

**目的**: 波打つ半透明の面を作る。地形のくぼみに水を張る、池・湖・海のように使う。

**ファイル案**: `Engine/WaterRenderer.h/.cpp`(Component。専用の格子メッシュとパイプラインを持つ)、`Shader3DWater.hlsl`(新しいシェーダー)。

**関数**:
```cpp
struct WaterSettings {
    XMFLOAT2 size{ 10000.f, 10000.f };           // 水面の大きさ(X, Z)
    int      cells = 128;                         // 格子の分割数(多いほど波が細かい)
    XMFLOAT4 color{ 0.1f, 0.4f, 0.6f, 0.6f };    // 水の色(wが透明度。0=透明、1=不透明)
    XMFLOAT4 skyColor{ 0.6f, 0.8f, 1.0f, 1.0f };  // 空の色(縁で映り込む色。反射の簡易表現)
    float    waveHeight = 15.f;                   // 波の高さ(山の高さの半分)
    float    waveLength = 400.f;                  // 波の長さ(山から山まで)
    float    waveSpeed = 120.f;                   // 波の進む速さ
    XMFLOAT2 waveDirection{ 1.f, 0.4f };          // 波の進む向き(X, Z)
    float    specularStrength = 1.0f;             // 太陽のきらめき
    float    fresnelPower = 4.f;                  // 縁(浅い角度)でどれだけ空が映るか
};

// 水面を描く Component。オーナーの Transform の位置が水面の中心の高さ(Y が水位)
class WaterRenderer : public Component {
public:
    explicit WaterRenderer(const WaterSettings& settings = WaterSettings());
    WaterSettings& GetSettings();                       // 実行中に手直しできる
    float GetHeightAt(float worldX, float worldZ) const;  // その場所の波の高さ(浮かべる物・泳ぐ判定用。シェーダーと同じ式)
    void Draw() override;
    void Uninit() override;
};
```

**設計のポイント**:
- **波は頂点シェーダーで作る**: 格子の各頂点の高さを `Σ 振幅 × sin(向き・位置 × 周波数 − 時間 × 速さ)`(波を2〜4本重ねる)で動かす。CPUで頂点を毎フレーム書き換えない。**法線は、同じ式の微分(傾き)で解析的に求める**(頂点の法線を使わず、ピクセルシェーダーで計算すると、波がカクカクせず滑らかに見える)。
- **時間を渡す方法**: `Time::GetTime()`(倍率の影響を受ける=ポーズ中は止まる)を、`b5`(`WaterBuffer`)に入れて送る。波の本数分の「向き・振幅・周波数・速さ」も `b5` に入れる(`float4` の配列)。**落とし穴**: シェーダーの `float` で何時間も遊ぶと、`sin` の中の値の精度が落ちて波がガタつく。数十分〜数時間なら問題ないが、気になる時は `fmod` で周期を戻す。
- **`GetHeightAt`(CPU側)**: 浮かべる物・泳ぐ判定用に、**シェーダーと同じ波の式を C++ にも書く**。2つの式がずれると、水に浮かべた物が波と合わなくなるので、式を1か所のコメントに書き、2つとも同じ定数(`b5` に入れる値と同じ配列)から計算する。
- **色と透明度**: 水の `color.w` で透明度。`Mesh` と同じブレンド(`SRC_ALPHA` / `INV_SRC_ALPHA`)で、深度テストはするが**深度は書かない**(水の中の物が消えないように)。両面描く(`CULL_NONE`)。
- **反射の簡易表現**: 本物の反射(反射用の別画面を描く)は大きな機能なので、今回は**フレネル効果**で代用する。見る角度が浅い(水面に近い角度)ほど空の色が映る: `fresnel = pow(1 − saturate(dot(N, V)), fresnelPower)`、`色 = lerp(waterColor, skyColor, fresnel)`。`V`(カメラへの向き)は 6.4 の `b3` の `eyePos` を使う。
- **太陽のきらめき**: 平行光源(`Image::GetLight()` の向き、`b0`)で、ブリン・フォンの鏡面を足す(6.3 と同じ式)。
- **深度との関係(将来)**: 「浅い所は薄く、深い所は濃く」「岸辺に白い泡」は、**水面の下の地形までの距離**が要る。それには、シーンの深度バッファをシェーダーから読む必要があるが、今の深度バッファは `D24_UNORM_S8_UINT`(読めない形)で、読める形に作り直す(`Graphics.cpp` の変更。描画全体に影響する)ことになる。**今回は作らない**(水と地形の交わる線は、くっきり切れて見える)。
- **描く順番(落とし穴)**: 半透明なので、**不透明な物(地形・Player・Sphere)の後**に描く必要がある。`GameObject::SetDrawPriority` を大きくする(例 100)。ただし `Scene::Draw` はカメラの距離順ではないので、水面の中にいる半透明のスプライト(敵の画像)との前後が正しくならないことがある(許容する)。
- **水面の位置**: 波の山が `waveHeight` ぶん上がるので、水位を地形のくぼみの縁より少し下にしておく。大きな格子を `size` で伸ばすので、頂点の間隔が広いと波が荒くなる。`cells` を `size / waveLength × 4` 以上にする(波の1つに格子4個以上)。
- **シェーダー**: `Shader3DWater.hlsl` を新しく作り、`b0` は `Shader3D.hlsl` と同じ形(`MeshConstantBuffer`)にする。`Engine.vcxproj` の `UpToDateCheckInput` と `CopyRuntimeFiles` に足す(3章7番)。`SkinMesh` のように、専用の入力レイアウト・ステートを `WaterRenderer` の `static` 共有物にする(最後の1体で解放)。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 新規: `Engine/WaterRenderer.h/.cpp`、`Shader3DWater.hlsl`(ソリューション直下)、`Engine.vcxproj(.filters)`(`UpToDateCheckInput` と `CopyRuntimeFiles` にも)
- 既存の `Shader.hlsl`・`Shader3D.hlsl`・`PhysX.props` は変えない
- Sample2D: 影響なし(水面は3D専用)。Sample3D: 水面を置くまでは何も変わらない。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D に新しい工場関数: Game/Objects/Water.cpp  CreateWater(Scene& scene, float waterLevel)
GameObject* water = scene.CreateObject("Water");
water->transform.position = { 0.f, waterLevel, 0.f };
water->SetDrawPriority(100);                       // 不透明な物より後に描く
water->AddComponent<WaterRenderer>(waterSettings);

// Sample3D GameScene::Init: 地形のくぼみに水を張る
CreateWater(*this, -20.f);

// Sample3D PlayerController::Update(任意): 水に入ると遅くなる
float waterY = water->GetComponent<WaterRenderer>()->GetHeightAt(pos.x, pos.z);
if (pos.y < waterY) speed *= 0.5f;
```
対象: Sample3D に新しく池を置く(置き換える既存のコードは無いので、**新しい `Water` オブジェクトを追加する**)。

**依存する機能**: 6.3(アルファ・描画順の考え方)、6.4(`eyePos`)、6.7(地形のくぼみに水を置く。無くても平らな床の上に置ける)。

**未確定の点**:
- 水の色・波の大きさの既定値でよいか(遊んで調整)
- 水に入ると遅くなる処理(Player)まで入れるか
- 「深い所は濃い」「岸辺の泡」(深度を読む)を将来やるか

---

## 7. 全体の未確定事項(ユーザーに決めてほしいこと)

1. **了承の取り方**: この設計書の了承を、各機能の着手前の確認(①何を②なぜ③両サンプルへの影響)として扱ってよいか。それとも、機能ごとに改めて確認するか。(決まるまでは機能ごとに確認)
2. **シェーダーの共通化(5.2)**: `Shader3DCommon.hlsli` を作って2つのシェーダーから `#include` するか、同じ関数を2か所に書き写すか。
3. **6.3 段階2(`Mesh` の作り直し)**: 頂点バッファを一度だけ作る・色を定数バッファで渡す・インデックス対応は、地形と影の前提です。6.3 の中でやるか。→ **やった(6.3 に書いた)**
4. **6.1 カメラ判定の既定**: `SetFrustumCulling` を既定ONにするか、OFFにするか。
5. **6.1 見張り役 `Guard` と壁 `Wall`**: Sample3D(と Sample2D)に新しく置いてよいか。壁のタグ名は `"Wall"` でよいか。相手の判定は1点か2点か。
6. **6.2 カプセル**: 2D用のクラス名(`Capsule2DRigidbodyComponent`)でよいか。Player の見た目を立方体からカプセルに変えてよいか(それとも当たり判定だけ)。Sample2D には細長い障害物を新しく置くか。
7. **6.3 透明**: `color.w < 1` の時、`depthWrite = false` と `drawPriority` を自動で変えてよいか。→ **自動では変えない(6.3 に書いた)**
8. **6.4 ライト**: 最大数8でよいか。Sample3D の環境光を暗くしてよいか(見た目が変わる)。敵・弾・爆発にも光を付けるか。
9. **6.5 影**: `Component` に `DrawShadow` を足す方式か、登録簿方式か。解像度と範囲の既定値(2048、3000)。敵(画像)の足元に丸い影を置くか。スポットの影(段階3)まで作るか。
10. **6.6 枠線**: 「光る」にブルームは期待しない前提(リム + 法線拡大)でよいか。敵の画像にも枠線がほしいか。
11. **6.7 地形**: 画像(PNG)を先、CSVを後でよいか。高さの精度(8ビット/16ビット)。大きさ・高さ・分割数の既定値。サンプルの高さ画像を作ってよいか。地形の外側の床を残すか。
12. **6.8 地形コライダー**: 地形の下の保険の床を残すか。地形に `"Wall"` タグを付けるか。
13. **6.9 水面**: 水に入ると遅くなる処理まで入れるか。深度を使った表現を将来やるか。
14. **作る順番・担当者**: 4章の順番でよいか。担当者は誰か。

---

## 8. Engine の変更ファイル一覧とリスク

### 8.1 機能ごとに変える予定の Engine ファイル(すべて、変える前にユーザーの了承が必要)

| 機能 | 新しいファイル | 変える既存ファイル |
|---|---|---|
| 6.1 視界判定 | `Visibility.h/.cpp`、`VisionComponent.h/.cpp` | `MeshRenderer.h`、`ModelRenderer.h/.cpp`、`SpriteRenderer.h/.cpp`、`Model.h/.cpp`、`GameEngine.cpp`、`Image.h/.cpp`(縦横比。任意)、`Engine.vcxproj(.filters)` |
| 6.2 カプセル | (なし) | `RigidbodyComponent.h/.cpp`、`Mesh.h/.cpp` |
| 6.3 マテリアル | `Material.h`、(任意)`Shader3DCommon.hlsli` | `Shader3D.hlsl`、`Shader3DSkin.hlsl`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`、`MeshRenderer.h`、`ModelRenderer.h/.cpp`、`Engine.vcxproj`(`.hlsli` の登録) |
| 6.4 ライト | `LightComponent.h/.cpp`、`Lighting.h/.cpp` | `Shader3D.hlsl`、`Shader3DSkin.hlsl`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`、`GameEngine.cpp`、`Engine.vcxproj(.filters)` |
| 6.5 影 | `Shadow.h/.cpp` | `Shader3D.hlsl`、`Shader3DSkin.hlsl`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`、`Component.h`、`GameObject.h`、`Scene.h/.cpp`、`SceneManager.h/.cpp`、`MeshRenderer.h`、`ModelRenderer.h/.cpp`、`GameEngine.cpp`、`Graphics.h/.cpp`(必要なら)、`Engine.vcxproj(.filters)` |
| 6.6 枠線 | (なし) | `Shader3D.hlsl`、`Shader3DSkin.hlsl`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`、`MeshRenderer.h`、`ModelRenderer.h/.cpp` |
| 6.7 地形 | `Terrain.h/.cpp`、`TerrainRenderer.h/.cpp` | `Engine.vcxproj(.filters)`(6.3の段階2で `Mesh` を作り直し済みの前提) |
| 6.8 地形コライダー | (なし) | `RigidbodyComponent.h/.cpp`、`Physics.h/.cpp` |
| 6.9 水面 | `WaterRenderer.h/.cpp`、`Shader3DWater.hlsl` | `Engine.vcxproj(.filters)`(`UpToDateCheckInput` と `CopyRuntimeFiles`) |

**`Shader.hlsl`(スプライト用)と `PhysX.props` は、9機能のどれでも変えません**(クッキングのライブラリは既にリンク済み)。`Shader3D.hlsl` / `Shader3DSkin.hlsl` は 6.3・6.4・6.5・6.6 で変えます。

### 8.2 設計上の大きなリスク

1. **影(6.5)が一番大きい。** 描画の流れ(`GameEngine::Run`)に影のパスが加わり、レンダーターゲット・ビューポートを切り替える。戻し忘れると画面が真っ黒になる。`Component` / `GameObject` / `Scene` / `SceneManager`(Engineの根幹)にも手が入り、Sample2D を含む全体に影響する。影のバイアス(シャドウアクネ・影の浮き)の調整は、数値の試行錯誤が必要で、GPU・解像度で見え方が変わる。段階分け(平行光源だけ → スキンメッシュ → スポット)で、一歩ずつ進める。
2. **`Mesh::Draw` の作り(6.3 段階2)が、地形・影の前提。** 今は描く度に頂点をコピーする。これを直さずに地形・影を作ると、重くて動かない。6.3 で必ず直す。直す時に、既存の呼び出し(デバッグ表示・`GroundController`・`ModelRenderer`)が壊れないよう、`Draw` の引数は残す。
3. **地形コライダー(6.8)は、見た目(6.7)との「ずれ」が一番の危険。** ①三角形の対角線の向き(`tessFlag`) ②高さが `int16` の丸め ③行(X)・列(Z)の向き、のどれがずれても、物が浮いたり、地面に埋まったりする。`GetHeightAt` と `Physics::Raycast` の高さを比べる検証を必ず行う。地形の下に潜ると落ち続ける危険、Dynamic の箱が坂で引っかかる危険(→カプセル)もある。
4. **シェーダーの変更は、実機で見ないと分からない。** `Shader3D.hlsl` は Sample2D のデバッグ表示も通る。コンパイルエラーは実行時に出力ウィンドウに出る(ビルドは通る)ため、**ビルドが通っても画面が真っ黒になりうる**。必ず起動して確認する。
5. **`#include` 化(5.2)と新しいシェーダーのコピー設定を忘れると、実行時に何も描かれない。** `Engine.vcxproj` の `CopyRuntimeFiles` / `UpToDateCheckInput` への登録を、チェックリストに入れる。
6. **視界判定(6.1)の画面外判定は、半径が小さいと物が消える。** 骨のモデル・アニメーションで動くパーツは余裕を持たせる。`GetCulledCount()` で確認する。
7. **半透明の描画順(6.3・6.9)は、`Scene::Draw` が距離順でないため完全ではない。** 許容するか、`Scene::Draw` を直すか(2Dにも影響)は別の相談。
8. **敵(画像のビルボード)は、ライト・影・枠線に対応しない。** 見た目が揃わない場面が出る(ライトを置いても、敵だけ明るさが変わらない)。許容するか、敵を `ModelRenderer` に変えるか(5.16 の続き)は別の相談。
