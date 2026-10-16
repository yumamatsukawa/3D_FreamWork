# Engine 機能追加ロードマップ2(見え方・地形・物理の追加)

## 1. はじめに

### この文書の目的
Engine(`3D_FreamWork/Engine/`)に追加したい10個の機能(視界判定・カプセルコライダー・マテリアル・ライト・影・枠線・地形・地形コライダー・水面・ポイントライトの影)について、「どんな関数を作るか」「どの順番で作るか」「Engineのどのファイルを変えるか」「Sampleではどう使えるか」をまとめた設計書です。チーム(C++初心者を含む)で分担して実装する時の資料として使います。

### `docs/EngineRoadmap.md` との関係
- `EngineRoadmap.md`(ロードマップ1)の17機能は**すべて完了**しています。この文書はその**続き**です。書き方(目的 / ファイル案 / 関数 / 設計のポイント / 置き換えるSampleのコード / 依存する機能)は同じにそろえました。
- 番号は `EngineRoadmap.md` の「5.x」の続き(5.18〜)ではなく、この文書の中で **「6.1 視界判定」** のように付けます(1〜5章は説明、6章が機能ごとの設計)。
- ロードマップ1の機能(物理の問い合わせ 5.6、カメラ機能 5.7、FBX・骨アニメーション 5.16 など)は**もう使える前提**で書いています。特に、視界判定は 5.6 の `Physics::Raycast` を使います。

### 状態
**ここに書いてある内容は「設計案」です。6.1 視界判定・6.2 カプセルコライダー・6.3 マテリアル・6.4 ポイントライトとスポットライト・6.5 影・6.6 枠線とリム発光が実装済みです(4章の表の状態を見てください。ほかは「未着手」)。** 関数名・引数は、実装しながら変わってよいものです。この設計書は「実際のEngineのコードを読んで」書いたので、既存のクラス名・関数名・ファイル名は本物です。新しく作る名前だけが案です。

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
- **ポイントライトの影は最後**: 6.5 で作った影の仕組み(`Shadow`・`VSShadow`・`DrawDepth`・`ShadowBuffer`)をそのまま使って、6面ぶん描くだけなので、6.5 の後ならいつでも作れます。ほかの機能(枠線・地形・水面)とは独立しています。
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
| 4 | [6.4 ポイントライトとスポットライト](#64-ポイントライト点光源とスポットライト実装済み) | 6.3 | | 実装済み |
| 5 | [6.5 影](#65-影シャドウマップ実装済み) | 6.3(段階2)・6.4 | | 実装済み(段階1・2・3) |
| 6 | [6.6 枠線を光らせる](#66-枠線を光らせるアウトラインリム発光実装済み) | 6.3・6.4 | | 実装済み(リム発光 + 法線拡大の枠線) |
| 7 | [6.7 地形](#67-地形実装済み) | 6.3(段階2)。6.1は任意 | | 実装済み(画像・CSV・描画。遠くに置いた例) |
| 8 | [6.8 地形に合わせたコライダー](#68-地形に合わせたコライダー実装済み) | 6.7 | | 実装済み(高さフィールド。実測で見た目と一致) |
| 9 | [6.9 水面](#69-水面) | 6.3・6.4。6.7があると使い方が分かりやすい | | 未着手 |
| 10 | [6.10 ポイントライトの影](#610-ポイントライトの影) | 6.4・6.5(`Shadow` の仕組みを使う。6.6〜6.9とは独立) | | 未着手 |

---

## 5. 全機能で共通する設計(シェーダーの約束)

マテリアル・ライト・影・枠線・水面は、どれも `Shader3D.hlsl` と `Shader3DSkin.hlsl`(水面は専用のシェーダー)に項目を足します。バラバラに足すとレジスタ番号がぶつかるので、**最初に番号を決めておきます**。

### 5.1 レジスタの割り当て表(案)
| レジスタ | 中身 | 作る機能 | 更新の頻度 |
|---|---|---|---|
| `b0` | `MeshConstantBuffer`(world / view / proj / lightDir / lightColor / ambient)。**今のまま変えない** | (今ある) | 描く度 |
| `b1` | 骨の行列(`Shader3DSkin.hlsl` だけ。今のまま) | (今ある) | 描く度 |
| `b2` | `MaterialBuffer`(鏡面・自発光・縁の光・フラグ) | 6.3 | 描く度 |
| `b3` | `CameraBuffer`(カメラ位置 `eyePos`) | 6.3 | 描く度 |
| `b4` | `LightBuffer`(点光源・スポットライトの配列。**6.4 で `b3` ではなく `b4` にした**。カメラ位置は描く度に変わるが光は1フレームに1回なので、分けた) | 6.4 | 1フレームに1回 |
| `b5` | `ShadowBuffer`(光から見たview×proj・バイアスなど。6.4 が `b4` を使ったので `b5` にずらす) | 6.5 | 1フレームに1回 |
| `t0` / `s0` | 画像 / サンプラー(今のまま) | (今ある) | |
| `t1` / `t2` / `s1` | 太陽の影・スポットの影の深度テクスチャ / 比較サンプラー | 6.5 | |
| `t3` | ポイントライトの影(`TextureCube`。6面の深度)。`s1` を共用。定数は `b5` の末尾に足す | 6.10 | |
| `b6` | `WaterBuffer`(水面だけ。6.4・6.5 で番号がずれたので `b6`) | 6.9 | 1フレームに1回 |

**`b0` を変えない理由**: `MeshConstantBuffer` は `Mesh` と `SkinMesh` と2つのシェーダーが同じ形で使っていて、平行光源はここに入っています。ここを変えると全部を同時に直す必要があります。**平行光源は今のまま `b0`、増える光(点光源・スポット)は新しい `b4`**(設計時は `b3` と書いたが、6.4 で `b4` にした)にすれば、新しい光を1つも置かない限り、今のゲームの見た目は変わりません。

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
- **`Material` の項目**: `color`・`texture`・`lit`・`depthWrite`・`specularColor`・`specularStrength`・`shininess`・`emissiveColor`・`emissiveStrength` の9個。**設計書の `receiveShadow` / `castShadow` / `rim*` / `outline*` は、まだ足していない**(使う機能 6.5・6.6 を作る時に足す。定数バッファ `params` の `y`/`z` を予約してある。**6.5 で `castShadow` / `receiveShadow` を、6.6 で `rim*` / `outline*` を足した**)。
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

### 6.4 ポイントライト(点光源)とスポットライト(実装済み)

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

**実装して決めたこと(実装済み)**:
- **新しいファイル**: `Engine/LightComponent.h/.cpp`、`Engine/Lighting.h/.cpp`(`Engine.vcxproj(.filters)` に登録)。変更: `Shader3DCommon.hlsli`(`LightBuffer` と光のループ。`Shader3D.hlsl` / `Shader3DSkin.hlsl` は共通ファイルを `#include` しているので、中身は変えていない)、`Engine/Mesh.cpp`・`SkinMesh.cpp`(描く時に `Lighting::Bind()` を1行)、`Engine/GameEngine.cpp`(`Lighting::Update()` を `Visibility::BeginFrame()` の次に、`Lighting::Shutdown()` を終了時に)。`Light.h`(平行光源)・`PhysX.props`・`Shader.hlsl` は変えていない。
- **レジスタは `b4`**(設計書は `b3`)。`b3` は 6.3 で `eyePos` だけの `CameraBuffer` として描く度に書いている。光(512バイト+数)を同じバッファに入れると、描く度に書き直すので無駄になる。そこで光は `b4`(1フレームに1回)にし、5.1の表の `ShadowBuffer` を `b5`、`WaterBuffer` を `b6` にずらした。
- **`LightBuffer`**: `int4 lightCounts`(`x`=光の数)+ `GpuLight lights[8]`(`float4` を4本=64バイト。`positionRange` / `colorType` / `directionOuter` / `innerPad`)= 528バイト。C++側は `Lighting::LightConstantBuffer`(`static_assert` でサイズを確認)。`Lighting::Update()` は、光が1つも無くてまだバッファが無い間は何も作らず、`Bind()` は `nullptr` をセットする(シェーダーは0として読むので、光は0個の扱い)。
- **ライトの数が `Lighting::maxLights`(8)を超えた時**: **カメラに近い8個だけ**を使う(3Dカメラ=画面の揺れが入った位置との距離の2乗で並べ、同じ距離なら登録順)。1フレームに1回だけ選ぶ(設計書どおり)。超えた間は、出力ウィンドウに警告が**1回だけ**出る(超え続けても毎フレーム出さず、いったん8個以下に戻ってから、また超えたら出す)。
- **`LightComponent` は Component**(設計書のおすすめどおり)。`Init()` で `Lighting::Register`、`Uninit()` と**デストラクタ**で `Unregister`(`Uninit` が呼ばれずに壊されても、古いポインタが残らない)。集める時に `GetOwner()->GetIsActive()` と `IsEnabled()` を見て、`SetActive(false)`(`ObjectPool` で休んでいる物)・`SetEnabled(false)` の光は使わない。`Scene::Uninit` と、`Destroy` された物の破棄の前に `Uninit()` が呼ばれるので、シーンを切り替えても残らない。
- **光の式**: 色 = `SetColor` × `SetIntensity`。距離の減衰 = `saturate(1 - (d/range)^2)^2`(`d >= range` は0)。スポットは `dot(ピクセルから光への逆向き, 向き)` を `cos(外側)`〜`cos(内側)` で 0〜1 にし、`s*s*(3-2*s)` で滑らかにつなぐ。内側 > 外側 にした時は `SetSpotAngle` が内側を外側に合わせる(0〜89度に収める)。内側=外側でも、割り算が0にならないよう下限を置いている。`dot(N, L)` に掛けて、平行光源・環境光の上に足す。
- **光沢(スペキュラ)はポイント/スポットにも効く**(同じ `specularColor` / `specularStrength` / `shininess`。光の色×強さ×減衰を掛ける)。`Material::lit = false`(unlit)の物は、光も光沢も受けない(スカイボックスなど)。自発光は光の計算の後に足すだけ(今まで通り)。
- **スポットの向き**: `SetDirection` で指定。長さ0なら、オーナーの水平の正面(`Math::Forward3D`)。`rotate.y` しか見ないので、下向きなどにしたい時は `SetDirection` に毎フレーム向きを渡す(Sample3D の `GuardController`)。
- **環境光は暗くしていない**(今の見た目のまま)。光の効果を強く見せたい時は、使う側のシーンで `Image::GetLight().ambient` を暗くする(未確定の点の2つ目は「変えない」とした)。
- **2D(Sample2D)では光は効かない**。スプライトの `Shader.hlsl` は光を計算しない。Sample2D の当たり判定のデバッグ表示(`Mesh`)は光の数が0なので同じ見た目。
- **影の余地(6.5)**: `GpuLight` の `innerPad` の `y`〜`w` と `directionOuter` を予約しておく。6.5 では `LightComponent` に「影を落とすか」を足し、`Lighting` が影を落とす光を選ぶ形にできる。
- **Sample**: Sample3D に `Lantern`(ポイントライトを持つ光る球。原点の周りを回って上下に揺れる。`LanternController`)を置き、`Guard` に懐中電灯(スポット)を付けた(`GuardController` が視線の正面を少し下へ傾けた向きを毎フレーム渡す)。**既存のオブジェクトの見た目は、光が当たる範囲で明るくなる**(環境光は変えていない)。
- **確認(WARP=ソフトウェアのD3D11)**: ①光を置かない絵(地面・立方体・光沢の球・unlit の空・輪郭の線・スキンメッシュ)が、変更前(HEAD)のシェーダーと**1ピクセルも違わない**(262144バイト一致)。②平らな床を上から見て、全ピクセルを式(別に書いたCPUの計算)と比べ、ポイントライト・スポットライト(内側/外側/向き)・色・強さ・複数の光・光沢・自発光・平行光源との足し算が**最大1/255のずれ**で一致。③10個置くと近い8個だけが光る(近い順・超過フラグ・描いた絵が近い8個の式と一致)、ちょうど8個では超過しない。④`SetActive(false)`・`SetEnabled(false)`・`Uninit`・`Uninit` なしの `delete` で光が消え、`Init` を2回呼んでも1個。⑤`SkinMesh`(`Shader3DSkin.hlsl`)でも同じ式で一致。⑥unlit は光があっても同じ絵。
- **WARP で確かめていない物**: `MeshRenderer` / `ModelRenderer` を通した描画、FBXのモデル、実際の `GameScene`(Resources の初期化が必要なため)。Sample3D の見え方(`Lantern` の明るさ・`Guard` の光の輪)は実機で確認してもらう。

**未確定の点(決まったこと)**:
- 最大数8でよいか → **8**(超えたらカメラに近い8個。警告は1回)
- Sample3D の環境光を暗くして光の効果を見せてよいか → **暗くしない**(既存の見た目を変えない。使う側で `Image::GetLight().ambient` を変えられる)
- 敵・弾・爆発にも光を付けるか → **付けていない**(`CreateExplosion` が一瞬光る、などは今後)

---

### 6.5 影(シャドウマップ)(実装済み)

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
    void Bind();          // Mesh/SkinMesh が描く時に、b5 / t1 / s1 をセットする(6.4 が b4 を使ったので b5)
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
- **将来(今回は作らない)**: カスケード(遠近で影の解像度を変える)、点光源の影(キューブマップ。→ **6.10 で設計**)、柔らかい影(PCSS)。

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

// Sample3D Player / Sphere / Cube: 影を落とす・受ける(実装では Material::castShadow / receiveShadow は既定 false。使う物に true を書く)
// Sample3D GroundController のタイル: Mesh::Draw に receiveShadow = true のマテリアルを渡して影を受ける
// Skybox: 何も書かない(castShadow / receiveShadow が既定の false のまま)
// (敵は画像のビルボードなので影を落とさない。丸い影の案は7章で確認)
```
対象: Sample3D の Player・Sphere・Cube・ModelDisplay・Ground・Skybox。

**依存する機能**: 6.3(段階2)、6.4(光の向き・位置)。

**実装して決めたこと(実装済み。段階1・2・3とも)**:
- **方式は「登録簿方式」にした**(`Component::DrawShadow` を足す案は選ばなかった)。`MeshRenderer` / `ModelRenderer` が `Init` で `Shadow::Register` に自分を登録し(`Uninit` とデストラクタで解除)、`Shadow::Render` が登録簿を回して、アクティブ(`SetActive`)で有効(`SetEnabled`)な物の `DrawShadow` を呼ぶ。選んだ理由: ①`Component.h` / `GameObject.h` / `Scene.h/.cpp` / `SceneManager.h/.cpp` の根幹を1行も触らずに済み、Sample2D を含む全オブジェクトへの影響が無い(`Lighting` と同じ作りで、チームも読みやすい) ②アクティブ・有効の判定は `GetOwner()->GetIsActive()` と `IsEnabled()` の2行で済み、`GameObject::Draw` と同じ決まりになる ③`DrawShadow` を実装した物だけが影のパスに出るので、`Component` を使う全部のクラスに口を増やさない。デメリット(登録の消し忘れ)は、`Uninit` とデストラクタの両方で外すことで防いだ。
- **新しいファイル**: `Engine/Shadow.h/.cpp`(`Engine.vcxproj(.filters)` に登録)。**変えた Engine**: `Shader3DCommon.hlsli`(`ShadowBuffer` b5・`SunShadow` / `SpotShadow` / PCF。`ShadePixel` で引く)、`Shader3D.hlsl`・`Shader3DSkin.hlsl`(深度だけの頂点シェーダー `VSShadow` を足しただけ)、`Mesh.h/.cpp`・`SkinMesh.h/.cpp`(`DrawDepth` と深度用の頂点シェーダー。`Draw` が `Shadow::Bind()` を1行)、`MeshRenderer.h`・`ModelRenderer.h/.cpp`(登録と `DrawShadow`)、`Material.h`(`castShadow` / `receiveShadow`)、`GameEngine.cpp`(`Shadow::Render()` と `Shadow::Shutdown()`)。**変えていない**: `Component.h`・`GameObject.h`・`Scene.h/.cpp`・`SceneManager.h/.cpp`・`Graphics.h/.cpp`(描画先の保存と復元を `Shadow::Render` の中で `OMGet○○` / `RSGet○○` でやったので不要)・`Shader.hlsl`・`PhysX.props`。段階3のために、6.4 で「影の余地」として予約していた範囲で `LightComponent.h`(`SetCastShadow`)と `Lighting.h/.cpp`(影を落とすスポットを1つ選び、`GpuLight::innerPad[1]` に印を付ける。`GetShadowSpotLight()`)も変えた。
- **`Material` の `castShadow` / `receiveShadow` は、既定を両方 `false` にした**(設計書は `true`)。「影を有効にしない限り既存の見た目を変えない」ことに加えて、影を有効にしても、指定した物だけが影の影響を受けるようにするため。`RigidbodyComponent` の枠線表示や、色指定の `Mesh::Draw` も、既定のマテリアル(=影を受けない)になる。
- **既定値(`Shadow::Settings`)**: `enabled = false`、`resolution = 2048`、`distance = 3000`、`depthBias = 0.0005`(設計書は0.0015)、`normalBias = 2.0`、`strength = 0.6`、`pcfRadius = 1`、スポット用に `spotResolution = 1024`・`spotDepthBias = 3.0`(ワールド単位)・`spotStrength = 1.0`。`depthBias` を設計書より小さくしたのは、影の奥行きの範囲が `2 × distance`(既定6000)で、0.0005 がワールド単位で約3に当たり、0.0015(約9)だと影が物から浮いて見えるため。ラスタライザの傾き補正(`SlopeScaledDepthBias = 1.5`)と合わせて、WARP で床・箱・球・スキンの自己影が出ないことを確認した。Sample3D は `distance = 2500` にしている。
- **影の範囲**: 3Dカメラの前方 `distance / 2` の点を中心に、一辺 `distance` の正方形(光に垂直な面)×奥行き `2 × distance`(中心の前後に `distance` ずつ)の箱。中心の縦横は、光から見て1テクセルの大きさの整数倍に丸める(カメラが動いても縁がちらつかない)。範囲の縁(4%)は影を弱くして、影が急に消えて見えないようにした。範囲の外・奥行きの外は影なし(明るい)。カメラが高い所から真下を見るような時は、床が奥行きの範囲(中心から光の向きに `distance` まで)の外になって影が出ないので、`distance` を大きくする。
- **描画の流れ**: `GameEngine::Run` で `Lighting::Update()` の次、`Text::BeginDraw()` の前に `Shadow::Render()`。影が無効の時は何もしない(コスト0。深度テクスチャも作らない)。有効の時は、描画先・深度・ビューポート・ラスタライザ・ブレンド・深度ステンシル・ピクセルシェーダーを覚えて、影のパスの後に**必ず元へ戻す**。影のテクスチャを読むバインド(`t1`・`t2`)は、書く前に外す(同じ物を同時に読み書きしない)。
- **レジスタ**: `b5` = `ShadowBuffer`(平行光源の行列とパラメーター + スポットの行列とパラメーター。`Shadow::ShadowConstantBuffer`)、`t1` = 平行光源の影、`t2` = スポットライトの影、`s1` = 比較サンプラー。影を使わないフレームも、`s1`(比較サンプラー)は常にセットする(セットしないと、デバッグレイヤーが警告する)。`b5` と `t1`・`t2` には `nullptr` をセットするので、シェーダーは0として読み、影なしになる。
- **影が効くのは平行光源と、影を落とすスポット1つだけ**。環境光・ポイントライト・影を落とさないスポットは、影で暗くならない。unlit(`lit = false`)の物は影を受けない。平行光源の影は「平行光源の光(拡散+光沢)」だけを `strength` の分だけ暗くする。
- **段階2(スキンメッシュ)**: `Shader3DSkin.hlsl` の `VSShadow` が、骨で曲げた後の位置を深度に書く。`ModelRenderer::DrawShadow` が、描く時と同じ行列・骨の行列で `SkinMesh::DrawDepth` を呼ぶ。骨の動きが影に反映される。
- **段階3(スポットの影)**: 作った(影を落とせるスポットは同時に1つ)。`LightComponent::SetCastShadow(true)` にしたスポットのうち、使われる光の中で3Dカメラに一番近い1つを `Lighting` が選び、`Shadow::Render` がその光から透視投影(視野 = 外側の角度の2倍、最大120度。奥行きは `range` まで)で深度を描く。光から遠いほど深度が粗いので、`spotDepthBias`(ワールド単位)を、その場所の深度の傾きで換算して引く。外側の角度が60度を超える部分には影が出ない。ポイントライトの影は作れない(`SetCastShadow` を呼んでも無視する)。
- **透明部分(画像のアルファ)**: 影のパスにはピクセルシェーダーが無いので、画像の透明な部分も含めて、板全体が影になる(設計書の段階2にあった「アルファの切り抜きの影」は、今回の範囲に入れていない)。半透明の物(`GlassPanel` など)は `castShadow` を `false` のままにする。
- **敵(スプライト)の丸い影(ブロブシャドウ)は置いていない**(今後。敵は `SpriteRenderer` なので、影を落とさない)。
- **Sample**: Sample3D に `Sun`(`Objects/Sun.cpp`。`ShadowManager` が、このシーンの間だけ影を有効にして `distance = 2500` にし、`Uninit` で既定に戻す)を置いた。地面のタイル(`GroundController` が `Material::receiveShadow = true`)が影を受け、`Player`・`Wall`・`ModelDisplay`・`SkinDisplay` が影を落とし・受け、`Guard` は影を受けるだけ(懐中電灯の光源が自分の立方体の内側にあり、落とすと自分の面が光をすべてさえぎるため)。`Guard` の懐中電灯は `SetCastShadow(true)`(壁や `Player` の後ろに光が届かない)。**影を有効にした分、Sample3D の地面に影が落ち、懐中電灯の光が壁・Playerでさえぎられる(ここは見た目が変わる)。** `GlassPanel`・`Lantern`・`Skybox`・敵・弾は影の対象にしていない。Sample2D は影を使わない(見た目は変わらない)。
- **確認(WARP=ソフトウェアのD3D11。デバッグレイヤー有効)**: ①影を使わない絵(地面・立方体・光沢の球・unlit の空・輪郭の線・スキン)が、変更前(6.4)の絵と**1ピクセルも違わない**(`Shadow::Render` を呼んでも、影を有効にしても `castShadow` / `receiveShadow` を指定しなければ同じ) ②`Shader3D.hlsl` / `Shader3DSkin.hlsl` の `VS`・`PS`・`VSShadow` が警告なしでコンパイルできる ③上から見た床に落ちる回転した箱の影の位置と形が、光線と三角形の交差(CPUの別の計算)と一致(影の中・外とも不一致0)。影の無い所は影なしの絵と同じ ④床の自己影・球・箱の自己影が出ない(光の高さ13〜87度、斜めのカメラ。最大差は2/255以内) ⑤PCF の半径を大きくすると縁が柔らかい(中間の明るさの画素が増える) ⑥スキンメッシュの影が、骨で曲げた後の形のCPUの計算と一致。骨を変えると影が変わる ⑦`distance` を小さくして、範囲の外の箱は影を落とさず、範囲の中の箱は落とす ⑧`receiveShadow = false`・unlit・`strength = 0` で影が出ない ⑨解像度を1024・4096に変えても正しい ⑩`SetActive(false)`・`SetEnabled(false)`・`Uninit`・`delete` で影が消え、登録が残らない ⑪影のパスの後に、描画先・深度・ビューポート・ラスタライザ・ブレンド・深度ステンシルが元に戻る ⑫点光源だけで照らした絵は、影の有無で変わらない ⑬スポットの影の位置・形が、光と床を結ぶ線と箱の交差(CPU)と一致。平行光源の影とスポットの影が、それぞれの光だけを暗くする(足し算が一致)。スポットの自己影(アクネ)が、光の高さ60〜900・いろいろな向きで出ない。複数ある時はカメラに近い1つを選び、ポイントライトは無視 ⑭デバッグレイヤーに警告・エラーが出ない(読み書きのバインド競合・比較サンプラーの不足なし)。終了時に、影のテクスチャ・ステート・バッファが解放され、生存オブジェクトが残らない。
- **WARP で確かめていない物**: `MeshRenderer` / `ModelRenderer` を通した影(`Resources` の初期化が必要なため。同じ登録・`DrawDepth` の部分は、同じ作りのテスト用 Component で確認した)、FBXモデルの影、実際の `GameScene`、実機GPUでの見た目(バイアスは GPU・解像度で見え方が変わる)。Sample3D の見え方(影の濃さ・縁・懐中電灯の影)は、実機で確認してもらう。

**未確定の点(決まったこと)**:
- 影の範囲(`distance`)と解像度の既定値でよいか(2048、3000) → **そのまま**(Sample3D は `distance = 2500`)。`depthBias` だけ 0.0005 に変えた
- `DrawShadow` を `Component` に足す方式にするか、登録簿の方式にするか → **登録簿方式**(根幹のファイルは触っていない)
- 敵(画像)の足元に丸い影を置くか → **置いていない**(今後)
- スポットライトの影(段階3)まで作るか → **作った**(同時に影を落とせるスポットは1つ)

---

### 6.6 枠線を光らせる(アウトライン・リム発光)(実装済み)

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
- B(ステンシル方式。物を少し大きくして印の無い所だけ描く)は作らない。ただし、Aの「本物の内側に枠線が漏れない」ための印として、ステンシルは使っている(下の「実装して決めたこと」)。

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

**実装して決めたこと(実装済み。段階1・2とも)**:
- **方式**: 設計書のおすすめどおり、**C. リムライト(段階1)+ A. 法線拡大の2パス(段階2)の組み合わせ**。B(ステンシル方式の枠線)は作っていない(A の中で、本物の内側に描かないための印にステンシルを使う)。ブルームは作っていない(枠線はくっきりした線で、画面の外へにじまない)。新しいファイルは無い。
- **変えたEngine**: `Shader3DCommon.hlsli`(`MaterialBuffer` に `matRim` / `matOutline` / `matOutlineParams`、`ShadePixel` の最後にリム、`OutlineClipPos()` / `ShadeOutline()`)、`Shader3D.hlsl` / `Shader3DSkin.hlsl`(`VSOutline` / `PSOutline`)、`Material.h`、`Mesh.h/.cpp`・`SkinMesh.h/.cpp`(`DrawOutline`、平均した法線のバッファ)、`MeshRenderer.h`・`ModelRenderer.h/.cpp`(`SetRimLight` / `SetOutline`、描いた直後に枠線)。**`Shader.hlsl`・`PhysX.props`・`Component.h`・`Scene` などは変えていない。**
- **`Material` の項目**: `rimColor`(白)・`rimStrength`(0)・`rimPower`(3)・`outlineColor`(`{1,1,0,1}`。**aは濃さ**)・`outlineWidth`(0)の5つ。**既定はどちらもオフ**。`MaterialConstantBuffer`(`b2`)は `float4` 4本 → 7本(112バイト)で、`rim`(rgb=色, w=強さ)・`outline`(rgb=色, w=濃さ)・`outlineParams`(x=太さ, y=画面の幅, z=画面の高さ)を足した。`params.y` を `rimPower` に使った。C++側に `static_assert(sizeof == 16 * 7)`、HLSL の `MaterialBuffer` と並びを合わせた。
- **太さの単位は画面のピクセル**(ワールド単位ではない)。頂点シェーダーで、法線が画面上で動く向き(`d(clip.xy/clip.w)/dt`)へ `太さ × 2 / 画面の大きさ × w` だけクリップ空間でずらすので、遠くても近くても同じピクセル数になる。画面の大きさは `Graphics::width` / `Graphics::height`。
- **枠線は「裏向きの面だけ」をピクセルシェーダーで選ぶ**(設計書の `D3D11_CULL_FRONT` のラスタライザは作っていない)。理由: このエンジンはカリングなしで描いていて、メッシュによってポリゴンの巻き方向が揃っていない(立方体・球・OBJで違う)ので、カリングだと表裏を間違える。代わりに `ShadeOutline()` が、元の法線とカメラへの向きの内積が正なら `discard` する(巻き方向に関係なく動く)。深度は書く(本物の裏側の深さを、`太さ(ピクセル)× 1ピクセルのワールドの長さ × 3` だけカメラ側へずらした値)。**最初は奥へずらしていたが、地面に立つ物の接地部で、膨らんだ殻が地面と同じ高さか下に入り、地面に隠されて枠線が切れた(実機で発見)ため、手前へずらす形に直した。** 手前へずらすと、縁のそばで本物の内側へ漏れるので、**ステンシルで本物の見えている画素に枠線を描かない**(`DrawOutline` が 印→枠線→印を消す の3回描く。`D24_UNORM_S8_UINT` のステンシルを使い、毎フレームのクリアはせず、描いた後に0へ戻す)。手前の壁が、ずらした量より近いと、枠線が少し突き抜ける。
- **平均した法線**: 三角形の角度で重みを付けて、同じ位置(1/10000に丸めて比較)の頂点の法線を平均する(`Mesh::CreateSmoothNormals`。`SkinMesh` も使う)。**`Mesh` / `SkinMesh` を作る時に1回だけ、別の頂点バッファ(slot 1。1頂点12バイト)として作る**(`D3D11_USAGE_IMMUTABLE`)。三角形のメッシュだけ(線のメッシュは枠線なし)。入力レイアウトは枠線専用に別に作り、普通の描画のレイアウトは変えていない。
- **描画ステートの復元**: `DrawOutline` は「ステンシルで本物の見えている画素に印を付ける → 印の無い画素にだけ枠線を描く → 印を消す」の3回描きで、終わりに深度ステンシルを普通の状態へ、slot 1 の頂点バッファを外してラスタライザを既定(`nullptr`)に戻す(`Draw` と同じ決まり)。枠線の深度は、本物の裏側の深さを**カメラ側へ**ずらした値(奥ではない)。
- **壁越しの枠線(seeThrough。後から追加)**: `SetOutline(color, width, seeThrough = false)` / `Material::outlineSeeThrough`。`true` の時、`DrawOutline` は①深度テストなしで、本物のシルエット全体のステンシルを1にする ②深度テストも深度の書き込みもなしで、ステンシルが1でない画素(=シルエットの外側の輪)にだけ枠線を描く ③ステンシルを0に戻す。`false`(既定)の経路・描画回数は変えていない(ピクセル一致を確認)。壁など**後から描かれる物が枠線を上書きする**ので、使う物は `SetDrawPriority` を壁より大きくする。隠れている間だけ色・濃さを変える機能は作っていない(拡張案: `outlineHiddenColor` を足し、本物の深度テストを通る画素だけ通常の色、通らない画素は別の色にする=ピクセルシェーダーで深度を読むか、2回描き分ける)。Sample3D は `Guard`(`SetDrawPriority(5)`、見つけていない間は細い青、見つけた時は太い赤の壁越しの枠線)。
- **透明部分のある画像**: アルファで抜く画像のメッシュは、透明部分でも深度とステンシルが書かれる(本物の板の形で書かれる)ので、透明部分には枠線が出ない(抜き絵の輪郭に沿った枠線は付かない)。
- **影・光との関係**: 枠線のパスは光の計算も影の計算も通らない(`PSOutline` は枠線の色を返すだけ)ので、影を落とさず・受けず、光で色が変わらない。影のパス(`DrawDepth`)は枠線を描かない。
- **半透明・スカイボックス(`depthWrite = false`)**: `MeshRenderer` / `ModelRenderer` は枠線を**描かない**(本物が深度に書かれないと、膨らませた殻が内側まで枠線の色になるため)。
- **リム**: `pow(1 - |dot(N, V)|, rimPower) × rimStrength` を `rimColor` に掛けて最後に足す(裏向きの面でも真っ白にならないよう、内積は絶対値)。`lit = false` の物にも付く。`rimStrength = 0` の物は計算を飛ばす。
- **`MeshRenderer` / `ModelRenderer` の追加関数**: `SetRimLight(color, strength, power = 3)` / `SetOutline(color, widthPixels)`(`Material` の項目を書き換えるだけ)。
- **Sample**: Sample3D の `Player` に `SetRimLight`(青いリム)、`Guard` が相手を見つけたら赤い枠線(`GuardController`)、新しい `HighlightController`(`Components/`)を `ModelDisplay`(黄色)と `SkinDisplay`(緑。骨で曲げたモデル)に付けて、Playerが半径700以内に近づくと枠線を光らせる(`Physics::OverlapSphere`)。`Sphere` の例(設計書)は、シーンで未使用のため使っていない。**見た目が変わる所: `Player` の縁が少し青くなる。Guard が相手を見つけると赤い枠線が付く。モデルに近づくと枠線が光る。**
- **確認(WARP=ソフトウェアのD3D11。デバッグレイヤー有効)**: ①枠線・リムを指定しない絵(地面・立方体・光沢の球・unlit の空・半透明・輪郭の線・スキン・ポイントライト)が、変更前と**1ピクセルも違わない**(262144バイト一致) ②`VSOutline` / `PSOutline` を含むシェーダー10本が警告なしでコンパイルできる ③球の枠線: 太さ2・4・8ピクセルで、本体の内側は1画素も変わらず、外側にだけ、すき間なく、指定の太さ(本体からの距離が太さ+0.5以内)で出る。真ん中の行の太さが指定どおり。カメラを z=-400〜-3000 に離しても同じ太さ ④色・濃さ(a)を変えると変わる(赤・半透明は背景と半分混ざる。a=0・太さ0では何も描かない) ⑤立方体(回転・斜め)でも枠線が割れない(辺は約7割の太さまで) ⑥リムの値が、球に対するCPUの式と最大1/255で一致(強さ・powerを変えて4通り)。縁だけが光り、中心は変わらない。真正面を向いた面の真ん中は光らない ⑦スキンメッシュ(骨で曲げた球)でも、枠線が太さどおりに出て、骨を変えると枠線も動く。リムも光る ⑧手前の板に枠線は隠され、後ろの板の上には出る(描く順番を入れ替えても同じ絵) ⑨平行光源・点光源・影を落とすスポット・影のある絵の中でも、枠線の画素は枠線の色のまま。枠線の有無で、枠線以外の画素は変わらない ⑩枠線を本体より先に描いても、本体の内側は変わらない ⑪地面(平らな板)に立つ箱の接地部の枠線が地面の上に出る(直す前は0画素で再現)・本体の内側は1画素も変わらない・壁(間60・100)の向こうの枠線は壁を突き抜けない・スキンでも同じ ⑫デバッグレイヤーに警告・エラーが出ない。
- **WARP で確かめていない物**: `MeshRenderer` / `ModelRenderer` を通した描画(`Resources` の初期化が必要なため。同じ `DrawOutline` を直接呼んで確認した)、FBXモデル、実際の `GameScene`、実機GPUでの見た目。Sample3D の見え方(枠線の太さ・リムの強さ・近づいた時の切り替わり)は、実機で確認してもらう。

**未確定の点(決まったこと)**:
- 「光る」を、ブルーム(画面全体をにじませる)まで期待するか → **期待しない前提で、リム + 法線拡大の枠線を作った**(ブルームは作っていない)
- 敵(画像)にも枠線を付けたいか → **付けていない**(`SpriteRenderer` は `Shader.hlsl` で描くため、枠線は効かない。必要なら画像用の別の仕組みを相談)

---

### 6.7 地形(実装済み)

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

**実装して決めたこと(実装済み)**:
- **ファイル**: 新規は `Engine/Terrain.h/.cpp`・`Engine/TerrainRenderer.h/.cpp`(`Engine.vcxproj(.filters)` に登録)と、サンプルの高さ画像 `Assets/Terrain/heightmap.png`(129x129、16ビットのグレースケールPNG。なだらかな丘と谷。PowerShellで数式から自動生成)。Sample3D は `Objects/Hills.h/.cpp`(`CreateHills`)を足して `GameScene` に1行置いた。**既存のEngineファイル・`Shader*.hlsl`・`PhysX.props` は1つも変えていない**(`Mesh::InitIndexed` など、6.3 で作った物をそのまま使った)。
- **高さの入力(PNG先・CSVも作った)**: `Terrain::LoadFromImage`(WIC。`SpriteTransition` と同じ `IWICImagingFactory`。`Texture::Load` は GPU 用でCPUに画素が残らないため、自前でWICから読む。`CoInitializeEx` は自分で呼び、戻す)。**16ビットのグレースケール(`GUID_WICPixelFormat16bppGray`)は65536段階でそのまま**、それ以外(8ビットのグレースケール・カラー・パレット)は `GUID_WICPixelFormat32bppRGBA` に変換して**赤(R)の256段階**。`Terrain::LoadFromCsv`(`Csv` クラスを使用。1行目 `x0,x1,x2,...`、1マス1つの数値0〜1)。プログラムで作る `Terrain::CreateFromSamples`(確認用にも使う)も足した。
- **画像の向き**: 画像の左が X=0、**画像の上が Z=最大**(上から見た地図と同じ。CSV は最初のデータ行が Z=最大)。頂点 (x, z) は `u = x / cellsX`、`v = 1 - z / cellsZ` で画像を**双線形補間**して読むので、画像の大きさと `cellsX/cellsZ` が違っても使える(画像が129で `cells` が128なら、頂点がちょうど画素の位置になる)。8ビットの階段は、補間と中心差分の法線でなめらかになる(隣との平均は、コライダー(6.8)とデータがずれるので足していない)。
- **高さの並び**: `GetHeights()[x * GetVertexCountZ() + z]`(行=X、列=Z。PhysX の高さフィールドと同じ並び)。**maxHeight を掛けた後のワールドの高さ**を持つ。6.8 はこの配列から `PxHeightField` を作る(`GetCellSizeX/Z()` で行・列のスケールも取れる)。
- **既定値**: 設計書のまま(10000 x 10000、最大高さ1000、分割128、`uvRepeat` 20)。分割数は 1〜256 に丸め(範囲外は出力ウィンドウに知らせる)、`width/depth` が0以下なら10000にする。256x256 マス=頂点66049個・インデックス393216個(32ビット)でも、CPU側の計算は約1〜3ミリ秒。描画は1回の `DrawIndexed`(IMMUTABLE を1回だけ作る)。
- **三角形の対角線(6.8 とそろえる)**: 全マスで **`(x, z) → (x+1, z+1)`**。`PxHeightFieldSample::tessFlag` のヘッダー説明は「立てると、そのサンプルから反対側の頂点へ対角線が走る」なので、**6.8 では全サンプルに `setTessFlag()` を立てる想定**(向きの最終確認は 6.8 で、`GetHeightAt` と `Physics::Raycast` の高さを比べて行う)。三角形は `(a, d, c)` と `(a, c, b)`(`a=(x,z)`、`b=(x+1,z)`、`c=(x+1,z+1)`、`d=(x,z+1)`。上から見て時計回り)。`GetHeightAt` は同じ三角形の上で補間する。
- **`GetHeightAt` / `GetNormalAt` / `Contains`**: 範囲外の座標は、一番近い端に丸めて返す(`Contains` で範囲内かを調べられる)。`GetNormalAt` は**三角形1枚ごとの面の向き**(コライダーと同じ向きで、坂道の判定に使える)。描画の法線は、隣の頂点との高さの差(中心差分。端は片側)でなめらかにしてあるので、少し違う。`TerrainRenderer` は同じ名前の関数でワールド座標を受ける(オーナーのワールド行列の位置を足し引きする)。
- **`TerrainRenderer`**: `Material` を持ち(最初の画像は `TerrainSettings::texture`)、`MeshRenderer` と同じ `Mesh::Draw`・同じ `Material`・同じライト(`Image::GetLight()`、ポイントライト、スポット)で描く。**影**: `receiveShadow = true` で受け、`castShadow = true` で落とす(`Shadow::Caster` に登録。既定は `Material` と同じ両方 `false`)。**枠線は付けない**(地形全体の枠線に意味が無い)。リムは `Material` で効く。**画面外の判定**: `SetFrustumCulling(true)` で、地形の箱を囲む球を `Visibility::IsSphereInView3D` で調べて省く(**既定は false**。1枚の大きな地形はたいてい映っているので。チャンク分けはしていない=設計書どおり段階2)。
- **Transform**: 位置(と親)だけを使う。拡大縮小・回転は使えない(使うと `GetHeightAt` とずれる)。
- **Sample3D は置き換えず、足した**: 設計書は「地形を使えば `GroundController` は不要」と書いているが、**今は残した**。理由: 地形コライダー(6.8)がまだ無く、置き換えると見た目と当たり判定(PhysX の地面)が変わるため。`CreateHills(*this, { 1500, -50, -2000 })` で、**4000 x 4000、最大高さ600の地形**を、Playerの東の方(X 1500〜5500、Z -2000〜2000)に置いた(丘の上を歩くと、まだ当たり判定が無いので突き抜ける)。地面と同じ画像(`bg_tile.png`)と色の掛け算、`receiveShadow = true`。底(Y=-50)は地面の上面と同じ高さ。画像の最低の高さが0より上(約22)なので、地面のタイルと同じ面でちらつかない。**既存のオブジェクト・Component・見た目は変えていない。** 6.8 の後で、`CreateGround` を `TerrainRenderer` + 地形コライダーに置き換え、`GroundController` を消し、`EnemyManager` の出現の高さを `GetHeightAt` にする。
- **地形の外の床**: 今の見えない大きな床を**そのまま残す**(地形を足しただけなので)。6.8 で置き換える時に、地形の外にも床を残すかを決める。
- **確認(WARP=ソフトウェアのD3D11。`Engine.lib` をリンクした確認用プログラム。プロジェクトには入れていない)**: ①サンプル画像から作った129x129の全頂点の高さが、元の画素(16ビット)と完全に一致 ②頂点の位置・UV・法線(中心差分を別に計算)・長さ1が全頂点で一致、インデックスが範囲内、全マスの2枚の三角形が `a-c` の対角線を共有、上から見て時計回り ③ランダムな50000点で `GetHeightAt` が「メッシュの三角形の上の補間」と最大0.0001、`GetNormalAt` が「メッシュの面の法線」と最大0.0006で一致。頂点の上では頂点の高さと一致。各マスの中心が `a-c` の中点に乗る ④範囲外は端の高さ・`Contains` ⑤8ビットの横・縦のグラデーション(分割数が画像と違う)で高さが直線的で、向きが仕様どおり(縦は画像の上が Z=最大)。カラーPNGのRだけを使う。存在しないファイル・壊れたファイル・列 `x0` の無いCSVは nullptr ⑥CSV の向き・0〜1への丸め ⑦分割数の丸め(1000→256、0→1)・不正な大きさ ⑧256x256 の頂点・インデックス数 ⑨WARP描画: 光に向いた斜面が明るく背の斜面が暗い(N・L の期待値と近い)、光の向きを変えると入れ替わる、`Material` の色(赤)・`lit = false` が効く ⑩**1マスの平らな地形と、同じ三角形・同じ法線・同じUVの `MeshRenderer` の板が、1ピクセルも違わない**(同じ描画経路) ⑪影: 上に置いた立方体の下で、`receiveShadow = true` の地形が暗くなり(輝度153対255)、`false` では変わらない。地形の `castShadow = true` を足しても影が壊れない ⑫`SetFrustumCulling(true)`: 映っている時は描き、カメラの後ろでは省いて `GetCulledCount() == 1`。⑬オーナーを動かしても `GetHeightAt` / `GetNormalAt` / `Contains` がワールドで正しい。既存のEngineファイルは編集しておらず(`Engine.vcxproj(.filters)` に新しい4ファイルを足しただけ)、`GameScene` に `CreateHills` を1行足しただけなので、既存の描画は変わりようがない。
- **WARP で確かめていない物**: `Resources` を通した画像の貼り付け(確認用プログラムは `Resources` を初期化しない)、実際の `GameScene` 上の見え方、実機GPUの見た目、Visual Studio のデバッグレイヤー。Sample3D の見え方(丘の形・色・影・遠くから見た時の大きさ)は、実機で確認してもらう。

**未確定の点(決まったこと)**:
- 高さのデータは、画像(PNG)を先にして、CSVは後でよいか → **画像を先に、CSVも同時に作った**
- 高さの精度(8ビット / 16ビット) → **両方対応**(16ビットのグレースケールPNGなら65536段階)
- 地形の大きさ・高さ(10000 × 10000、最大高さ 1000、分割 128 でよいか) → **既定値はそのまま**(Sample3D の例は 4000 x 4000、最大高さ600、分割128)
- 地形の外側に、落ちないための床(見えない箱)を残すか → **残した**(今回は置き換えていないので、今の床のまま。6.8 で決める)
- 高さの画像のサンプルを、こちらで作ってよいか → **作った**(`Assets/Terrain/heightmap.png`)
---

### 6.8 地形に合わせたコライダー(実装済み)

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

**実装して決めたこと(実装済み)**:
- **ファイル**: Engine は `RigidbodyComponent.h/.cpp`(`TerrainRigidbodyComponent` を追加)と `Physics.h/.cpp`(`GetCooking()` を追加。`Uninit()` で解放)の4ファイルだけ。新しいファイルは無し。`PhysX.props` は変えていない(`PhysXCooking_64.lib` が元から入っていた)。`Shader*.hlsl` も変えていない。Sample3D は `Objects/Hills.cpp` に `TerrainRigidbodyComponent` を1行足し、`Hills.h` と `GameScene.cpp` のコメントを直しただけ。
- **作り方**: `GetHeights()` の最大値 ÷ 32767 を `heightScale` にして、`lroundf(高さ ÷ heightScale)` を `PxI16` に入れる(平らな地形で0にならないよう、下限0.001)。`PxHeightFieldDesc`: `nbRows = cellsX+1`(行=X)、`nbColumns = cellsZ+1`(列=Z)、並びは `Terrain::GetHeights()` の `x*vertexCountZ+z` とそのまま同じ。`PxHeightFieldGeometry(heightField, PxMeshGeometryFlags(), heightScale, GetCellSizeX(), GetCellSizeZ())`。材質0、`materialIndex1` も0、穴(`eHOLE`)は使わない。`PxCooking` は `GetCooking()` が最初に呼ばれた時に `PxCreateCooking` で作る(Sample2D は呼ばないので何も変わらない)。
- **三角形の向き(実測で確認)**: **全サンプルに `tessFlag` を立てる(`PxBitAndByte(0, true)`)と、6.7 の対角線 `(x,z)→(x+1,z+1)` とそのまま一致した。`Terrain` は変えていない**。行=X・列=Z の向きも、`PxHeightFieldGeometry` の `rowScale = X方向のマスの長さ`・`columnScale = Z方向` で一致した。
- **原点**: 角(オーナーの位置が地形の左手前の角で、Yが高さ0)。`TerrainRenderer` と同じで、ずらす計算は無い。中心に置きたい時は `position = { -width/2, 高さ, -depth/2 }`(`Ground.cpp` 案どおり)。
- **許容誤差と実測**(WARP不要・GPUなし。`Engine.lib` をリンクした確認用プログラム。プロジェクトには入れていない): 地形の真上から真下へ `Physics::Raycast` を撃ち、`hit.point.y` と `オーナーY + Terrain::GetHeightAt` を比べた。許容誤差 = `heightScale/2 + 0.02`(量子化の最大誤差 + 浮動小数の余裕)。①本物の `heightmap.png`(4000x4000、最大高さ600、128x128マス): ランダム2万点 + 全マスの中央・1/4点・辺の近く・四隅の近く(13点 x 16384マス)+ 内側の全頂点で、**計24.9万点、ミス0、最大誤差 0.0078**(許容 0.0277)。②正方形でない・分割数が違う・ランダムな高さの地形4種(6x15マス 1200x3000、25x4マス 5000x900、3x3マスで最大高さ8000、1マス)で、各約2万点、**全点で一致(最大誤差 0.0106 / 0.0151 / 0.1001(許容 0.129。heightScale 0.219) / 0.0032)**。**X・Zの取り違えがあれば大きくずれる形**(縦横比が違うので)で一致した。③**逆の対角線との違いを見分けられる**ことも確認: 逆の対角線(b-d)の高さと許容誤差の4倍以上違う点が、本物の画像で約1万点、ランダム地形で各約2万点あり、そのすべてで `tessFlag` 側(a-c)と一致した。④法線(`hit.normal` と `GetNormalAt` の内積が0.999以上)・地形の外(4方向)に当たらない・オーナーの位置がワールドに反映される(角の位置)。
- **Dynamic**: 平らな地形(高さ150)に落とした箱(100)・球(半径50)・カプセル(半径40、高さ180)が、**ちょうど期待の高さ(150 + 半分)で止まった**(誤差0.001未満)。約3000/秒で落とした小さな球(半径20)が、すり抜けずに跳ねて最後は上で止まった(最小Y=期待どおり)。約26.6度の斜面で、球が低い方(-X)へ転がり、箱・球とも地形にめり込まない(球の地形からの高さの最小53.9 >= 半径50)。Player相当(100の箱、回転固定、`SetVelocity` で速さ200)と、半径50・高さ100のカプセルが、**本物の丘(最大の傾き約37度、平均約13度)を4方向とも12秒で水平に約2100〜2290進んだ**(平地なら2400。止まらず、めり込まず、地形の上を沿って歩けた)。
- **Static は動かない(仕様)**: オーナーを動かしても、当たり判定は作った位置のまま(確認済み)。README に明記した。
- **解放**: `Uninit` でアクターを先に、その後に高さフィールドを解放する。`getNbHeightFields()` が 解放前3 → 解放後0、50回の作成・解放のあとも0、`Uninit` を2回呼んでも二重解放しない、`Physics::Uninit` → `Init` をやり直しても地形が使える(`PxCooking` の作り直し)、地形を使わずに `Init` → `Uninit` も確認。既存の箱の `Raycast` は変わらない。
- **Sample3D**: `Hills` に `TerrainRigidbodyComponent` を付けた。**`Ground`(`GroundController` と見えない大きな箱)は置き換えず、残した**。理由: 床の上面(Y=-50)は丘の底と同じ高さで、丘の端は床の1.5上までスロープで下がって床につながり、内側では床は丘の下に隠れるだけで見た目も動きも変わらない。丘の外に落ちない保険にもなる。`GroundController` の削除、`EnemyManager` の出現の高さを `GetHeightAt` にすること、Player の出現位置は、置き換えていないので今回はしていない(敵は今の地面のまま。Playerと敵は丘から遠い所に出る)。**弾**: 今は地面の箱にも反応しない(弾は敵だけに当たって消える)ので、地形に当たっても何も起こらない(変更なし)。
- **端の壁(落とし穴の記録。サンプルは解決済み)**: 高さフィールドには**側面の壁が無い**。実測で、地面(箱)の上を歩いた Dynamic の箱が、地形の端(端の高さは72)から横へ歩き込むと、**地形の下にもぐった**(西・南・東の3方向とも)。PhysX 4.1 の `PxHeightFieldDesc` には、下へ厚みを付ける項目(`thickness`)が無い。設計書に「外周に壁を足す」は無いので**足していない**。**対策として、サンプルの `heightmap.png` を外周25%(約1000)で smoothstep により端へ下げ、端の高さを1.5(周りの地面 Y=-50 のすぐ上。ちょうど0だと地面のタイルとチラつく恐れがあるため)にした。丘の内側の形はそのまま。最大の傾きは約33度。実測で、地面から Player 相当の箱(速さ200)が東西南北4つの端から歩き込み、すべて丘の上に登れた(もぐらない)。** 自分の画像でも、端を0(周りの地面と同じ高さ)に下げると、もぐらない。地形の下に保険の床は、`Ground` が残っているので要らない。
- **`isTrigger`**: PhysX の高さフィールドはトリガーにできない。引数は設計書どおり残したが、`true` を渡すと出力ウィンドウに知らせて普通の当たり判定にする。
- **デバッグ表示**: 地形には枠線を描かない(`RigidbodyComponent::Draw` は高さフィールドで何もしない。設計書どおり)。
- **確認できていないこと**: 実機で Player を操作して丘を登る・降りる(ユーザーの確認待ち)。Visual Studio のデバッグレイヤー。`GetNormalAt` が急な坂で Player の移動に与える影響(未使用)。

**未確定の点(決まったこと)**:
- 地形の下に、保険の薄い見えない床を残すか → **`Ground`(見えない大きな箱)をそのまま残した**(地形の下に隠れる)
- 地形の外側(高さフィールドの外)の扱い(落ちる / 見えない壁で囲む) → **落ちる(足していない)**。Sample3D は `Ground` があるので落ちない。端から地形の下にもぐる件は上に記録
- 地形に `"Wall"` のタグを付けて、視界(6.1)をさえぎる地形にするか → **付けていない**(`Hills` は視線をさえぎらない。付けたい時は `SetTag("Wall")` だけでよい)

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
- **時間を渡す方法**: `Time::GetTime()`(倍率の影響を受ける=ポーズ中は止まる)を、`b6`(`WaterBuffer`。5.1の表のとおり)に入れて送る。波の本数分の「向き・振幅・周波数・速さ」も `b6` に入れる(`float4` の配列)。**落とし穴**: シェーダーの `float` で何時間も遊ぶと、`sin` の中の値の精度が落ちて波がガタつく。数十分〜数時間なら問題ないが、気になる時は `fmod` で周期を戻す。
- **`GetHeightAt`(CPU側)**: 浮かべる物・泳ぐ判定用に、**シェーダーと同じ波の式を C++ にも書く**。2つの式がずれると、水に浮かべた物が波と合わなくなるので、式を1か所のコメントに書き、2つとも同じ定数(`b6` に入れる値と同じ配列)から計算する。
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

### 6.10 ポイントライトの影

**目的**: 全方向に光るポイントライト(`Lantern` のようなランタン・たいまつ)が、壁や `Player` の後ろに影を落とすようにする。6.5 で作った影(太陽・スポット)の続きで、**6.5 で「作れない」としていたポイントライトの影**を作る。

> **6.5 との違い**: 太陽は一方向、スポットは円すいの範囲なので、光から見た絵が1枚で済みます。ポイントライトは**360度すべての向き**に光るので、光から見た絵が1枚では足りません。**上下左右前後の6方向の絵(キューブマップ)**を作ります。影のパスが6回に増えるのが、この機能の一番のコストです。

**ファイル案**: 新しいファイルは作らない。`Engine/Shadow.h/.cpp`(キューブの深度テクスチャ・6面のパス・`Bind` の `t3`)、`Engine/Lighting.h/.cpp`(影を落とすポイントライトの選択)、`Shader3DCommon.hlsli`(`PointShadow`・`b5` の拡張・`t3`)、`Engine/LightComponent.h`(`SetCastShadow` の説明をポイントにも効くように直す)。

**関数**:
```cpp
// Shadow::Settings に足す(Shadow.h)
int   pointResolution = 512;    // キューブの1面の大きさ(正方形。256 / 512 / 1024)。6面ぶん描くので、スポットより小さめ
float pointDepthBias = 3.0f;    // 判定のゆとり(ワールド単位。光から見た距離の方向)
float pointStrength = 1.0f;     // 影の濃さ(0=影なし、1=そのポイントライトの光が真っ暗)。normalBias と pcfRadius は平行光源と同じ値を使う

// Lighting に足す(Lighting.h)。GetShadowSpotLight と同じ形
const LightComponent* GetShadowPointLight();   // 今フレームの、影を落とすポイントライト(無ければnullptr)

// 使う側(LightComponent は今のまま。ポイントライトにも効くようになるだけ)
light->SetCastShadow(true);   // 今は「スポットだけ」。ポイントライトにも効かせる
```
```hlsl
// Shader3DCommon.hlsli
TextureCube pointShadowMap : register(t3);    // ポイントライトの影(6面の深度)
// ShadowBuffer(b5)の末尾に足す: pointPos(xyz=光の位置, w=1/解像度)、pointParams(x=使う, y=depthBias, z=near, w=far)、
//                                 pointParams2(x=ぼかしの半径, y=normalBias, z=strength)
float PointShadow(float3 worldPos, float3 normal, float3 lightPos, float lightNdotL);   // 0〜1(1=影なし)
```

**方式の比較(キューブマップの作り方)**:

| 方式 | 仕組み | 長所 | 短所 |
|---|---|---|---|
| **A. 6面の深度 + `TextureCube` の比較サンプリング**(おすすめ) | 光の位置から、+X −X +Y −Y +Z −Z の6方向を、視野90度・縦横比1の透視投影で深度だけ描く(6.5 のスポットの影を6回やる形)。本番では「光から見たピクセルの向き」でキューブを引き、比較サンプラー(`s1`)で影かどうかを判定する | **6.5 の `VSShadow` と `DrawDepth` をそのまま使える**(ライト空間の行列を面ごとに差し替えて6回呼ぶだけ)。新しいピクセルシェーダーが要らない。比較サンプラーの2x2補間(PCF)がハードウェアで効く | 比べる深度は「その面のz(奥行き)」なので、ピクセルの向きから**どの面か(絶対値が最大の軸)とそのz**を自分で求める必要がある。透視投影の深度は遠いほど粗いので、スポットと同じく、ワールド単位のバイアスを深度に直す |
| **B. 距離を書く方式** | 6面を `R32_FLOAT` のレンダーターゲットに、「光からの距離」をピクセルシェーダーで書く(別に深度バッファも要る)。本番では、ピクセルの向きでキューブを引いて、書かれた距離と自分の距離を比べる | 距離そのものを比べるので、軸やzの計算が要らず、バイアスも「距離の差」で単純に書ける | **新しいピクセルシェーダー**(ワールド位置を渡す頂点シェーダーと、距離を書くピクセルシェーダー)が `Shader3D.hlsl`・`Shader3DSkin.hlsl` の両方に要る。比較サンプラーが使えないので、PCFは自分で複数回読んで平均する(ハードウェアの補間が効かない)。レンダーターゲット+深度の両方が6面ぶん要る |

**おすすめは A**。理由: ①既存の `VSShadow`・`Mesh::DrawDepth`・`SkinMesh::DrawDepth` が**そのまま使え**、`Shader3D.hlsl`・`Shader3DSkin.hlsl` を変えずに済む(Engineの変更が少ない) ②`Shadow::Caster::DrawShadow(const LightView&)` の形も変えずに、登録済みの物を6回呼ぶだけ ③PCFがハードウェアに任せられる。Bは、Aで面の境目の乱れが直らなかった時の代案として残す。

**設計のポイント(仕組み)**:
1. **影のパス(1フレームに最大6回)**: 光の位置 `p` から、次の6つの向きを見る行列(`XMMatrixLookToLH`)と、視野90度・縦横比1・near〜far の透視投影(`far` = 光の `range`、near は小さく。例 10)を作り、`R32_TYPELESS` の6枚の配列テクスチャ(`MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE`、`ArraySize = 6`)の1枚ずつに深度を描く。面ごとに `D32_FLOAT` の DSV(`Texture2DArray` で `FirstArraySlice = 面の番号`、`ArraySize = 1`)を作り、読む時は `R32_FLOAT` の `TextureCube` の SRV を1つ作る。

   | 面の番号 | 向き | 上 |
   |---|---|---|
   | 0 | +X | +Y |
   | 1 | −X | +Y |
   | 2 | +Y | −Z |
   | 3 | −Y | +Z |
   | 4 | +Z | +Y |
   | 5 | −Z | +Y |
2. **本番(ピクセルシェーダー)**: ライトからピクセルへのベクトル `v = worldPos(法線で少しずらした位置) − lightPos` を求める。**`v` の絶対値が最大の成分が、見るべき面**で、その成分の大きさ `z = max(|v.x|, |v.y|, |v.z|)` が、その面の透視投影の「奥行き」になる。比べる深度は `depth = (far / (far − near)) − (far × near / (far − near)) / z`(左手系・0〜1の深度)。これを `pointShadowMap.SampleCmpLevelZero(shadowSmp, v, depth − biasInDepth)` で比べる(キューブは `v` の向きで引ける)。
3. **バイアス**: スポットの `SpotShadow` と同じ考え方。ワールド単位の `pointDepthBias` を、`z` を `z − pointDepthBias` にずらした深度の差として引く(`depth(z − bias)`)。加えて、法線方向へ `normalBias × (1 − ndotl)` だけずらす。ラスタライザの `SlopeScaledDepthBias` は、スポットの深度パスと同じ状態を使い回す。
4. **PCF**: 向き `v` を、面の接線の方向に少しずらして複数回読む(`pcfRadius` = 平行光源と同じ値。1 = 3x3)。**ずらした向きごとに、`depth` と面を求め直す**(下の「面の境目」を参照)。
5. **影を落とすポイントライトは1つまで**: `Lighting` が、`SetCastShadow(true)` のポイントライトのうち、使われる光(カメラに近い8個)の中で**3Dカメラに一番近い1つ**を選ぶ(スポットと同じ選び方)。スポットとは別枠なので、**影を落とすスポット1つ + ポイント1つ + 太陽**を同時に使える。選んだ光の `GpuLight::innerPad[1]` に印(1)を付けるのは今のまま(種類は `colorType.w` で分かるので、ポイントとスポットのどちらの影を引くかはシェーダーが決める)。
6. **無効の時**: `Shadow::GetSettings().enabled = false`、または影を落とすポイントライトが無い、または `pointResolution` ぶんのテクスチャが未作成の間は、**6回のパスを実行しない**(コスト0・見た目は今と同じ)。キューブは、使う最初のフレームで作る(使わないシーンではメモリを使わない)。
7. **描画状態の保存と復元**: 6.5 の `Shadow::Render` が、描画先・深度・ビューポート・ラスタライザ・ブレンド・深度ステンシルを覚えて戻す仕組みの中に、6面のループを足すだけにする。**`t3` も、書く前に外す**(同じ物を同時に読み書きしない)。
8. **スキンメッシュ**: **描く**(`ModelRenderer::DrawShadow` が、呼ばれるたびに `SkinMesh::DrawDepth` を呼ぶ作りなので、6回呼ばれるだけで、`MeshRenderer`・`ModelRenderer` を変えずに済む)。骨の行列を6回送る分だけ少し重い。
9. **範囲の絞り込み(任意)**: 光の `range` の外にある物は、どの面にも映らないので、`DrawShadow` の前に球の判定(物の位置が光から `range` + 半径以内か)で省く。`Visibility` の関数は、カメラ用なので使えない(光は別の位置から見る)。まず省かずに作って、重ければ足す。
10. **レジスタ**: 新しいテクスチャスロットは **`t3`**(`t1` = 太陽、`t2` = スポット、`t3` = ポイント)。比較サンプラー `s1` は共用する。定数バッファは **`b5`(`ShadowBuffer`)の末尾に足す**(別の `b` を作らない)。**理由**: ①6面の行列は影のパスで `DrawDepth` に引数で渡すので、本番のシェーダーには要らない(`b5` に足すのは位置・near・far・パラメーターだけで、64バイト程度) ②1フレームに1回しか更新しないので、`b5` と更新の頻度が同じ ③末尾に足せば、今ある項目の位置(オフセット)が変わらない。`Shadow::ShadowConstantBuffer` も同じ並びで末尾に足し、`static_assert(sizeof % 16 == 0)` を守る。**落とし穴**: C++ の `ShadowConstantBuffer` と HLSL の `ShadowBuffer` の並びがずれると、全部の影が壊れる。両方を同時に直す。

**難しい・大きい点(正直に)**:
- **性能(影のパスが6回増える)**: 1フレームの描画命令は「影を落とす物の数 × 6」増える(太陽・スポットの影と合わせて、最大 1 + 1 + 6 = 8パス)。Sample3D の規模(落とす物が5〜10個)なら問題ないが、物が多いと重くなる。解像度は512(6面で 512×512×4バイト×6 = 約6MB。1024 にすると約24MB)。重ければ、解像度を下げる・`range` の外を省く(設計のポイント9)・動かない光は毎フレーム描かない、で下げる。
- **面の境目(落とし穴)**: ピクセルの向きが2つの面の境目にある時、PCFで周りを読むと**隣の面**を読む。隣の面の深度は、別の軸のz(奥行き)なので、**中心の面のzで作った `depth` と比べると影が崩れる**(境目に線が出る)。対策: ①ずらした向きごとに、面とzを求め直して比べる(設計のポイント4) ②`pcfRadius` は1まで ③面の取り違えを避けるため、視野を90度ちょうどにし、キューブのハードウェアの継ぎ目の補間(D3D11 は標準で有効)を使う。**境目の見え方は、実機で確かめる**。
- **面の向きの取り違え**: キューブの6面の「向き」と「上」(設計のポイント1の表)は、D3D のキューブの座標系(左手系)と合わせる。1面でもずれると、影が別の方向に出る。**確認の方法**: 光の周りの6方向に1つずつ箱を置いて、それぞれの影が正しい方向に出ることを見る。
- **光源が自分のメッシュの内側にある問題(6.5 の `Guard` と同じ)**: `Lantern` は光る球(`MeshRenderer`)の中心にポイントライトがある。この球が `castShadow = true` だと、**光源が球の内側にあるので、球が全方向の光をすべてさえぎる**(周りが真っ暗になる)。対策: ①`Lantern` の球は `castShadow = false` のままにする(`Material` の既定が `false` なので、何も書かない) ②光源を内側に含む物に `castShadow` を付けない、を約束にする(`Lantern` の `.cpp` のコメントに書く)。将来の代案は、影のパスで表面(CCW)を描かず裏面だけ描く(光源を囲む物が影を落とさない)が、通常の物の影が崩れるので今回は採らない。
- **光の `range` の外**: ピクセルが光から `range`(= `far`)より遠いと、キューブに映らない(深度が1.0)。ただし、その場所は `range` で光が0になる(`ShadePixel` の `dist >= lightRange` で `continue` する)ので、**影を引かない**(`PointShadow` は `dist < range` の中でしか呼ばれない)。`far` を `range` と同じにするので、`range` の端で影が途切れることはない。`near` が大きすぎると、光のすぐ近くの物が映らない。
- **複数のポイントライト**: 影を落とせるのは1つだけ。2つ目以降は、`SetCastShadow(true)` にしても**影が出ない**(明るく光る)。選ぶ基準は「カメラに近い1つ」なので、カメラが動くと、影を落とす光が**切り替わる**(影が急に出たり消えたりする)。許容するか、優先順位(`SetShadowPriority` など)を足すかは未確定。
- **透明部分・敵(スプライト)**: 6.5 と同じ。画像の透明部分も板全体が影になり、`SpriteRenderer`(敵)は影を落とさない。
- **実機GPUでの見た目**: バイアスと境目は、GPU・解像度で見え方が変わる。必ずユーザーが遊んで確認する。

**段階分け(提案)**:
- **段階1**: 影を落とすポイントライト1つ、`MeshRenderer` / `ModelRenderer`(静止のパーツ)が影を落とす。PCFなし(`pcfRadius = 0` 相当)。解像度は設定。`Lantern` の `SetCastShadow(true)` で確認する。
- **段階2**: PCF(面の境目の対策つき)、スキンメッシュ、`range` の外の物を省く。
- **将来(今回は作らない)**: 複数のポイントライトの影、動かない光のキューブの使い回し(毎フレーム描かない)、光源を囲む物の扱い(裏面だけ描く)、柔らかい影(PCSS)。

**両サンプルへの影響(変えるEngineファイル。了承が必要)**:
- 新規: なし(`Engine.vcxproj(.filters)` も変えない)
- 変更: `Engine/Shadow.h/.cpp`(`Settings` に `pointResolution` / `pointDepthBias` / `pointStrength`、キューブのテクスチャ・6面の DSV・SRV、`Render` に6面のパス、`Bind` に `t3`、`ShadowConstantBuffer` の末尾に項目、`GetDrawnCount` にポイントの分)、`Engine/Lighting.h/.cpp`(影を落とすポイントライトの選択。`GetShadowPointLight()`)、`Shader3DCommon.hlsli`(`b5` の末尾・`t3`・`PointShadow`・`ShadePixel` のループで、ポイントライトの影を引く)、`Engine/LightComponent.h`(`SetCastShadow` の説明と、ポイントに効くようにした注意書き)
- **変えない**: `Shader3D.hlsl`・`Shader3DSkin.hlsl`(`VSShadow` を使い回す)、`Mesh.h/.cpp`・`SkinMesh.h/.cpp`(`DrawDepth` は行列を引数で受け取る作りのまま。`Draw` の `Shadow::Bind()` が `t3` も一緒にセットする)、`MeshRenderer.h`・`ModelRenderer.h/.cpp`(`DrawShadow` が6回呼ばれるだけ)、`GameEngine.cpp`(`Shadow::Render` / `Shutdown` を呼んでいる今のまま)、`Material.h`、`Component.h`・`GameObject.h`・`Scene`・`SceneManager`・`Graphics`、`Shader.hlsl`、`PhysX.props`
- **場合により変える**: 影のパスの呼び出し側(`Mesh` / `SkinMesh` の `DrawDepth`)に、面ごとの引数(ビューポートの切り替えなど)が要るとわかったら、その時だけ変える(今の `DrawDepth` は、描画先・ビューポートを `Shadow::Render` が決める作りなので、要らない見込み)。
- **`enabled = false`、または影を落とすポイントライトが無い間は、6面のパスを実行しないので、見た目は今と同じ**(既定OFF。`SetCastShadow` の既定は `false`)。Sample2D はポイントライトも影も使わない(影響なし)。ただし `Shader3DCommon.hlsli` はSample2D のデバッグ表示(枠線)も通るので、**コンパイルが通り、見た目が変わらないこと**を確認する。

**置き換える・使うSampleのコード**:
```cpp
// Sample3D Lantern(Objects/Lantern.cpp): ポイントライトが影を落とす
auto* light = lantern->AddComponent<LightComponent>(LightType::Point);
light->SetRange(800.0f);
light->SetCastShadow(true);    // 追加: ランタンの光が、壁や Player の後ろにさえぎられる
// 球の見た目(meshRenderer)は castShadow を指定しない(false のまま)。
// 光源が球の内側にあるので、球が影を落とすと全方向の光がさえぎられて真っ暗になる(Guard と同じ理由)

// Sample3D ShadowManager: 変更なし(Shadow::GetSettings().enabled = true は6.5で済み)
```
対象: Sample3D の `Lantern`(1行足すだけ)。ランタンが回りながら、`Player`・`Wall`・`ModelDisplay` の影が地面に回って動く。Sample2D は使わない。

**依存する機能**: 6.4(ポイントライト・`Lighting`)、6.5(`Shadow`・`VSShadow`・`DrawDepth`・`ShadowBuffer`・比較サンプラー)。

**未確定の点**:
- 方式は A(6面の深度 + `TextureCube` の比較サンプリング)でよいか(Bは、Aで境目が直らなかった時の代案)
- 影を落とすポイントライトを1つまでに制限してよいか(カメラが動くと切り替わる点を許容するか。優先順位の指定を足すか)
- 解像度の既定値(512)と、`pointDepthBias`(3.0)の初期値でよいか(実機で調整)
- `LightComponent::SetCastShadow` を、ポイントにも効かせる形(今のまま。名前を変えない)でよいか
- 光源を囲む物(`Lantern` の球)の扱いは、「`castShadow` を付けない」という約束だけでよいか(裏面だけ描く方式は将来)
- 範囲の絞り込み(`range` の外を省く)を段階1に入れるか、重くなってから足すか
- スキンメッシュの影を、段階1から入れるか(`DrawShadow` が6回呼ばれるだけなので、入れても手間は小さい見込み)

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
9. **6.5 影**: `Component` に `DrawShadow` を足す方式か、登録簿方式か。解像度と範囲の既定値(2048、3000)。敵(画像)の足元に丸い影を置くか。スポットの影(段階3)まで作るか。→ **登録簿方式・既定値はそのまま・丸い影は置かない・スポットの影まで作った(6.5 に書いた)**
10. **6.6 枠線**: 「光る」にブルームは期待しない前提(リム + 法線拡大)でよいか。敵の画像にも枠線がほしいか。→ **ブルームは作らない・リム + 法線拡大でよい・敵の枠線は付けない(6.6 に書いた)**
11. **6.7 地形**: 画像(PNG)を先、CSVを後でよいか。高さの精度(8ビット/16ビット)。大きさ・高さ・分割数の既定値。サンプルの高さ画像を作ってよいか。地形の外側の床を残すか。
12. **6.8 地形コライダー**: 地形の下の保険の床を残すか。地形に `"Wall"` タグを付けるか。
13. **6.9 水面**: 水に入ると遅くなる処理まで入れるか。深度を使った表現を将来やるか。
14. **6.10 ポイントライトの影**: 6面の深度のキューブ方式(A)でよいか。影を落とすポイントライトは1つまででよいか。解像度512・`Lantern` に `SetCastShadow(true)` でよいか。
15. **作る順番・担当者**: 4章の順番でよいか。担当者は誰か。

---

## 8. Engine の変更ファイル一覧とリスク

### 8.1 機能ごとに変える予定の Engine ファイル(すべて、変える前にユーザーの了承が必要)

| 機能 | 新しいファイル | 変える既存ファイル |
|---|---|---|
| 6.1 視界判定 | `Visibility.h/.cpp`、`VisionComponent.h/.cpp` | `MeshRenderer.h`、`ModelRenderer.h/.cpp`、`SpriteRenderer.h/.cpp`、`Model.h/.cpp`、`GameEngine.cpp`、`Image.h/.cpp`(縦横比。任意)、`Engine.vcxproj(.filters)` |
| 6.2 カプセル | (なし) | `RigidbodyComponent.h/.cpp`、`Mesh.h/.cpp` |
| 6.3 マテリアル | `Material.h`、(任意)`Shader3DCommon.hlsli` | `Shader3D.hlsl`、`Shader3DSkin.hlsl`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`、`MeshRenderer.h`、`ModelRenderer.h/.cpp`、`Engine.vcxproj`(`.hlsli` の登録) |
| 6.4 ライト | `LightComponent.h/.cpp`、`Lighting.h/.cpp` | `Shader3D.hlsl`、`Shader3DSkin.hlsl`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`、`GameEngine.cpp`、`Engine.vcxproj(.filters)` |
| 6.5 影 | `Shadow.h/.cpp` | `Shader3DCommon.hlsli`、`Shader3D.hlsl`、`Shader3DSkin.hlsl`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`、`Material.h`、`MeshRenderer.h`、`ModelRenderer.h/.cpp`、`LightComponent.h`、`Lighting.h/.cpp`、`GameEngine.cpp`、`Engine.vcxproj(.filters)`(登録簿方式にしたので、`Component.h`・`GameObject.h`・`Scene`・`SceneManager`・`Graphics` は変えていない) |
| 6.6 枠線 | (なし) | `Shader3DCommon.hlsli`、`Shader3D.hlsl`、`Shader3DSkin.hlsl`、`Material.h`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`、`MeshRenderer.h`、`ModelRenderer.h/.cpp`(新しいファイルなし) |
| 6.7 地形 | `Terrain.h/.cpp`、`TerrainRenderer.h/.cpp` | `Engine.vcxproj(.filters)`(6.3の段階2で `Mesh` を作り直し済みの前提) |
| 6.8 地形コライダー | (なし) | `RigidbodyComponent.h/.cpp`、`Physics.h/.cpp` |
| 6.9 水面 | `WaterRenderer.h/.cpp`、`Shader3DWater.hlsl` | `Engine.vcxproj(.filters)`(`UpToDateCheckInput` と `CopyRuntimeFiles`) |
| 6.10 ポイントライトの影 | (なし) | `Shadow.h/.cpp`、`Lighting.h/.cpp`、`Shader3DCommon.hlsli`、`LightComponent.h`(`VSShadow`・`DrawDepth` を使い回すので、`Shader3D.hlsl`・`Shader3DSkin.hlsl`・`Mesh`・`SkinMesh`・`MeshRenderer`・`ModelRenderer` は変えない見込み) |

**`Shader.hlsl`(スプライト用)と `PhysX.props` は、10機能のどれでも変えません**(クッキングのライブラリは既にリンク済み)。`Shader3D.hlsl` / `Shader3DSkin.hlsl` は 6.3・6.4・6.5・6.6 で変えます。

### 8.2 設計上の大きなリスク

1. **影(6.5)が一番大きい(実装済み。登録簿方式にして、根幹のファイルは触らずに済んだ)。** 描画の流れ(`GameEngine::Run`)に影のパスが加わり、レンダーターゲット・ビューポートを切り替える。戻し忘れると画面が真っ黒になる。`Component` / `GameObject` / `Scene` / `SceneManager`(Engineの根幹)にも手が入り、Sample2D を含む全体に影響する。影のバイアス(シャドウアクネ・影の浮き)の調整は、数値の試行錯誤が必要で、GPU・解像度で見え方が変わる。段階分け(平行光源だけ → スキンメッシュ → スポット)で、一歩ずつ進める。
2. **`Mesh::Draw` の作り(6.3 段階2)が、地形・影の前提。** 今は描く度に頂点をコピーする。これを直さずに地形・影を作ると、重くて動かない。6.3 で必ず直す。直す時に、既存の呼び出し(デバッグ表示・`GroundController`・`ModelRenderer`)が壊れないよう、`Draw` の引数は残す。
3. **地形コライダー(6.8)は、見た目(6.7)との「ずれ」が一番の危険。** ①三角形の対角線の向き(`tessFlag`) ②高さが `int16` の丸め ③行(X)・列(Z)の向き、のどれがずれても、物が浮いたり、地面に埋まったりする。`GetHeightAt` と `Physics::Raycast` の高さを比べる検証を必ず行う。地形の下に潜ると落ち続ける危険、Dynamic の箱が坂で引っかかる危険(→カプセル)もある。
4. **シェーダーの変更は、実機で見ないと分からない。** `Shader3D.hlsl` は Sample2D のデバッグ表示も通る。コンパイルエラーは実行時に出力ウィンドウに出る(ビルドは通る)ため、**ビルドが通っても画面が真っ黒になりうる**。必ず起動して確認する。
5. **`#include` 化(5.2)と新しいシェーダーのコピー設定を忘れると、実行時に何も描かれない。** `Engine.vcxproj` の `CopyRuntimeFiles` / `UpToDateCheckInput` への登録を、チェックリストに入れる。
6. **視界判定(6.1)の画面外判定は、半径が小さいと物が消える。** 骨のモデル・アニメーションで動くパーツは余裕を持たせる。`GetCulledCount()` で確認する。
7. **半透明の描画順(6.3・6.9)は、`Scene::Draw` が距離順でないため完全ではない。** 許容するか、`Scene::Draw` を直すか(2Dにも影響)は別の相談。
8. **敵(画像のビルボード)は、ライト・影・枠線に対応しない。** 見た目が揃わない場面が出る(ライトを置いても、敵だけ明るさが変わらない)。許容するか、敵を `ModelRenderer` に変えるか(5.16 の続き)は別の相談。
