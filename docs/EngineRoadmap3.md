# Engine 機能追加ロードマップ3(経路探索・LOD)

## 1. はじめに

### この文書の目的
Engine(`3D_FreamWork/Engine/`)に追加したい2つの機能(**経路探索**=敵が壁・障害物を避けて目的地へ向かう、**LOD**=遠い物は軽い形で描く・描かない)について、「どんな方式にするか」「どんな関数を作るか」「どの順番で作るか」「Engineのどのファイルを変えるか」「Sampleではどう使えるか」をまとめた設計書です。チーム(C++初心者を含む)で分担して実装する時の資料として使います。

### `docs/EngineRoadmap.md` / `docs/EngineRoadmap2.md` との関係
- `EngineRoadmap.md`(ロードマップ1)の17機能と、`EngineRoadmap2.md`(ロードマップ2)の視界判定・カプセル・マテリアル・ライト・影・枠線・地形・地形コライダー・水面は**実装済み**です(ロードマップ2のポイントライトの影は見送り)。この文書はその**続き**です。書き方(目的 / ファイル案 / 関数 / 設計のポイント / 使うSampleのコード / 依存する機能 / 未確定の点)は同じにそろえました。
- ロードマップ2の機能が「6.x」だったので、この文書の機能は **「7.1 経路探索」「7.2 LOD」** と番号を付けます(6章は用語の説明、7章が機能ごとの設計)。
- ロードマップ1・2の機能は**もう使える前提**で書いています。特に、経路探索は `Physics::OverlapBox`(5.6)、`TerrainRenderer::GetHeightAt/GetNormalAt`(6.7)、`MoveComponent`(5.x)を使います。LODは `Visibility`(6.1)、`Shadow`(6.5)、`Material`(6.3)、`TerrainRenderer`(6.7)の上に作ります。

### 状態
**7.1 経路探索は実装済みです(段階1〜3。7.1 の「実装したこと・決めたこと」を見てください)。7.2 LOD は段階1(距離で省く+描画カウンタ)だけ実装済みで、段階2・3・4は測定結果を見て決めます(7.2 の「段階1 実装したこと・決めたこと」と4章の表を見てください)。** 関数名・引数は、実装しながら変わってよいものです。この設計書は「実際のEngineのコードを読んで」書いたので、既存のクラス名・関数名・ファイル名は本物です。新しく作る名前だけが案です。

### 進め方の方針(1機能ずつ)
1機能ごとに、次の流れで進めます。

1. **着手前の確認**: 何を作るか・どのEngineファイルを変えるか・両サンプルへの影響を、短く説明する(下の「重要」を参照)
2. **implementer(作業役)が実装** + Debug/Release のビルド確認
3. **reviewer(レビュー役)がレビュー**(Engineの変更・新しいファイル・ロジックの変更は必須)
4. **ユーザーが遊んで確認**(キー入力・マウス操作の確認はユーザーにお願いする。合成したキー入力は送らない)
5. **コミット文を考える**(`commit-message` スキル。READMEも同じターンで更新)

> **重要: この文書の機能は、Engineを変える時にユーザーの了承が必要です。**
> `CLAUDE.md` の「docs/EngineRoadmap.md の機能を作る間の例外(了承を待たずに進めてよい)」の対象は、**`docs/EngineRoadmap.md` の機能だけ**です。この `EngineRoadmap3.md` の機能は対象に**入っていません**(`EngineRoadmap2.md` と同じ扱い)。そのため、Engine(`3D_FreamWork/Engine/` 以下、`Shader*.hlsl`、`PhysX.props`)を変える時は、通常のルールどおり **①何を変えるか ②なぜ必要か ③Sample2D・Sample3D 両方への影響** を説明して、ユーザーの了承をもらってから手を付けます。
>
> 「この設計書を読んで了承してもらうこと」を、各機能の着手前の確認として扱ってよいかは、**ユーザーが決めます**(8章の未確定事項 1番)。決まるまでは、安全側(機能ごとに着手前の確認をする)で進めます。
> また、了承は「その機能1つ分」だけです。前の機能で了承をもらっていても、次の機能では改めて聞きます。

### 名前の付け方・ファイルを追加する時
`EngineRoadmap.md` の「2. 共通ルール」と同じです(`CLAUDE.md` の命名規則に従う / 新しいファイルは `Engine.vcxproj` と `Engine.vcxproj.filters` の両方に登録 / 日本語を含む `.h`・`.cpp` は UTF-8 BOM付き / Debug と Release の両方でビルド確認)。
Engine は機能別の11フォルダ(`Core` / `Render` / `Render3D` / `Physics` / `Audio` / `UI` / `Animation` / `Transition` / `Behavior` / `Data` / `ThirdParty`)に分かれています(README の「Engineのフォルダ構成」)。新しいファイルは**一番近い役割のフォルダ**に置き、`.filters` のフィルターも同じフォルダ名にします。この文書のファイル案は、すべてフォルダ付きで書いてあります。

### 作ったら必ずSampleのコードを置き換える(使う)
これまでのロードマップと同じ約束です。1機能作ったら、Sampleの該当するコードを必ず置き換える(または新しく使う)ようにします。各節の「使うSampleのコード」がその対象です。

---

## 2. 作る順番の考え方

一覧表は 4章にあります。この章では、その順番にした理由を説明します。

- **7.1 経路探索を先、7.2 LOD を後にしました。ただし2つは独立しています**(お互いに依存しない)。担当者が分かれているなら、並行して作ってかまいません。順番を変えたい時は、8章の未確定事項 2番で決めてください。
- **経路探索が先の理由**: 今の敵(`ChaseComponent`)は壁を避けず、壁に当たったまま止まります。Sample2D・Sample3D の両方に壁(`Wall`)が置いてあるので、ゲームの動きとして一番目に見える改善です。また、障害物のデータ(格子)を作る部分は `Physics::OverlapBox` と `TerrainRenderer` が既にあるので、新しい仕組みを足さずに作れます。
- **LOD が後の理由**: LODは「速くする」機能なので、**速さを測る仕組み(描画した数のカウンタ)を最初に作る**のが段階1の仕事です。測らずに作ると、効いたのか分かりません。また、段階2(メッシュの切り替え)・段階3(地形のチャンク分け)は、効果が小さい場面もあります(7.2 の「正直な話」を参照)。段階1だけ先に作って、効果を測ってから先へ進むか決めるのがおすすめです。
- **それぞれの中も段階分けにしました**。経路探索は「2Dの格子+A*」→「3D(地形の傾き)」→「動く障害物」→(任意)フローフィールド。LODは「距離で省く+カウンタ」→「メッシュ切り替え」→「地形チャンク」です。

---

## 3. 今のEngineで分かっている事実(コードを読んで確認したこと)

設計に関わる「今のコードの事実」です。各機能の設計は、これを前提に書いています。

### 経路探索まわり
1. **`ChaseComponent` は壁を避けません。** `MoveComponent`(親クラス)が「目的地までのずれ」を求めて、まっすぐ向かう速度を作り、`ApplyVelocity` で動かすだけです。間に壁があっても、壁に押し付けたまま止まります。
2. **`MoveComponent` が使い回せる部品を持っています**(`protected`): `target` / `targetPosition` / `speed` / `stopDistance` / `plane`(`MovePlane::XY` か `XZ`)/ `GetOffsetToDestination` / `IsWithinStopDistance` / `ToPlane` / `GetDynamicRigidbody` / `ApplyVelocity`。`ApplyVelocity` は、Dynamic なら `RigidbodyComponent::SetVelocity`(XZ では今のYの速度=重力で落ちる速さを残す)、Kinematic・Rigidbody 無しなら `transform.position` を直接動かし、Static なら動かさず1回だけ知らせます。**経路を歩くComponentは、`MoveComponent` を親にすれば、この決まりをそのまま引き継げます。** コンストラクタが `protected` なので、`ChaseComponent` と同じ形(派生クラスで `AddComponent` する)にします。
3. **両サンプルの敵は `ChaseComponent` を使っています。** `EnemyManager.cpp` の `SetupEnemy`(プールに新しい敵を足す時の組み立て)で `AddComponent<ChaseComponent>()` し、`Spawn` で `SetTarget` / `SetSpeed` を渡し、`ApplySpeedToEnemies`(Enemies.csv の速さを実行中に反映する)が `GetComponent<ChaseComponent>()` で探しています。Sample3D は `chase->SetPlane(MovePlane::XZ)`。
4. **敵の体の大きさ**: Sample3D は `CapsuleRigidbodyComponent`(半径40・高さ100・Dynamic・重力あり・回転固定)、Sample2D は `SquareRigidbodyComponent`(一辺100・重力なし・Z固定)。敵は Player の周り `spawnDistance` の円周上に出て、Sample3D は `spawnHeight` の高さから落ちてきます。
5. **壁**: Sample3D は `CreateWall(scene, 位置, 大きさ)`(`MeshRenderer` の立方体 + `BoxRigidbodyComponent(BodyType::Static)`、タグ `"Wall"`、高さ200)、Sample2D は `CreateWall(scene, 位置, 大きさ)`(`SpriteRenderer` + `BoxRigidbodyComponent(Static, {w, h, 100})`、タグ `"Wall"`)。どちらも**動かず、回転もしていません**。`Ground`(Sample3D の地面)は 100000 四方の見えない大きな箱で、上面の高さは `-50`、タグは付いていません。
6. **`Physics::OverlapBox(center, size, tag, triggers)`**: 中心と**辺の長さ**(`transform.scale` と同じ)で箱を渡すと、中に入っている物(`GameObject*`)の一覧を返します。箱は回転しません。`tag` を指定すると、そのタグの物だけ。トリガーは既定で無視。**位置は最後に PhysX が計算した時のもの**で、出したばかりのオブジェクトは次のフレームから見つかります(`Physics.h` の決まり)。`OverlapSphere` / `Raycast` も同じ決まりです。
7. **地形**: `TerrainRenderer` は、ワールド座標で `GetHeightAt(worldX, worldZ)` / `GetNormalAt(worldX, worldZ)`(三角形1枚ごとの面の向き。長さ1)/ `Contains(worldX, worldZ)` を使えます。Sample3D の `Hills` は 4000 x 4000、`maxHeight` 600、128 x 128 マス、左手前の角が `{1500, -50, -2000}`。`TerrainRigidbodyComponent`(PhysXの高さフィールド)が同じデータから当たり判定を作るので、敵は丘を歩けます。`Hills` のオブジェクトにもタグは付いていません。`Water`(水面)には当たり判定が無く、通り抜けられます。
8. **デバッグ表示の流儀**: `RigidbodyComponent::SetDebugDrawEnabled(bool)` と `VisionComponent::SetDebugDrawEnabled(bool)` は、どちらも `static` で、「全部まとめてON/OFF」します。線は `Mesh`(`D3D11_PRIMITIVE_TOPOLOGY_LINELIST`)で描きます。**`Mesh` の頂点バッファは作る時に1回だけ送る作り(変更不可)**なので、`VisionComponent` は「距離1の扇形」を作っておき、`Transform` の `scale` と `rotate` で大きさと向きを変えて描いています。
9. **`VisionComponent` の前例**: 壁の判定(Raycast)の間隔を `SetCheckInterval`(既定0.1秒)で空け、最初の判定を ±20% ばらして同じフレームに集中しないようにしています。経路の再探索も同じ作戦が使えます。
10. **`Component::SetEnabled(bool)` がある**(`Update` / `Draw` を止める)ので、使わない間のComponentを止められます。`Time::GetDeltaTime()` / `Time::GetTime()` と、`Math::Distance` / `Math::DistanceSq` / `Math::Normalize` / `Math::Dot` があります。

### LOD・描画まわり
11. **`MeshRenderer` は、コンストラクタで自分専用の `Mesh` を作ります**(`mesh->Init(vertices)`)。同じ頂点データの壁が100個あれば、頂点バッファも100個できます(画像だけは `Resources` で共有)。`ModelRenderer` が使う `Model` は `Resources::LoadModel` で共有され、パーツごとの `Mesh` / `SkinMesh` は最初に描く時に作られて、みんなで共有します。
12. **`Mesh` の頂点バッファは作る時に1回だけ作る(変更不可)。** `Init`(頂点だけ)と `InitIndexed`(頂点+インデックス)があります。形の作り方は `CreateCube` / `CreateSphere(rings, segments)` / `CreateCapsule` / `LoadOBJ`。**`CreateSphere` は分割数を引数で変えられる**ので、段階の違う球は自動で作れます(LODの一番かんたんな材料)。
13. **`Mesh::Draw` は、物ごとに描画命令を出し、描く度に定数バッファ(`b0` / `b2` / `b3`)を送ります**。ライト(`Lighting::Bind`)と影(`Shadow::Bind`)のセットも描く度です。つまり「物の数」が増えるほど、三角形の数が少なくても重くなります。
14. **枠線(`Mesh::DrawOutline`)は、本物を描いた直後に追加で描きます**(ステンシルを書く・外側の輪を描くなど複数回)。`material.outlineWidth > 0` の物だけ。
15. **影のパス**: `Shadow::Render` が、登録された `Shadow::Caster`(`MeshRenderer` / `ModelRenderer` / `TerrainRenderer`)のうち、アクティブで有効な物の `DrawShadow` を呼びます。`material.castShadow = true` の物は、**カメラからの距離に関係なく**、物の数だけ `DrawDepth` が呼ばれます(`Shadow::GetCasterCount()` / `GetDrawnCount()` で数を見られる)。影の範囲は「3Dカメラの前方 `distance`(既定3000)の箱」ですが、箱の外の物を描かない仕組みは、今は書かれていません。
16. **`ModelRenderer` は、パーツごとに描きます**(`Draw` の中のループ)。描く度にパーツごとの行列を作り直し(`UpdateModelMatrices`、影のパスでもう1回)、骨で曲げるパーツ(`SkinMesh`)は**描く度に骨の行列(最大64個)を計算して送ります**。`ModelAnimationComponent::Update` は毎フレーム全パーツの動きを書き込みます(再生の間隔を空ける機能は今はありません)。
17. **`TerrainRenderer` は1つの `Mesh`(インデックス付き)で地形全体を描きます**。128 x 128 マスなら、頂点は 129 x 129 = **16,641個**、インデックスは 98,304個(**三角形32,768枚**)。最大の 256 x 256 マスでも頂点 66,049個です(`TerrainRenderer.h` のコメント)。`SetFrustumCulling(true)` は「地形全体を囲む球1つ」が映らない時だけ描かない(チャンク分けはしていない)。`castShadow = true` にすると影のパスでも全体を描きます。
18. **`Visibility`(6.1)**: `IsTransformedSphereInView3D(world, localRadius)` で、ワールド行列と半径から視錐台に入っているかを調べます。`SetFrustumCulling` は既定OFFで、`MeshRenderer`(頂点の最大距離)・`ModelRenderer`(`Model::GetBoundingRadius()` × 1.5)・`TerrainRenderer`・`SpriteRenderer` に付いています。`GetCulledCount()`(前のフレームで省いた数)は確認用です。**`GameEngine::Run` は `CameraShake::BeginDraw()` → `Visibility::BeginFrame()` → `Shadow::Render()` の順**なので、描画も影のパスも、カメラの揺れ込みの位置を `Image::GetCamera3D()` で読めます。
19. **確認用の数え方は、すでに2種類あります**: `Visibility::GetCulledCount()` と `Shadow::GetCasterCount()` / `GetDrawnCount()`。ただし、描画命令の数・三角形の数を数える仕組みはありません。
20. **`MaterialConstantBuffer`(シェーダーの `b2`)は float4 を7本使っていて、`outlineParams` の `w` だけ使われていません**(x=太さ、y・z=画面の大きさ)。将来、フェードの値を1つ送る場所に使えます(7.2 の段階4)。
21. **ロードマップ2の流儀**: 新しい設定は**既定OFF**にして、何もしなければ見た目も速さも今まで通りにします(`SetFrustumCulling` の既定OFF、`Shadow` の既定OFF)。この文書の機能も同じにします。

---

## 4. 作る順番(一覧表)

| 順 | 機能 | 理由・依存 | 担当者 | 状態 |
|---|---|---|---|---|
| 1 | [7.1 経路探索](#71-経路探索) | 5.6(`Physics::OverlapBox`)・6.7(`TerrainRenderer`)を使う。`MoveComponent` を親にする。単独 | | **実装済み(段階1〜3。段階4=フローフィールドは作らない)** |
| 2 | [7.2 LOD](#72-lod) | 6.1(`Visibility`)・6.5(`Shadow`)・6.7(地形)の上に作る。7.1とは独立 | | **段階1(距離で省く+描画カウンタ)だけ実装済み。段階2・3・4は測定結果を見て決める(未着手)** |

7.1 と 7.2 は独立なので、順番は入れ替えても構いません(8章の未確定事項 2番)。

---

## 5. 全機能で共通する設計

### 5.1 シェーダーは(段階1〜3では)変えない
7.1 は描画を変えません(デバッグの線を `Mesh` で描くだけ)。7.2 も、段階1〜3は `Shader3D.hlsl` などを**変えずに**作れます(描く物・描く回数を変えるだけ)。シェーダーを変える可能性があるのは、7.2 の段階4(フェード。任意)だけです。`PhysX.props` も変えません。

### 5.2 新しいEngineファイルの置き場所
| 新しいファイル | 置くフォルダ | 理由 |
|---|---|---|
| `NavGrid` / `PathFinder` / `NavGridComponent` / `PathFollowComponent` | `Engine/Behavior/` | 「動きの部品」の仲間(`ChaseComponent` と同じ場所)。格子のデータも一緒に置く |
| `RenderStats` / `Lod` | `Engine/Render3D/` | 3Dの描画の仲間(`Shadow` / `Material` と同じ場所) |

経路探索のファイルが増えるので、将来 `Engine/Navigation/` のような**12個目のフォルダ**を作る案もあります。ただし README の「11フォルダ」を変えることになるので、ユーザーが決めます(8章の未確定事項 3番)。決まるまでは `Behavior/` に置きます。

### 5.3 デバッグ表示は既存の流儀に合わせる
`SetDebugDrawEnabled(bool)` を `static` で用意し、「全部まとめてON/OFF」します(`RigidbodyComponent` / `VisionComponent` と同じ)。ONにするキー操作はSample側(既存のデバッグ切り替えの方法)に任せ、Engineは関数だけ用意します。

---

## 6. 用語のやさしい説明

- **経路探索**: 今いる場所から目的地まで、壁などにぶつからずに行ける道すじ(経路)を、プログラムで見つけること。
- **格子(グリッド)**: 地面を升目(マス)に区切ったもの。各マスに「通れる/通れない」を書いておく。
- **A\*(エースター)**: 経路探索の定番の方法。出発点から少しずつ広げながら、「目的地に近そうなマス」を優先して調べるので、全部のマスを調べなくても最短の道が見つかる。
- **ナビゲーションメッシュ(ナビメッシュ)**: 歩ける場所を、大きな三角形・多角形の集まりで表したもの。格子より少ないデータで広い場所を表せるが、作るのが難しい。
- **ウェイポイント**: 人が手で置いた「通過点」の点。点と点を線でつないで、その上を探索する。
- **フローフィールド**: 目的地から全マスに向かって「どちらに進めば近いか」の矢印を一括で作る方法。敵が大勢いて、目的地が同じ(Player)時に強い。
- **ストリングプル(経路の直線化)**: 経路の途中の点を、「まっすぐ見通せるなら飛ばして」短くすること。ひもをぴんと張るイメージ。
- **LOD(エルオーディー、Level Of Detail)**: 遠くにある物は小さくしか見えないので、三角形の少ない軽い形に差し替える(または描かない)こと。
- **ヒステリシス**: 切り替えの境目で行ったり来たりしてちらつかないように、「入る距離」と「出る距離」を少しずらしておくこと。
- **チャンク**: 大きな地形を、四角い小さな区画に分けたもの。区画ごとに「描く/描かない」「細かさ」を決められる。
- **描画命令(ドローコール)**: 「この物を描いて」とGPUに出す命令。三角形の数が少なくても、命令の数が多いと重くなる。

---

## 7. 機能ごとの設計

各節は「目的 / 方式の比較とおすすめ / ファイル案 / 関数 / 設計のポイント / 両サンプルへの影響 / 使うSampleのコード / 依存する機能 / 未確定の点」の順に書いています。「両サンプルへの影響」には、**ユーザーの了承が必要になる Engine の変更点**(変えるファイル)をまとめて書いています。

---

### 7.1 経路探索

**目的**: 敵が壁・障害物を避けて目的地(Player)へ向かえるようにする。今の `ChaseComponent` は「まっすぐ向かう」だけなので、壁の向こうにPlayerがいると壁に張り付いて止まる。**2D(Sample2D のトップダウン)と3D(Sample3D の地形・壁・丘)の両方**で使えること。初心者が読んで分かる作りにすること。

**状態: 実装済み(段階1〜3)。段階4(フローフィールド)は作らない。** 下の設計のうち、変わった所・決めたことを先にまとめます(使い方は README §36)。

#### 実装したこと・決めたこと

- **ユーザーの決定**: ①ファイルは `Engine/Behavior/` に置く(新フォルダは作らない) ②経路が見つからない時は**止まる**(設計のおすすめの「まっすぐ追う」ではない。`SetFallbackToStraight` は作らなかった)。止まっている間も探し直す ③両サンプルの敵を `ChaseComponent` から `PathFollowComponent` に置き換える ④池(`Water`)は通れるまま(障害物にしない) ⑤この設計書を、7.1 の着手前の確認として扱う ⑥フローフィールドは作らない。
- **新規ファイル(Engine)**: `Engine/Behavior/NavGrid.h/.cpp`・`PathFinder.h/.cpp`・`NavGridComponent.h/.cpp`・`PathFollowComponent.h/.cpp` と、`Engine.vcxproj(.filters)` への登録だけ。**既存のEngineファイル・`Shader*.hlsl`・`PhysX.props` は変えていない。**
- **Sample**: 両サンプルに `Objects/Navigation.h/.cpp`(`CreateNavigation`)を足し、`GameScene` で壁・丘の後に1行並べ、格子を `CreateEnemySpawner` の引数で `EnemyManager` に渡す(`EnemyManager` は `Setup(target, navigation)`)。`EnemyManager` で `ChaseComponent` を使っていた3か所(`SetupEnemy`・`Spawn`・`ApplySpeedToEnemies`)を `PathFollowComponent` にした。プール・CSVの速さの反映・倒した時の返却は今までどおり。
  - Sample2D の格子: 原点のまわり 6000 x 6000(`(-3000, -3000)` から)、`cellSize` 50、`agentRadius` 50。Sample3D の格子: `(-3000, -3000)` から 9000 x 9000、`cellSize` 50、`agentRadius` 40、`baseHeight` -50、丘の `TerrainRenderer` を `SetTerrain`、`maxSlopeDegrees` は既定の40度。
- **宣言案から変わった所**:
  - `NavGrid`: 障害物を「手で登録 / 物理から / 地形から」の3種類のビットで別々に覚える。焼き直しは自分の種類だけ作り直す(手で登録した分は消えない)。`Clear()` は全部消す。`IsInside(cellX, cellZ)` を足した。`version` は、障害物や高さが**実際に変わった時だけ**増える。
  - `NavGrid::BakeFromPhysics` は、8 x 8 マスをまとめた大きな箱でまず調べ、何も無ければそのまとまりを飛ばす(未確定の点 (e) の最適化を最初から入れた。1マスずつより約3倍速い)。
  - `BakeFromTerrain` の段差の判定は、「`maxStepHeight`」と「坂の傾きで説明できる高さ(`tan(maxSlopeDegrees)` x マスの距離)」の大きい方を超えたら、そのマスを塞ぐ(マスごと塞ぐ方式。行き来の禁止はしない)。`maxStepHeight` だけだと、40度の坂(50のマスで高さの差が約42)が段差30と間違われるため。
  - `NavGridComponent`: 焼くのは2回目の `Update`。`IsBaked()`・`GetLastBakeMilliseconds()`・`TryBeginSearch()`・`SetMaxSearchesPerFrame()`(1フレームの探索回数の上限。既定8)を足した。`RebakeRegion(中心, 大きさ)` は、次のフレームの `Update` で焼く(Kinematicの壁の位置が、動かしたフレームの次の `Physics::Update` で変わるため)。`RequestRebake()` と `SetAutoRebakeInterval()` も次のフレームの `Update` で焼く。
  - `PathFollowComponent`: `SetFallbackToStraight` は無い(止まる)。`SetRepathInterval` は最小0.1秒。経路が見つからなかった時は、探し直す間隔を4倍にする(行けない所の探索が重いため。目的地が1マス以上動いた時・格子が変わった時は、すぐ探し直す)。目的地が壁際(通れないマスの中)の時は、近くの通れるマスの中心まで行き、残りは目的地へまっすぐ向かう。
  - `PathFinder`: 始点から目的地へ `HasLineOfWalk` が通れば、A\* を呼ばずに `[目的地]` を返す(広い所の探索がほぼ0になる)。壁際の始点・目的地が同じマスに寄る時は、寄せずに目的地へまっすぐ。
- **焼くタイミングの実測**: 物理に置いた Static の箱は、`Physics::Update` の前でも `OverlapBox` に見つかった(出したばかりでも)。ただし Dynamic・Kinematic は次の更新からなので、安全のため2回目の `Update` で焼く。
- **実測(Release)**: 焼き(物理)は Sample2D の格子(120 x 120)で約0.1〜0.25ミリ秒、Sample3D の格子(180 x 180)で約0.2ミリ秒。丘の高さ・傾きの取り込みは約12ミリ秒(Debugは約200ミリ秒)。探索は、見通せる所で約0.008ミリ秒、壁を回り込んで約0.01〜0.04ミリ秒、丘をまたぐ長い道(約5,900マス)で約1.1〜1.3ミリ秒、行けない探索(20000マスまで調べて諦める)で約4.6〜4.8ミリ秒。メモリは Sample3D の格子で約700KB(格子 約160KB + 探索の作業用 約520KB)。
- **Hills の傾き**: 地形の三角形の傾きは最大で約33度、平均で約13度。`maxSlopeDegrees = 40` では丘の坂は1マスも塞がらない(20度にすると丘のマスの約15%、30度にすると約1%)。坂を避けさせたい時は、`CreateNavigation` の `maxSlopeDegrees` を下げる。
- **確認できたこと**: 画面なし・物理ありのスクラッチテスト(Debug・Release)で、ランダムな迷路600個のA\*の最短がDijkstraと一致、斜めのすき間は抜けない、到達不能・上限超え・格子の外は空、直線化した経路が壁を通らない、物理から取り込んだマスが計算と全マス一致、地形の傾き・高さ、Dynamicの箱(2D)・カプセル(3D)が壁を回り込んで着く、扉が開くと止まっていた敵が動き出す、一時停止・`ObjectPool` の再利用。**確認できていないこと**: 実際のゲームでの見え方・遊んだ時の難しさ・デバッグの線の描画(画面が要るため)。
- **ゲームの難しさが変わる**: 敵が壁を避けて回り込むので、壁の陰に隠れても追いつかれる。逆に、格子の外に出た敵は止まる。
#### 方式の比較とおすすめ

| 方式 | 仕組み | 良い点 | 悪い点 | 2D / 3D | 初心者向け |
|---|---|---|---|---|---|
| **格子(グリッド)+ A\*** | 地面を升目にして、通れないマスを避けて最短の道を探す | 仕組みが分かりやすい。2Dも3D(平らな地面+起伏)も同じ作りで使える。壁を自動で取り込める | マスの大きさで精度が決まる。広いとマスが多い。経路がカクカクしやすい(→直線化で解決) | ◎ / ○ | **◎** |
| ナビメッシュ | 歩ける場所を多角形で表し、その上を探索する | データが少ない。経路が自然。広い3Dに向く | 自動生成(三角形分割・穴あけ・結合)が難しい。バグりやすい。地形(起伏)から作るのも大変 | △ / ◎ | △ |
| ウェイポイント | 手で置いた点どうしをつなぎ、その上を探索する | 一番かんたん。動きを自分で決められる | 点を置く手間。点の間で壁にぶつかる。敵が増えたり壁を動かすと破綻。自動化できない | ○ / ○ | ○(小さい場面だけ) |
| フローフィールド | 目的地から全マスへ「進む向き」を一括で作る | 敵が何百体いても1回の計算で済む(目的地が同じ時) | 目的地が1つ(か少数)の時だけ。向きがマスの単位でカクカクする | ○ / ○ | ○ |

**おすすめ: 「格子(`NavGrid`)+ A\* + 見通しによる経路の直線化」を土台にする(段階1〜3)。必要になったら、同じ格子の上にフローフィールドを足す(段階4・任意)。**
- 理由1: 格子の「通れる/通れない」は、**物理の `Physics::OverlapBox`(`"Wall"` タグ)で自動で作れる**ので、壁を置くだけで経路探索に反映される(手作業が要らない)。
- 理由2: 2D(XY平面)も3D(XZ平面)も、**同じ格子のクラス**で扱える(`MovePlane` で平面を切り替える。`MoveComponent` と同じ考え方)。
- 理由3: 3Dの起伏は、格子の各マスに「地面の高さ」と「傾き」を覚えさせれば、`TerrainRenderer::GetHeightAt/GetNormalAt` で足りる。ナビメッシュのような新しい幾何の仕組みは要らない。
- 理由4: カクカクは「直線化」(格子の上で見通しを調べて飛ばす)で、見た目が自然になる程度まで減らせる。
- ナビメッシュは、広い3Dの本格的なゲーム向けで、**今のEngineと学生チームにとっては作りが大きすぎる**ので見送りを提案します。ウェイポイントは、格子でできることの下位互換なので採用しません(必要なら、将来「経路の途中に通過点を挟む」機能として足せる)。

#### ファイル案
新規(すべて新しいファイル。フォルダは 5.2 のとおり):
- `Engine/Behavior/NavGrid.h/.cpp` : 格子のデータ(通れる/通れない・高さ)と、マスと座標の変換、障害物の登録(手で/物理から/地形から)
- `Engine/Behavior/PathFinder.h/.cpp` : A\* による探索と、経路の直線化。作業用の配列を持ち回して使い回す
- `Engine/Behavior/NavGridComponent.h/.cpp` : `NavGrid` と `PathFinder` を持つComponent。シーンに1つ置く。焼く(障害物を読む)・再び焼く・デバッグ表示を担当
- `Engine/Behavior/PathFollowComponent.h/.cpp` : 経路に沿って歩くComponent(`MoveComponent` を継承)
- `Engine.vcxproj` と `Engine.vcxproj.filters` に上の4組(8ファイル)を登録(フィルター `Behavior`)

既存の変更: **Engine は変えない**(`MoveComponent` / `ChaseComponent` / `Physics` / `Terrain*` は読むだけ)。

#### 関数(宣言案)
```cpp
// ─── 格子 ───
struct NavGridSettings {
    XMFLOAT3 origin{};               // 格子の左手前の角のワールド座標(2D=XY平面なら x,y を使う)
    XMFLOAT2 size{ 6000.f, 6000.f }; // 格子の大きさ(2D=XY、3D=XZ)
    float    cellSize = 50.f;        // 1マスの辺の長さ。大きいほど軽くて粗い。敵の半径くらいがおすすめ
    MovePlane plane = MovePlane::XZ; // 歩く平面(MoveComponent と同じ)
    float    agentRadius = 40.f;     // 歩く物の半径。この分だけ障害物を太らせて、壁にこすらない道を作る
    std::string obstacleTag = "Wall";// このタグの物が障害物(VisionComponent の壁と同じタグ)
    // ── 3Dの高さ(planeがXZの時だけ使う) ──
    float    baseHeight = 0.f;       // 地形が無い所の地面の高さ(Sample3D の Ground なら -50)
    float    agentHeight = 100.f;    // 歩く物の背の高さ。これより低い物(段差)は障害物にしない
    float    maxStepHeight = 30.f;   // 登れる段差の高さ。隣のマスとの高さの差がこれを超えたら通れない
    float    maxSlopeDegrees = 40.f; // 登れる坂の角度。これより急なマスは通れない
};

class NavGrid {
public:
    static constexpr int maxCells = 512 * 512;           // 格子の上限(超えたら cellSize を自動で大きくして出力ウィンドウに知らせる)

    explicit NavGrid(const NavGridSettings& settings);

    // ── 障害物を取り込む(どれも「通れるマスを通れなくする」。Clear は全部通れるに戻す) ──
    void Clear();
    void SetBlocked(int cellX, int cellZ, bool blocked);                          // 手で1マス
    void SetBlockedBox(XMFLOAT3 center, XMFLOAT3 size, bool blocked = true);      // 手で箱の範囲(池・柵など、物理が無い障害物)
    void BakeFromPhysics();            // 全マスを Physics::OverlapBox(obstacleTag) で調べて、壁に重なるマスを通れなくする
    void BakeRegionFromPhysics(XMFLOAT3 center, XMFLOAT3 size);                   // 範囲だけ調べ直す(動く壁用)
    void BakeFromTerrain(const TerrainRenderer& terrain);                         // 地形の高さを各マスに覚え、急な坂・大きな段差を通れなくする

    // ── 調べる ──
    bool WorldToCell(XMFLOAT3 position, int& cellX, int& cellZ) const;            // 格子の外ならfalse
    XMFLOAT3 CellToWorld(int cellX, int cellZ) const;                             // マスの中心(yは覚えた高さ)
    bool IsWalkable(int cellX, int cellZ) const;                                  // 通れるか(格子の外は通れない)
    bool IsWalkable(XMFLOAT3 position) const;
    bool HasLineOfWalk(XMFLOAT3 from, XMFLOAT3 to) const;                         // 2点の間を通れるマスだけでまっすぐ行けるか(経路の直線化に使う)
    bool FindNearestWalkable(XMFLOAT3 position, XMFLOAT3& result, int maxRing = 6) const;  // 通れないマスの中にいる時に、近くの通れるマスを探す

    int  GetCellCountX() const;
    int  GetCellCountZ() const;
    const NavGridSettings& GetSettings() const;
    int  GetVersion() const;           // 障害物が変わる度に増える番号(歩く側が「経路が古くなった」と気付くため)
};

// ─── 探索 ───
struct PathOptions {
    bool  smooth = true;               // 経路を直線化する
    bool  allowDiagonal = true;        // 斜めの移動を使う(8方向)。壁の角を斜めに抜けるのは禁止する
    int   maxExpandedCells = 20000;    // 1回の探索で調べるマスの上限(これを超えたら諦めて空の経路を返す)
};

class PathFinder {
public:
    explicit PathFinder(const NavGrid* grid);
    // start から goal までの経路(ワールド座標の点の並び。先頭は start の次の点、最後は goal)。見つからなければ空
    // start / goal が通れないマスの中なら、近くの通れるマスに寄せてから探す
    std::vector<XMFLOAT3> FindPath(XMFLOAT3 start, XMFLOAT3 goal, const PathOptions& options = PathOptions{});
    int GetLastExpandedCount() const;  // 直前の探索で調べたマスの数(確認用)
};

// ─── 格子を持つComponent(シーンに1つ置く) ───
class NavGridComponent : public Component {
public:
    explicit NavGridComponent(const NavGridSettings& settings);
    NavGrid& GetGrid();
    PathFinder& GetPathFinder();
    void SetTerrain(TerrainRenderer* terrain);       // あれば、焼く時に地形の高さ・傾きを取り込む
    void SetAutoRebakeInterval(float seconds);       // 0=自動で焼き直さない(既定)。動く壁がある時だけ設定する
    void RequestRebake();                            // 次のフレームで全体を焼き直す
    static void SetDebugDrawEnabled(bool enabled);   // 通れないマスを線で描く(RigidbodyComponent と同じ使い方)
    void Update(float dt) override;                  // 最初の1回の焼き(物理に壁が現れる次のフレーム)、自動の焼き直し
    void Draw() override;
    void Uninit() override;
};

// ─── 経路に沿って歩くComponent ───
class PathFollowComponent : public MoveComponent {
public:
    void SetNavigation(NavGridComponent* navigation);  // どの格子で探すか(Spawn の度に渡す)
    void SetRepathInterval(float seconds);             // 再探索の間隔(既定0.5秒。±20%ばらす)
    void SetWaypointRadius(float distance);            // この距離まで近づいたら次の点へ(既定は cellSize の半分くらい)
    // ※経路が見つからない(格子の外・行けない)時は、止まる(ユーザーの決定。まっすぐ追う設定 SetFallbackToStraight は作らなかった)
    bool HasPath() const;
    const std::vector<XMFLOAT3>& GetPath() const;      // 今の経路(確認・デバッグ用)
    static void SetDebugDrawEnabled(bool enabled);     // 今の経路を線で描く
    void Update(float dt) override;
    void Draw() override;
};
```
`SetTarget` / `SetTargetPosition` / `SetSpeed` / `SetStopDistance` / `SetPlane` / `HasArrived` は `MoveComponent` のものをそのまま使います(`ChaseComponent` と同じ呼び方)。

#### 設計のポイント(落とし穴を含む)

**(a) 障害物の取り込み方**

| 取り込み方 | 内容 | 使い所 |
|---|---|---|
| **物理から自動(おすすめ・既定)** | 全マスについて `Physics::OverlapBox(マスの中心, マスの大きさ + 2 × agentRadius, obstacleTag)` を呼び、1つでも見つかったらそのマスを通れなくする | `"Wall"` タグの Static な剛体。Sample の壁はこれで足りる |
| 手で登録 | `SetBlocked` / `SetBlockedBox` | 池(`Water` は当たり判定が無い)・見えない柵・物理を付けていない障害物 |
| 地形から自動 | `BakeFromTerrain`: 各マスの `GetNormalAt` の傾き(`normal.y < cos(maxSlope)`)と、隣のマスとの高さの差(`maxStepHeight` 超え)で通れなくする | Sample3D の `Hills` の急な斜面 |

- **障害物を太らせる(agentRadius)**: マスの箱を「マスの大きさ + 2 × 半径」にして調べるので、**壁から半径ぶん離れたマスも通れなくなります**。こうすると、敵の体が壁にこすらない道ができ、「1マスの点として歩けば体が壁に当たる」問題が起きません。落とし穴: 半径が大きいと、**狭い通路が全部塞がります**(敵の体が入らないので正しいが、「通れるはずなのに回り道する」と見える)。
- **焼くタイミングの落とし穴**: `Physics::OverlapBox` は「出したばかりのオブジェクトは次のフレームから見つかる」(`Physics.h`)。**`NavGridComponent::Init` の中で焼くと、壁がまだ見つからず、全部通れる格子になります。** 最初の `Update`(シーンの全オブジェクトが置かれた後の最初のフレーム、実装時に「何フレーム目なら見つかるか」を実測して決める)で1回だけ焼きます。焼き終わるまで、歩く側は止まって待ちます(`IsBaked()`)。
- **2D(XY平面)**: 壁の `BoxRigidbodyComponent` は奥行き(Z)が100あり、敵は Z=0 にいます。`OverlapBox` の箱は Z を 10 くらいの薄さにする(Zが合わないと見つからない)。2D では `baseHeight` / 高さ関連の設定は使いません。
- **回転した壁**: `OverlapBox` の「問い合わせの箱」は回転しませんが、**当たる相手の壁は回転していても正しく判定されます**(PhysXが形で調べる)。マスごとに調べる方式のおかげで、壁の向きを気にしなくてよいのが、「壁の大きさ・位置から計算で塗る」方式より優れる点です。
- **3Dの高さの窓**: 壁の判定の箱は、縦(Y)を「地面の高さ + `maxStepHeight` から `agentHeight` まで」の窓にします。地面そのもの(`Ground` の大きな箱)や低い段差を障害物にしないためです(`Ground` にはタグが無いので `"Wall"` では見つからないが、箱の窓を決めておけば他のタグを使う時も安全)。

**(b) 格子の大きさ・解像度・メモリ・生成の時間**
- マスの数 = (大きさ ÷ `cellSize`)の2乗。例: Sample3D で 6000 x 6000 を `cellSize` 50 にすると 120 x 120 = **14,400マス**。9000 四方なら 180 x 180 = 32,400マス。
- メモリ: 格子は 1マス 約5バイト(通れるか1バイト + 高さ4バイト)。32,400マスで約160KB。A\* の作業用配列(距離・親・状態で約9バイト/マス)は**格子ごとに1組だけ**で、敵ごとには持たない(敵が100体いても増えない)。
- 上限は `maxCells`(512 x 512 = 262,144 マス)。超える設定は、`cellSize` を自動で大きくして出力ウィンドウに知らせる。
- `cellSize` のおすすめ: 敵の半径と同じくらい(Sample3D の敵は半径40 → 50、Sample2D は一辺100 → 50)。粗いほど速いが、細い道が塞がる。
- 生成の時間: `BakeFromPhysics` はマスの数だけ `OverlapBox` を呼びます(14,400回)。**1回数十ミリ秒程度と見込みますが、これは推測で、実装時に測ります**。読み込み時に1回だけなので、起動時のわずかな引っかかりで済みます。重ければ「壁を全部1回の大きな箱で集めて、その近くのマスだけ調べる」最適化を足せます(未確定の点)。
- 格子は**有限の範囲**です。Sample3D の世界は広い(地面は10万四方)ので、格子の外にいる敵は「経路なし → 止まる」になります。Player を中心に格子が動く方式は、今回は作りません(未確定の点)。

**(c) 経路の滑らかさ(直線化)**
- A\* の結果は、マスを1つずつたどる点の列なので、そのまま歩くとカクカク(8方向だけ)になります。**`smooth = true` の時は、「今の点から、見通せる限り先の点へ飛ばす」処理をします。** 見通せるかは `NavGrid::HasLineOfWalk`(2点の間のマスを順に調べ、通れないマスが1つでもあれば不可)で調べます。
- `Physics::Raycast` を使わない理由: ① 太らせた障害物(`agentRadius`)の分が自動で入る(Raycastは細い線なので、壁の角をかすめる道を許してしまう) ② 格子の配列を読むだけなので、とても速い(物理を呼ばない) ③ 地形の急な坂も同じ仕組みで避けられる。
- 落とし穴: 「壁の角を斜めに抜ける」移動は禁止します(A\* の斜め移動は、隣り合う2マスの両方が通れる時だけ許す)。許すと、角で壁にめり込む。

**(d) 経路を歩く Component(`PathFollowComponent`)**
- `MoveComponent` を継承します(`ChaseComponent` と同じ形)。目的地は `GetOffsetToDestination` から求め、動かすのは `ApplyVelocity` なので、「Dynamic は `SetVelocity`、Kinematic は位置を直接、XZ では重力のYを残す」という既存の決まりがそのまま効きます。**`RigidbodyComponent::SetVelocity` を直接呼ぶ必要はありません**(`MoveComponent` が呼ぶ)。
- 流れ(`Update`):
  1. 目的地が無い・非アクティブ・`StopDistance` より近い → 止まる(`ChaseComponent` と同じ)
  2. 再探索のタイマーが0になる、または**目的地が前回の探索から1マス以上動いた**、または**経路が無い**、または**格子の `GetVersion()` が変わった**、または**詰まった(後述)**時に、`FindPath` を呼ぶ
  3. 経路の「今向かっている点」までの距離が `waypointRadius` より小さければ、次の点へ進む。最後の点なら目的地そのものへ
  4. 今向かっている点へ向かう速度(長さ `speed`)を作って `ApplyVelocity`
- **再探索の頻度**: 既定0.5秒(`VisionComponent` と同じく ±20% ばらして、同じフレームに集中させない)。Player が止まっていれば、目的地のマスが変わらない限り再探索しない。
- **詰まったら**: 1秒間、進んだ距離が `speed × 0.2 × 1秒` に満たなければ「詰まった」とみなして再探索する。(敵どうしの押し合い・段差で止まった時の保険)
- **到着の判定**: 最後の点は目的地そのもの(Player の位置)にして、`StopDistance` は `MoveComponent` の決まりに従う。途中の点は `waypointRadius` で判定する。
- **目的地が通れないマスの中**(Player が壁際に立っている・太らせた領域の中): `FindPath` が近くの通れるマスに寄せてから探す(`FindNearestWalkable`)。寄せられなければ(格子の外など)経路なしになる。
- **経路が無い時**: **止まる**(ユーザーの決定。設計のおすすめは「まっすぐ追う」だったが、止まるにした)。止まっている間も探し直し、目的地や障害物が変わって行けるようになったら動き出す。
- **`ChaseComponent` との関係**: **`ChaseComponent` は変えません。** 経路探索の版を別のクラスとして作り、Sample の `EnemyManager` で `ChaseComponent` を `PathFollowComponent` に置き換えます(`AccelerateChaseComponent` / `HomingComponent` はそのまま使える)。`PathFollowComponent` は `MoveComponent` の子なので、`SetTarget` / `SetSpeed` の呼び方は同じです。

**(e) デバッグ表示**
- `NavGridComponent::SetDebugDrawEnabled(true)`: **通れないマスの輪郭**を赤い線で描く。`Mesh` は作った後に変えられないので、(焼き直す度に)通れないマスの線データをまとめて1つの `Mesh`(LINELIST)に作り直す。数が多い時の重さに注意(焼き直しは稀なので許容)。
- `PathFollowComponent::SetDebugDrawEnabled(true)`: 今の経路を線でつなぐ。線は「単位の長さ1の線」の `Mesh` を1つ共有し、点と点の間を `Transform` の `scale`(長さ)と `rotate`(向き)で描く(`VisionComponent` の扇形と同じ工夫)。敵が100体いると線が増えるので、ONの時だけ描く。
- 色: 格子の障害物=赤、経路=緑(`VisionComponent` は見えている=赤、見えていない=緑)。

**(f) 非同期/分割して探索する必要があるか**
- **まず作らない提案です。** A\* を 14,400マスの格子で最悪まで調べても、1回は Release で0.1〜0.5ミリ秒程度と見込みます(推測。実装時に `GetLastExpandedCount()` と時間で測る)。敵が100体、0.5秒おきなら、1秒あたり200回 = 1フレーム(60fps)あたり約3回で、許容範囲です。
- ただし**同じフレームに集中すると重くなる**ので、①再探索のタイミングを ±20% ばらす ②`maxExpandedCells` で1回の上限を決める、の2つで抑える。
- それでも足りなければ(未確定の点)、後から「1フレームに探索する回数の上限(順番待ちの列)」や「探索を途中で止めて次のフレームに続ける(分割A\*)」を足す。マルチスレッドは、PhysXの問い合わせ(`OverlapBox`)を使う焼きの部分が絡むので、**やらない**。

**(g) 3Dの高さ(Terrain の起伏・段差・坂)**
- 3D(`plane == XZ`)では、格子は「平面上の升目 + 各マスの地面の高さ」という**2.5次元**の表現です。洞窟・橋の下をくぐる、のような「同じXZに複数の床がある」場所は扱えません(今のサンプルには無い)。
- **高さの出所**: 地形があるマス(`terrain.Contains(x, z)`)は `TerrainRenderer::GetHeightAt`、地形が無いマスは `baseHeight`(Sample3D の `Ground` なら `-50`)。
- **傾き**: マスの中心と4つの角の `GetNormalAt` を調べ、一番急なものが `maxSlopeDegrees` を超えたらそのマスを通れなくする(中心1点だと、細い崖を見逃す)。
- **段差**: 隣り合うマス(8方向)の高さの差が `maxStepHeight` を超えたら、その2マスの行き来を禁止する。(マス自体は通れるまま。全部通れなくすると、緩やかな斜面まで塞がる場合があるため、実装時に「マスを塞ぐ」か「行き来を禁止」かを選ぶ)
- 経路の点の `y` は覚えた地面の高さ。ただし、動かすのは `ApplyVelocity`(XZ平面)なので、**Y方向は重力と地形のコライダーに任せる**(敵が丘の斜面を上る時は、物理が足元を押し上げる)。`y` は主にデバッグの線のため。
- 落とし穴: 地形の分割(Hills は 4000 / 128 ≒ 31)より `cellSize` が大きいと、細い谷・尾根を見逃す。`cellSize` 50〜100 なら、丘の起伏は十分に拾える。
- 水面(`Water`)は当たり判定が無く、歩けてしまう。池に入らせたくない時は `SetBlockedBox` で手で登録する(`Water` の位置と大きさは `CreateWater` の引数)。

**(h) 動く障害物(他の敵・Player)の扱い**
- **他の敵と Player は、格子に入れません**(`obstacleTag` の `"Wall"` だけが対象)。敵どうしは物理(Dynamic の剛体)で押し合うので、完全には避けられないが、動いていれば自然にばらける。Player は目的地なので、障害物にしてはいけない。
- 「他の敵を避ける」動き(群れの分離)は、経路探索とは別の機能で、今回は作りません。気になる時は、`PathFollowComponent` の速度に「近くの敵から離れる向き」を足す案(`Physics::OverlapSphere` で調べる)を、将来の拡張として残します。
- **動く壁(Kinematic の壁・ドア)**: 壁のオブジェクトに `"Wall"` タグを付けておくと、焼き直しで反映される。`NavGridComponent::SetAutoRebakeInterval(1.f)` で1秒ごとに焼き直すか、動かした側が `RequestRebake()` / `BakeRegionFromPhysics(中心, 大きさ)` を呼ぶ。範囲だけの焼き直しは軽い。格子が変わると `GetVersion()` が増え、歩く側が経路を引き直す。
- 動く壁の位置は、物理の「前の `Physics::Update` の位置」を使う(1フレーム遅れる)ので、動いた直後の焼き直しは1フレーム遅れて反映される。

**(i) 関数の宣言案** は上の「関数(宣言案)」のとおり。

**(j) 段階分け**
| 段階 | 内容 | Sample |
|---|---|---|
| 1 | `NavGrid`(2D・手動+物理から自動)+ `PathFinder`(A\* + 直線化)+ `NavGridComponent`(焼き・デバッグ表示)+ `PathFollowComponent` | Sample2D の敵が壁を避けて Player を追う |
| 2 | 3D対応: `BakeFromTerrain`、高さ・傾き・段差、`plane == XZ` | Sample3D の敵が Wall と Hills の急斜面を避ける |
| 3 | 動く障害物: `BakeRegionFromPhysics`、`SetAutoRebakeInterval`、詰まり検知の調整 | (任意)動く壁を1つ置いて確認 |
| 4(任意) | フローフィールド: `NavFlowField`(目的地から全マスの距離を一括で計算し、各敵は自分のマスの隣で一番小さい所へ進む)。敵が多い時の軽さが目的 | 敵が増えた時に切り替え |

段階1だけでも、Sample2D の敵が壁を避けるところまで動かせます。

#### 両サンプルへの影響(変えるEngineファイル。了承が必要)
- **新規のみ**: `Engine/Behavior/NavGrid.h/.cpp`、`PathFinder.h/.cpp`、`NavGridComponent.h/.cpp`、`PathFollowComponent.h/.cpp`、`Engine.vcxproj`、`Engine.vcxproj.filters`
- **既存のEngineファイルは変えません**(`MoveComponent` / `ChaseComponent` / `Physics` / `Terrain` / `TerrainRenderer` は読むだけ)。`Shader*.hlsl`・`PhysX.props` も変えません。
- Engineに新しいファイルを足すだけなので、**何もしなければ両サンプルの動きは変わりません**(既存の `ChaseComponent` の敵は今まで通り)。変わるのは、Sampleが `PathFollowComponent` を使うようにした時だけ。
- Sample2D: 敵が壁を避ける(見た目の動きが変わる)。Sample3D: 同じ。**ゲームの難しさが変わる**(壁の陰に隠れても追われる)ので、遊んで確認してもらう。

#### 使うSampleのコード
```cpp
// ─── Sample2D/Game/Objects/Navigation.cpp(新規)───
// 工場関数: 格子を置く。Wall をすべて置いた後の、Scene::Init の最後の方で呼ぶ
NavGridComponent* CreateNavigation(Scene& scene) {
    GameObject* nav = scene.CreateObject("Navigation");
    NavGridSettings settings;
    settings.origin = { -1500.f, -1000.f, 0.f };   // 画面と壁をおおう範囲の左下
    settings.size   = { 3000.f, 2000.f };
    settings.cellSize = 50.f;
    settings.plane  = MovePlane::XY;
    settings.agentRadius = 50.f;                   // 敵は一辺100の四角
    return nav->AddComponent<NavGridComponent>(settings);   // Wall は最初の Update で自動で読まれる
}

// ─── Sample2D GameScene.cpp: 置くだけ ───
GameObject* player = CreatePlayer(*this);
CreateWall(*this, { -250.0f, 150.0f }, { 40.0f, 300.0f });
CreateWall(*this, { 400.0f, -350.0f }, { 300.0f, 40.0f });
NavGridComponent* nav = CreateNavigation(*this);
CreateEnemySpawner(*this, player, nav);            // 引数に nav を足す(target と同じ渡し方)

// ─── Sample2D EnemyManager.cpp: ChaseComponent → PathFollowComponent に置き換える ───
// SetupEnemy: 動き
//   今:  obj->AddComponent<ChaseComponent>();
//   後:  obj->AddComponent<PathFollowComponent>();
// Spawn: 追いかける相手・速さ・格子を渡す
//   今:  ChaseComponent* chase = enemy->GetComponent<ChaseComponent>(); chase->SetTarget(target); chase->SetSpeed(enemySpeed);
//   後:  PathFollowComponent* path = enemy->GetComponent<PathFollowComponent>();
//        path->SetTarget(target); path->SetSpeed(enemySpeed); path->SetNavigation(navigation);
// ApplySpeedToEnemies: GetComponent<ChaseComponent>() → GetComponent<PathFollowComponent>()

// ─── Sample3D EnemyManager.cpp: 同じ置き換え + 平面 ───
//   path->SetPlane(MovePlane::XZ);                 // SetupEnemy の chase->SetPlane(MovePlane::XZ) と同じ位置
// ─── Sample3D Objects/Navigation.cpp ───
//   settings.origin = { -3000.f, 0.f, -3000.f };  settings.size = { 9000.f, 9000.f };  settings.cellSize = 50.f;
//   settings.plane = MovePlane::XZ;  settings.baseHeight = -50.f;  settings.agentRadius = 40.f;
//   nav->GetComponent<NavGridComponent>()->SetTerrain(hillsRenderer);   // Hills の TerrainRenderer を渡す(CreateHills が返す GameObject から取る)
```
- 対象: 両サンプルの `EnemyManager.h/.cpp`(`ChaseComponent` の参照を置き換える)、`EnemySpawner.h/.cpp`(工場関数の引数に `nav` を足す)、`GameScene.cpp`(`CreateNavigation` を1行足す)、新しいファイル `Navigation.h/.cpp`(各サンプル)。
- `EnemyManager` の既存のしくみ(プール、CSVの速さの反映、ウェーブ)は**そのまま**。置き換えるのは `ChaseComponent` を使う3か所だけ。
- `Sample3D` の `Hills` は地形を `TerrainRenderer` で持つので、`CreateHills` が `TerrainRenderer` を取り出せる形(返り値の GameObject の `GetComponent<TerrainRenderer>()`)で `CreateNavigation` に渡す。

#### 依存する機能
5.6 物理の問い合わせ(`Physics::OverlapBox`)、`MoveComponent`(`ChaseComponent` の親)、6.7 地形(`TerrainRenderer::GetHeightAt/GetNormalAt/Contains`。段階2のみ)、5.1 計算系(`Math::Distance` など)

#### 未確定の点(ユーザーに聞きたいこと)
- (a) 方式は「格子+A\*+直線化」でよいか(ナビメッシュは見送り)。フローフィールド(段階4)は後回しでよいか
- (b) 新しいファイルを `Engine/Behavior/` に置くか、新フォルダ `Engine/Navigation/` を作るか(5.2)
- (c) 敵が経路なしの時の既定は「まっすぐ追う」でよいか(それとも止まる) → **決定: 止まる**
- (d) 格子は固定の範囲でよいか(Player を中心に動く方式は作らない)
- (e) 焼く時間が長い時の最適化(壁だけを先に集める)を、最初から入れるか、測ってから入れるか
- (f) 他の敵を避ける動き(群れの分離)は、今回は作らなくてよいか
- (g) Sample3D の池(`Water`)を、通れない障害物として登録してよいか(今は敵が水の中を歩ける)

---

### 7.2 LOD

**状態: 段階1だけ実装済み(段階2・3・4は未着手。測ってから決める)。** 下の設計のうち、段階1で決めたこと・実測を先にまとめます(使い方は README §37)。

#### 段階1 実装したこと・決めたこと

- **新しいファイル**: `Engine/Render3D/Lod.h/.cpp`(`LodSettings` = `drawDistance` / `shadowDistance` / `outlineDistance` の3つだけ。`Lod::GetCameraDistance` / `IsBeyond` / `SetEnabled` / `SetGlobalScale`)、`Engine/Render3D/RenderStats.h/.cpp`(カウンタ)。`Engine.vcxproj` / `.filters` に登録。
- **既存のEngineファイルの変更**: `MeshRenderer.h`・`ModelRenderer.h/.cpp`・`TerrainRenderer.h/.cpp`(`SetLod` / `GetLod` と距離の判断)、`Mesh.cpp`・`SkinMesh.cpp`・`WaterRenderer.cpp`(描画命令の直前に数える)、`GameEngine.cpp`(`RenderStats::BeginFrame()` を `Visibility::BeginFrame()` の次に1行)。`Mesh.h` / `SkinMesh.h` は変えていません。`Shader*.hlsl`・`PhysX.props` も変えていません。
- **作らなかったもの**: `SelectLevel` / `meshDistances` / `hysteresis`(段階2用。宣言案から外した)、任意の `ModelAnimationComponent::SetUpdateInterval` と `SpriteRenderer::SetMaxDrawDistance`(Sample側の `SetEnabled(false)` で代わりにできる)。
- **距離の基準**: `Image::GetCamera3D().position`(揺れ込み)から、物の球の表面まで(「中心までの距離 − 半径」)。ちょうど設定の距離なら省かない(超えたら省く)。半径は `MeshRenderer` は頂点の一番遠い点×拡大率の最大、`ModelRenderer` は止まっている姿の半径×1.5(画面判定と同じ)、`TerrainRenderer` は地形の箱を囲む球。カメラが球の中なら必ず「近い」。
- **`drawDistance` と画面外の判定の順**: 距離が先。遠ければ描かずに終わり(`LodCulled` に数える)、近ければ今までどおり画面の外かを調べる(`Visibility::GetCulledCount` に数える)。`drawDistance` で省いた物は枠線も描かない。枠線だけを省く時は `DrawOutline` を呼ばないので、ステンシルの印は付かず、状態は壊れない(確認済み)。
- **影**: `shadowDistance` は `DrawShadow` の最初で調べる(太陽のパスもスポットのパスも同じ距離)。`Shadow::GetDrawnCount` は「`DrawShadow` を呼んだ数」なので、距離で省いても減らない。実際に描いた影の数は `RenderStats::GetShadowDrawCount` を見る。
- **カウンタの数え方**: 描画命令は「GPUに出した Draw の数」。**枠線は1つの物で3つ**(印を付ける・枠線・印を消す)。三角形は `indexCount / 3`(インデックスなし=`vertexCount / 3`)、線のメッシュは0。種類別(通常/影/枠線)と合計、`LodCulled`(描画・影・枠線を別々に数えた合計)。前のフレームの数を返し、`GameEngine::Run` では `Visibility::BeginFrame()` の次(影のパスより前)で数え直す。
- **全体の切り替え**: `Lod::SetEnabled(false)` で、すべての設定を無視(LODなしと比べる確認用)。`Lod::SetGlobalScale` で距離を倍率でまとめて調整。距離が 0 の物は行列も読まない(OFFの時の追加の計算は `if` 1つ)。
- **Sample3D**: `Objects/WallField.h/.cpp`(`CreateWallField`。柱 80 x 300 x 80 を 15 x 15 本、間隔 300、中心 `(0, 0, -6000)`。経路探索の格子の外=敵の動きに影響しない。柱に枠線と `drawDistance 6000` / `shadowDistance 2500` / `outlineDistance 1500` を設定)。`HUDController` に `Draw: 合計 (Shadow, Outline)` と `Tri: ○ LodCulled: ○ LOD: ON/OFF` の2行。LODのON/OFFは、`GameScene.cpp` の `lodEnabled`(constexpr)を書き換えて再ビルド(キー操作は足していない)。Sample2D は変更なし。
- **確認したこと**(WARP=ソフトウェア描画のスクラッチテスト。Releaseの `Engine.lib`、116件): ①LODを設定しない絵が、変更前(HEAD)のEngineの絵と**ピクセル完全一致**(画素データのハッシュが同じ。影・枠線(壁越しを含む)・球・半透明・地形入りの場面)。巨大な距離を設定した絵、`SetEnabled(false)` で厳しい距離を設定した絵も、同じ絵 ②`drawDistance`: 超えると描かれず(赤い画素0)、内側は同じ絵。命令0・三角形0・`LodCulled` 1 ③`shadowDistance`: 超えた物は影のパスに出ず、床の絵が「影なし」と画素単位で一致。内側は影あり ④`outlineDistance`: 超えた物は枠線の緑が0で、「枠線の太さ0」の絵と一致。近い物は枠線あり。どちらも(壁越しでも)ステンシルは全画素0に戻る ⑤境目: ちょうどの距離は省かない、`nextafterf` で1つ小さくすると省く。半径を引く・カメラが球の中・不均一な拡大(最大を使う)・カメラの位置が変わると距離が変わる(揺れ込み)・`target` は無関係・`SetGlobalScale` ⑥`RenderStats`: 立方体3個+球2個の命令数・三角形数(12枚/768枚)が一致、影・枠線の内訳(枠線は3命令)、前のフレームの値、空のフレームは0、地形の三角形、スキンのモデル(`TestSkin.fbx`)と普通のモデル(`TestParts.fbx`)の命令・影・枠線 ⑦225〜900本の柱で、期待する命令数(柱ごとに独立に計算した値)と一致 ⑧`TerrainRenderer` の `drawDistance` / `shadowDistance`(境目、カメラが球の中)、画面外の判定は今までどおり。
- **確認していないこと**: 実際のSample3Dを動かした時の `Draw` / `Tri` の数と、遊んだ時の見え方(柱が近づくと順に出てくる様子・ポップの目立ち方)、実機のGPUでの速さ。

#### 段階1 の実測(WARP=ソフトウェア描画 480 x 270、Release。時間は参考値)

柱は WallField と同じ(80 x 300 x 80、影・枠線つき)。カメラは南向き、手前(Z=0)から。

| 場面 | 設定 | 通常 | 影 | 枠線 | 命令合計 | 三角形 | ms/フレーム |
|---|---|---|---|---|---|---|---|
| 15 x 15 = 225本 | OFF | 225 | 225 | 675 | 1125 | 13,500 | 11.5 |
| | drawDistance 6000 | 116 | 225 | 348 | 689 | 8,268 | 6.9 |
| | shadowDistance 2500 | 225 | 0 | 675 | 900 | 10,800 | 9.2 |
| | outlineDistance 1500 | 225 | 225 | 0 | 450 | 5,400 | 4.7 |
| | 3つとも | 116 | 0 | 0 | 116 | 1,392 | 1.7 |
| 30 x 30 = 900本 | OFF | 900 | 900 | 2700 | 4500 | 54,000 | 42.4 |
| | drawDistance 6000 | 418 | 900 | 1254 | 2572 | 30,864 | 23.7 |
| | shadowDistance 2500 | 900 | 42 | 2700 | 3642 | 43,704 | 34.4 |
| | outlineDistance 1500 | 900 | 900 | 12 | 1812 | 21,744 | 18.4 |
| | 3つとも | 418 | 42 | 12 | 472 | 5,664 | 6.7 |

- カメラが列に近づく(15 x 15、Z=-3000)と、3つとも設定しても命令は 1125 → 375(13.2 → 7.5 ms)。LODの効きは位置関係で大きく変わる。
- 三角形の影響(影・枠線なし、225個): 立方体 2.7 ms、球4x6 2.9 ms、球16x24(合計17万枚)6.1 ms、球48x72(合計155万枚)22.6 ms。

**設計書の見立てとの比較**
- 「効くのは、描画命令の数 → 影 → 枠線 → スキンアニメ」: **命令の数が時間を決める**(1命令約10マイクロ秒。WARPでの値)という点は見立てどおり。ただし**枠線は1つの物で命令3つ**なので、枠線を付けた物が多い時は、枠線を省くのが一番大きく効いた(225本で 11.5 → 4.7 ms)。影は1つで命令1つなので、小さめ(11.5 → 9.2 ms)。**「枠線は影より効く」は、見立てと違った点**(枠線を付けた物の数による)。スキンアニメは今回測っていない(骨の行列の送りは、スキンのモデルの命令にだけ関係する)。
- 「三角形の数は今のSampleでは小さい」: 見立てどおり。立方体〜768枚の球の差は数ms以内で、効き始めるのは合計が数十万枚を超えてから。Sample3D の丘は 32,768枚なので、段階3(地形チャンク)の価値は小さいと見ている。
- **段階2・3に進む価値**: 今のSampleでは**低い**(三角形が少ない物が中心で、命令の数と枠線・影が支配的)。FBXの別ファイルが用意できる、または球・FBXで合計数十万枚を超える場面が出てから決めるのがよい。物が数百個ある場面では、インスタンス描画(8章の未確定事項 9番)の方が効く可能性が高い。

**目的**: 遠い物を軽く描く(または描かない)ことで、物が増えても速さを保てるようにする。ただしこのEngineは**「何を軽くすると効果が大きいか」を先に知る必要がある**ので、最初に「測る仕組み」を作り、効果が大きい順に段階を踏む。

#### このEngineで何が重いか(現状から見た見立て)

| 重くなりうる所 | 現状 | LODで軽くできるか | 見立て |
|---|---|---|---|
| **描画命令の数** | `Mesh::Draw` は物ごと。描く度に定数バッファ(`b0` / `b2` / `b3`)・ライト・影のセットを送る。`ModelRenderer` はパーツごと | 遠い物を**描かない**ことで減る。メッシュを軽くしても減らない | **一番効く**(物が多い場面) |
| **影のパス** | `castShadow` の物は、距離に関係なくもう1回 `DrawDepth` | 遠い物は影を落とさない | **効く**(物の数に比例) |
| **枠線** | `outlineWidth > 0` の物は、本物のあとに複数回描く | 遠い物は枠線を描かない | 枠線を付けた物が多い時に効く |
| **スキンアニメ** | 描く度(と影のパスでも)に、骨の行列を最大64個計算して送る。`ModelAnimationComponent` は毎フレーム全パーツを更新 | 遠い物は更新の間隔を空ける/止める/描かない | キャラクターが多い時に効く |
| **三角形の数** | 立方体=12枚、球(16x24)=約760枚、地形=32,768枚、FBXは物による | メッシュの切り替えで減る | **今のSampleでは小さい**(GPUは数万枚を軽く描ける)。大きなFBXが多数ある時だけ効く |
| **スプライト** | 1枚ずつ描く(Sample3D の敵はビルボード) | 遠いビルボードは描かない | 敵が多い時 |

**正直な話**: 「LOD=メッシュを軽く差し替える」が一般的なイメージですが、**このEngineの現状では、メッシュの三角形を減らす効果は小さい**可能性が高いです(立方体の壁・小さな球が中心で、三角形の合計が少ない)。それより**「描かない」「影を落とさない」「枠線を描かない」「アニメを止める」**の方が、手間が小さく効果が大きい見込みです。ただし、これは**予想**なので、段階1で「描画命令の数・三角形の数のカウンタ」を作って測ってから、段階2・3に進むか決めるのを強くおすすめします。

#### 方式の比較とおすすめ

| 方式 | 仕組み | 良い点 | 悪い点 | 難しさ |
|---|---|---|---|---|
| **A. 距離で省く**(描画・影・枠線・アニメの距離カット) | 機能ごとに「この距離より遠いならやらない」を決める | 一番かんたん。効果が大きい。見た目の違和感が少ない(遠いと小さい) | 急に消える(ポップ)。距離の決め方を調整する必要 | ◎ |
| **B. メッシュの切り替え** | 距離に応じて、頂点の数が違う複数のメッシュを切り替える | 三角形が減る。形は残る | 用意が大変(手で作る or 自動削減は難しい)。切り替えでちらつく。メモリが増える | △ |
| **C. 地形のチャンク分け** | 地形を区画に分け、区画ごとに細かさを変える。見えない区画は描かない | 大きな地形で効く | 区画の継ぎ目が割れる(対策が必要)。作りが大きい | △ |
| **D. 画面上の大きさで決める** | 距離ではなく「画面に映る大きさ」で段階を決める | カメラの視野(FOV)・物の大きさに強い | 計算が少し複雑 | ○ |
| E. 頂点削減の自動生成 | Engineが頂点を減らした軽いメッシュを作る | 手で用意しなくてよい | UV・法線・骨の影響度を壊しやすい。難しい | ×(今回は見送り) |
| F. インスタンス描画(同じ物をまとめて1回で描く) | 同じメッシュの物を1回の命令で大量に描く | 描画命令の数が大きく減る | LODとは別の機能。シェーダー・定数バッファの作りが変わる大工事 | ×(別の機能として提案) |

**おすすめ: 段階分けで、A → (測ってから)B → (測ってから)C の順に進める。**
- **段階1(A + 測る仕組み)**: 距離で「描かない/影を落とさない/枠線を描かない/アニメを止める」を決められるようにし、**描画命令の数・三角形の数を数えるカウンタ**を作る。シェーダーも `Mesh` も変えない(描く物・描く回数を変えるだけ)。何もしなければ今まで通り(既定OFF)。
- **段階2(B)**: `MeshRenderer` / `ModelRenderer` に「距離に応じたメッシュ(モデル)の切り替え」を足す。**材料は手で用意する**(`CreateSphere(rings, segments)` の分割数違いは自動で作れるので、球から始めるのが簡単。FBXは別ファイルを用意)。ちらつき防止は**ヒステリシス**(入る距離と出る距離をずらす)。
- **段階3(C)**: 地形のチャンク分け。継ぎ目の割れは**スカート**(各区画の縁に下向きの壁を足す)で隠す、という一番かんたんな方法にする。
- **段階4(任意)**: ディザによるフェード(切り替えを滑らかにする)。`Shader3D.hlsl` / `Shader3DSkin.hlsl` を変える必要があるので、別途ユーザーの了承が必要。
- E(自動削減)とF(インスタンス描画)は、今回は作りません。**Fは、物が数百個ある場面ではLODより効果が大きい可能性が高い**ので、LODとは別の機能として将来提案します(8章の未確定事項 9番)。

#### ファイル案
新規:
- `Engine/Render3D/RenderStats.h/.cpp` : 描画命令の数・三角形の数のカウンタ(`Visibility::GetCulledCount` と同じ流儀)
- `Engine/Render3D/Lod.h/.cpp` : `LodSettings`(距離の設定)と、距離から段階を選ぶ関数(ヒステリシス込み)。画面に出ない計算だけなので、単独でテストしやすい
- `Engine.vcxproj` と `Engine.vcxproj.filters` に登録(フィルター `Render3D`)

既存の変更(**ユーザーの了承が必要**):
- `Engine/Render3D/MeshRenderer.h` : 描画・影・枠線の距離カット、段階2でメッシュの切り替え
- `Engine/Render3D/ModelRenderer.h/.cpp` : 同じ(段階2でモデルの切り替え)
- `Engine/Render3D/TerrainRenderer.h/.cpp` : 描画・影の距離カット、段階3でチャンク分け
- `Engine/Render3D/Terrain.h/.cpp` : 段階3で、区画ごと・細かさごとのメッシュを作る関数 `BuildChunkMesh` を足す
- `Engine/Render3D/Mesh.h/.cpp` / `SkinMesh.h/.cpp` : `Draw` / `DrawDepth` / `DrawOutline` の中でカウンタを増やす(1〜2行ずつ)
- `Engine/Core/GameEngine.cpp` : `RenderStats::BeginFrame()` を1行足す(`Visibility::BeginFrame()` の隣)
- `Engine/Animation/ModelAnimationComponent.h/.cpp` : (任意)`SetUpdateInterval(seconds)` を足す。Sample側だけで `SetEnabled(false)` で止める方法もある(下の設計のポイント(e))
- `Engine/Render/SpriteRenderer.h/.cpp` : (任意)`SetMaxDrawDistance` を足す(3Dの世界に立てたビルボードの敵用)
- `Shader*.hlsl` / `PhysX.props` は**段階1〜3では変えない**

#### 関数(宣言案)
```cpp
// ─── Engine/Render3D/Lod.h ───
// 距離の設定。0(または空)は「その機能は使わない=今まで通り」
struct LodSettings {
    float drawDistance    = 0.f;          // これより遠い物は描かない(0=いつも描く)
    float shadowDistance  = 0.f;          // これより遠い物は影を落とさない(0=いつも落とす)
    float outlineDistance = 0.f;          // これより遠い物は枠線を描かない(0=いつも描く)
    std::vector<float> meshDistances;     // 段階2: LOD0→1、1→2... に切り替える距離(昇順)。空=切り替えない
    float hysteresis = 0.1f;              // 切り替えの境目の幅(距離の±10%)。ちらつき防止
};

namespace Lod {
    // カメラ(Image::GetCamera3D().position)から、ワールド座標 position までの距離
    float GetCameraDistance(XMFLOAT3 position);
    // 距離から段階を選ぶ。currentLevel を渡すと、境目の手前ではそのままにする(ヒステリシス)
    int SelectLevel(float distance, const std::vector<float>& distances, int currentLevel, float hysteresis);
}

// ─── Engine/Render3D/RenderStats.h ───
namespace RenderStats {
    // 前のフレームの数(HUDに出して確認する。Visibility::GetCulledCount と同じ流儀)
    int GetDrawCallCount();        // 本番の描画で出した描画命令の数(枠線を除く)
    int GetShadowDrawCount();      // 影のパスで出した描画命令の数
    int GetOutlineDrawCount();     // 枠線の描画命令の数
    int GetTriangleCount();        // 描いた三角形の数(本番+影+枠線)
    int GetLodCulledCount();       // 距離で省いた数(描画・影・枠線の合計)

    // ─── Engine内部用(GameEngineと、Mesh / SkinMesh / 各Renderer だけが呼ぶ) ───
    void BeginFrame();
    void AddDraw(int triangleCount);
    void AddShadowDraw(int triangleCount);
    void AddOutlineDraw(int triangleCount);
    void AddLodCulled();
}

// ─── MeshRenderer / ModelRenderer / TerrainRenderer に追加 ───
void SetLod(const LodSettings& settings);               // 距離の設定をまとめて渡す
LodSettings& GetLod();                                  // 手直し用
// 段階2(MeshRenderer): 距離に応じて切り替える頂点データ(段階0は今の頂点)
void SetLodMeshes(const std::vector<std::vector<MeshVertex>>& lowerQuality);
// 段階2(ModelRenderer): 距離に応じて切り替えるモデル(パーツの名前・親子関係が同じであること)
void SetLodModels(const std::vector<std::wstring>& lowerQualityPaths);
// 段階3(TerrainRenderer): 区画の大きさ(マス単位)と、区画ごとの段階を切り替える距離
void SetChunking(int chunkCells, const std::vector<float>& lodDistances);

// ─── Engine/Render3D/Mesh.h に追加(段階2) ───
// 球の細かさ違いを、まとめて作る。例: CreateSphereLods({ {16, 24}, {8, 12}, {4, 6} })
static std::vector<std::vector<MeshVertex>> CreateSphereLods(const std::vector<std::pair<int, int>>& ringsAndSegments);
```
`LodSettings` の既定はすべて「使わない」なので、**何もしなければ見た目も速さも今まで通り**です。

#### 設計のポイント(落とし穴を含む)

**(a) LOD の段階の持ち方**
- 距離の設定は `LodSettings`(構造体)にまとめ、`MeshRenderer` / `ModelRenderer` / `TerrainRenderer` が1つずつ持つ。`Material` の中に入れない(`Material` は見た目の設定で、シェーダーの定数バッファに変換する形。距離はシェーダーに送らない)。
- 段階は 0(一番細かい)から。`meshDistances = {1500, 4000}` なら、1500未満=段階0、1500〜4000=段階1、4000以上=段階2。`drawDistance` を超えたら描かない。
- `MeshRenderer` は今、コンストラクタで自分専用の `Mesh` を1つ作る。段階2では `Mesh` を段階の数だけ持つ。**物ごとに複数の `Mesh` ができるので、メモリが増える**(壁100個 × 3段階 = 300個)。立方体のように三角形が元々少ない形は、LODしない方が得。

**(b) 切り替えのちらつき防止**
- **ヒステリシス**(段階1〜3で採用): 段階が上がる距離は `d × (1 + h)`、戻る距離は `d × (1 - h)`。`Lod::SelectLevel` が現在の段階を受け取って判断する。物が境目の距離を行ったり来たりしても、ぱっぱっと切り替わらない。
- **フェード**(段階4・任意): 切り替えの間、2つの段階を混ぜて描く(ディザで少しずつ消す)。**`Shader3D.hlsl` / `Shader3DSkin.hlsl` のピクセルシェーダーの変更が要り**(`clip` で市松模様に消す)、フェードの値を送る場所は、`MaterialConstantBuffer::outlineParams.w`(今は未使用)を使える。シェーダーを変えるので、段階1〜3とは別に了承を取る。
- 距離カット(`drawDistance`)の「急に消える」は、フェードをしない限り避けられない。`drawDistance` を**カメラの `farZ`(10000)より少し手前**に決めて、霧や背景で目立たなくするのが現実的。

**(c) LODメッシュの作り方**
- **手で用意する(段階2の方針)**: 
  - 球: `Mesh::CreateSphere(rings, segments)` の分割数を下げるだけ。`CreateSphereLods` で自動に作る。
  - OBJ: 別ファイルを用意して `Mesh::LoadOBJ`。
  - FBX: `Tank.fbx` に対して `Tank_Lod1.fbx` のように別ファイルを用意し、`ModelRenderer::SetLodModels` に渡す。**パーツの名前・親子関係を同じにする**必要がある(`nodeTransforms` とアニメーションがパーツの番号・名前で結びつくため)。読み込み時に名前・数を比べて、違えば出力ウィンドウに知らせて、そのLODは使わない。スキン(骨)の物は、骨の数・並びが同じである必要があり、作る側の負担が大きい。
- **Engineが頂点削減で自動生成する案は見送り**: 頂点を減らす(エッジ崩し)と、UV(画像の貼り方)・法線・骨の影響度が壊れやすく、バグの温床。立方体のような単純な形には不要。
- 画像: LODの段階でも同じ画像(`Resources` で共有)を使うので、メモリは増えない。

**(d) 地形のLOD(段階3)**
- **今の地形は1つの `Mesh`**(32,768枚の三角形、頂点16,641個)。`GetHeights()` の高さのデータがあるので、区画に分ける材料はそろっている。
- 区画: `chunkCells = 32` なら、128 x 128 マスは 4 x 4 = 16 区画。段階0=全部の頂点(33 x 33 = 1,089個)、段階1=1つおき(17 x 17 = 289個)、段階2=3つおき(9 x 9 = 81個)。三角形は 2,048 → 512 → 128枚。区画 x 段階の `Mesh` は、最初に必要になった時に作る(`Model` のパーツのバッファと同じ流儀)。
- 区画ごとの描画の判断: ①区画の球が映っていない(`Visibility`)なら描かない ②映っていれば、区画の中心とカメラの距離で段階を選ぶ(ヒステリシス付き)。**今の `SetFrustumCulling(true)` は地形全体の球1つ**なので、区画にすると「画面外の区画を描かない」が効くようになる。
- **継ぎ目の割れ**: 隣り合う区画の段階が違うと、境目の頂点が合わず、**隙間(クラック)が空く**。対策は2つ:
  1. **スカート(おすすめ)**: 各区画の縁に、下向きの壁(高さ `skirtDepth`)を足す。隙間が見えなくなる。実装がかんたん。段階の差が大きい所で、少し壁が見える場合がある。
  2. 縫い合わせ(隣の区画の段階に合わせて縁のインデックスを変える): 見た目はきれいだが、実装が複雑。見送り。
- **光の継ぎ目**: 法線は、今の `Terrain::BuildMesh` と同じく**元の全体の高さのデータの中心差分**で、細かさに関係なく計算する。同じ場所の法線が段階で変わらないので、光の段差が出ない。画像の貼り方(UV)も位置から決めるので連続する。
- **当たり判定は変えない**: `TerrainRigidbodyComponent` は `Terrain` のデータ(`GetHeights`)から作られ、`TerrainRenderer` のメッシュとは独立。LODで見た目だけが変わる。注意点: 遠い区画(粗い段階)の見た目の高さは、元の地形から少しずれる(最大で、1つおき・3つおきの頂点の間の凹凸ぶん)。**遠くの物が少し浮く/埋まって見える**可能性があるが、遠いので目立たない。
- **効果は小さい**: Hills は 32,768枚(最大でも 131,072枚)なので、地形だけを軽くしても、全体の速さはほとんど変わらないはず。**段階3は、段階1のカウンタで「三角形の数が支配的」と分かった時だけ作る**ことを、強くおすすめします。大きな地形を `castShadow = true` にした時(影のパスでも全体を描く)は、効果が出やすい。

**(e) 影・枠線・光沢などの、機能ごとの距離カット(段階1)**
- `shadowDistance`: `DrawShadow` の最初で、カメラからの距離がこれを超えたら描かない。**落とし穴**: 影の範囲は「カメラの前方の箱」で、光の向きによっては**カメラから遠い物の影が、近くの地面に落ちる**(背の高い物)。`shadowDistance` を短くしすぎると、「遠くの木の影が消える」不具合になる。物の大きさ(半径)を足して判断し、既定は0(OFF)にする。
- `outlineDistance`: `DrawOutline` を呼ぶ前に距離を調べる。遠い物の枠線は、画面上で数ピクセルの太さなので、消えても気付きにくい。
- `drawDistance`: 描かない。物理(当たり判定)は止まらない(`SetFrustumCulling` と同じ決まり)。
- 光沢(`specularStrength`)・縁の光(`rimStrength`)は、シェーダーの中の計算が少し増えるだけで、距離で切っても効果が小さい(描画命令の数は変わらない)ので、**段階1〜3では扱いません**。
- **スキンアニメ**: ①Sample側だけでできる方法 = 遠い物の `ModelAnimationComponent` を `SetEnabled(false)` で止める(止まった姿のまま。画面に映っていて遠い物は、止まって見える) ②Engineに `SetUpdateInterval(seconds)` を足し、遠いほど間隔を空ける(動きがカクつくが、止まりはしない)。①はEngineを変えなくてよい。②は任意の追加。
- スプライト(Sample3D の敵のビルボード): `SpriteRenderer::SetMaxDrawDistance`(任意)で、世界に立てた(`SetWorldSpace(true)`)スプライトを遠くで描かない。または Sample 側で、敵の `Update` から `SetEnabled(false)` / `Visibility` で省く方法もある。

**(f) LOD の判断に使う距離の基準**
- **カメラの位置は `Image::GetCamera3D().position`**(`Visibility` と同じ)。`GameEngine::Run` は `CameraShake::BeginDraw()` の後に描画と影のパスを行うので、**揺れ込みの位置**が読める。揺れの量は小さいので、距離の判定には影響しない。
- **物の位置はワールド行列の位置**(`transform.GetWorldMatrix()` の移動成分)。親子関係のある物も正しい。
- 「物の中心までの距離」だけだと、大きな物(地形の区画・長い壁)は、中心が遠くても手前の端が近い。**判断には「距離 − 半径」を使う**(半径は `Visibility` が使っている `boundingRadius` と同じもの)。
- 3Dカメラの視野(FOV)を狭くする(ズームする)と、遠い物が大きく見えるのに、距離で切り替えると粗いまま。ズームを使う場面では、方式D(画面上の大きさ)の方が正確だが、**既定は距離**(初心者に分かりやすい)。`Lod::GetCameraDistance` に `fovY` の補正を足す案は、未確定の点。

**(g) 効果を測る方法**
- `RenderStats`(段階1の最初に作る): `GetDrawCallCount()` / `GetShadowDrawCount()` / `GetOutlineDrawCount()` / `GetTriangleCount()` / `GetLodCulledCount()`。`Visibility::GetCulledCount()` と同じく**「前のフレームの数」**を返す(HUDが描く時点で今のフレームが途中でも、数が揺れないように)。
- 数える場所: `Mesh::Draw` / `Mesh::DrawDepth` / `Mesh::DrawOutline` と `SkinMesh` の同じ3つの関数の中で、描画命令を出す直前に1行足す(三角形の数は `indexCount / 3` か `vertexCount / 3`)。
- 既存の確認用の関数との関係: `Visibility::GetCulledCount()`(画面外で省いた数)、`Shadow::GetCasterCount()` / `GetDrawnCount()`(影の数)はそのまま使う。`RenderStats` はそれらに**足りない「描画命令の数・三角形の数」**を足すもの。HUD に出す時は、これらを並べて表示する(Sample の HUD の既存のやり方に合わせる)。
- 速さそのもの(フレーム時間)は、`Time::GetDeltaTime()` から。カウンタとフレーム時間を**同じ場面で、LODをON/OFFして比べる**のが基本。

**(h) 関数の宣言案** は上の「関数(宣言案)」のとおり。

**(i) 段階分け**
| 段階 | 内容 | 変えるEngineファイル | Sample |
|---|---|---|---|
| 1 | `RenderStats`(カウンタ)+ 距離カット(`drawDistance` / `shadowDistance` / `outlineDistance`)+(任意)アニメの間隔 | `MeshRenderer.h`、`ModelRenderer.h/.cpp`、`TerrainRenderer.h/.cpp`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`、`GameEngine.cpp`、新規 `RenderStats` / `Lod` | Sample3D の壁・モデル・地形に距離を設定し、HUDにカウンタを出す |
| 2 | メッシュ・モデルの切り替え(ヒステリシス付き)、`CreateSphereLods` | `MeshRenderer.h`、`ModelRenderer.h/.cpp`、`Mesh.h/.cpp` | 球をLOD付きにする。FBXは別ファイルを用意できる場合のみ |
| 3 | 地形のチャンク分け、継ぎ目のスカート | `TerrainRenderer.h/.cpp`、`Terrain.h/.cpp` | `Hills` にチャンクを設定 |
| 4(任意) | ディザのフェード | `Shader3D.hlsl`、`Shader3DSkin.hlsl`、`Material.h`(`outlineParams.w`) | — |

**段階1の後で、一度立ち止まって測る**(カウンタの数と速さを見て、段階2・3が本当に必要かをユーザーが決める)。

#### 両サンプルへの影響(変えるEngineファイル。了承が必要)
- **新規**: `Engine/Render3D/RenderStats.h/.cpp`、`Engine/Render3D/Lod.h/.cpp`、`Engine.vcxproj`、`Engine.vcxproj.filters`
- **既存の変更(段階1)**: `Engine/Render3D/MeshRenderer.h`、`ModelRenderer.h/.cpp`、`TerrainRenderer.h/.cpp`、`Mesh.h/.cpp`、`SkinMesh.h/.cpp`(カウンタ)、`Engine/Core/GameEngine.cpp`(`BeginFrame` の1行)。任意で `Engine/Animation/ModelAnimationComponent.h/.cpp`、`Engine/Render/SpriteRenderer.h/.cpp`
- **既存の変更(段階2・3)**: 上に加えて `Engine/Render3D/Terrain.h/.cpp`
- `Shader*.hlsl` と `PhysX.props` は段階1〜3では**変えない**(段階4のフェードだけ変える)。
- Sample2D: `Shader3D.hlsl` を使う `Mesh` の描画は、デバッグ表示(当たり判定の線)だけ。カウンタを数える行が増えるだけで、**見た目も動きも変わらない**。距離の設定をしなければ、LODは何もしない(既定OFF)。Sample3D: 設定した物だけが変わる。**段階2・3は見た目が変わる**(遠い物が粗くなる)ので、遊んで確認してもらう。

#### 使うSampleのコード
```cpp
// ─── Sample3D: 壁(CreateWall)に距離を設定する ───
auto* meshRenderer = wall->AddComponent<MeshRenderer>(Mesh::CreateCube());
meshRenderer->GetLod().drawDistance   = 9000.f;    // 9000より遠い壁は描かない(farZ=10000の少し手前)
meshRenderer->GetLod().shadowDistance = 3000.f;    // 影は3000までの壁だけ(Shadow::distance と同じくらい)

// ─── Sample3D: 球(Objects/Sphere.cpp)を、遠いほど粗い球にする(段階2)───
auto* sphereRenderer = sphere->AddComponent<MeshRenderer>(Mesh::CreateSphere(16, 24));
sphereRenderer->SetLodMeshes(Mesh::CreateSphereLods({ {8, 12}, {4, 6} }));   // 段階1・2(段階0は上の16x24)
sphereRenderer->GetLod().meshDistances = { 1500.f, 4000.f };
sphereRenderer->GetLod().outlineDistance = 1200.f;   // 枠線は近くだけ

// ─── Sample3D: FBX(ModelDisplay / SkinDisplay)に距離を設定する ───
auto* modelRenderer = model->AddComponent<ModelRenderer>(L"Assets/Models/Tank.fbx");
modelRenderer->GetLod().drawDistance = 8000.f;
modelRenderer->GetLod().shadowDistance = 2500.f;
// 遠いスキンアニメは止める(Engineを変えない方法。距離は Lod::GetCameraDistance で調べる)
//   Update 内: anim->SetEnabled(Lod::GetCameraDistance(GetOwner()->transform.position) < 3000.f);

// ─── Sample3D: 地形(Hills)にチャンクを設定する(段階3)───
renderer->SetChunking(32, { 2000.f, 4500.f });   // 32マスの区画。2000より遠いと1つおき、4500より遠いと3つおき

// ─── Sample3D: 多数の壁を置いて測る(新しいオブジェクト Objects/WallField.cpp。ユーザーに確認してから作る)───
//   GameScene.cpp: CreateWallField(*this, { 0.f, 50.f, 0.f }, 15);  // 15x15=225本の柱を格子状に置く(遠くまで広がる)

// ─── Sample3D HUD: 数を出す(HUDController.cpp。既存の表示の書き方に合わせる)───
//   "Draw " + std::to_wstring(RenderStats::GetDrawCallCount())
//   "Shadow " + std::to_wstring(RenderStats::GetShadowDrawCount())
//   "Tri " + std::to_wstring(RenderStats::GetTriangleCount())
//   "Culled " + std::to_wstring(Visibility::GetCulledCount() + RenderStats::GetLodCulledCount())
```
- 対象: Sample3D の `Wall.cpp`、`Sphere.cpp`、`ModelDisplay.cpp`、`SkinDisplay.cpp`、`Hills.cpp`、`HUDController.cpp`(カウンタの表示)。多数の壁で測るための `WallField` は新しいオブジェクトなので、**ユーザーに確認してから作る**。Sample2D は、カウンタの確認が主なので、LODの設定は使わない。
- 注意: 球のLODは、元の `Mesh::CreateSphere(16, 24)` だけで十分に軽い(約760枚)ので、**測って効果が小さければ段階2は作らない**(未確定の点)。

#### 依存する機能
6.1 視界判定(`Visibility`。半径の考え方と `GetCulledCount` の流儀)、6.5 影(`Shadow`。影のパスの距離カット)、6.3 マテリアル(`outlineParams.w`。段階4のみ)、6.7 地形(`Terrain` / `TerrainRenderer`。段階3)

#### 未確定の点(ユーザーに聞きたいこと)
- (a) 段階1(距離カット+カウンタ)だけ先に作って、測ってから段階2・3に進むか決める進め方でよいか
- (b) 距離の基準は「カメラからの距離」(既定)でよいか。ズーム(視野の変化)の補正を入れるか
- (c) `drawDistance` で消える時の「急に消える」は、フェード(段階4。シェーダー変更)をしない限り残る。許容するか
- (d) 影の距離カットが、遠い物の影を消す問題(設計のポイント(e))をどう扱うか。既定OFFで、Sample側で慎重に決めてよいか
- (e) FBXのLOD用の別ファイルを、誰が用意するか(自動削減は見送り)。用意できなければ、FBXのLODは作らない
- (f) 地形のチャンク(段階3)は、測って必要になるまで作らなくてよいか
- (g) `ModelAnimationComponent` に `SetUpdateInterval` を足す(Engine変更)か、Sample側の `SetEnabled(false)` で済ませるか
- (h) 多数の壁で測るための `WallField` を Sample3D に置いてよいか
- (i) インスタンス描画(同じ物をまとめて描く)を、別の機能として将来提案してよいか

---

## 8. 全体の未確定事項(ユーザーに決めてほしいこと)

1. **この設計書の了承の扱い**: 「この設計書を読んで了承してもらうこと」を、各機能(7.1・7.2)の着手前の確認として扱ってよいか。 → **決定: 扱ってよい(はい)**。(`EngineRoadmap2.md` のロードマップ2では扱ってよいことになった。今回も同じにするか。決まるまでは、機能ごとに着手前の確認をする。段階ごと(7.1 の段階1〜4、7.2 の段階1〜4)の確認は、8章の1番とは別に、段階2・3・4で新しい種類の変更(シェーダー・地形のメッシュなど)に入る時に聞く。)
2. **作る順番**: 7.1 経路探索 → 7.2 LOD の順でよいか。入れ替えるか。並行して作るか
3. **新しいファイルの置き場所**: 経路探索のファイルを `Engine/Behavior/` に置くか、新しいフォルダ `Engine/Navigation/` を作るか(README の「11フォルダ」の説明を変えることになる) → **決定: `Engine/Behavior/`(新フォルダは作らない)**
4. **7.1 の方式**: 「格子+A\*+直線化」でよいか。ナビメッシュは見送りでよいか。フローフィールドは段階4(任意・後回し)でよいか
5. **7.1 経路が見つからない時の既定**: まっすぐ追う(おすすめ)か、止まるか → **決定: 止まる**(7.1 で実装済み)
6. **7.1 Sample での使い方**: 両サンプルの敵の `ChaseComponent` を `PathFollowComponent` に置き換えてよいか(敵の動きが変わる=ゲームの難しさが変わる)。池(`Water`)を通れない障害物にしてよいか → **決定: 敵は置き換える / 池は通れるまま**
7. **7.2 の進め方**: 段階1(距離カット+カウンタ)を先に作り、測ってから段階2(メッシュ切り替え)・段階3(地形チャンク)に進むか決める進め方でよいか → **決定: よい(はい)**。段階1は実装済み。
8. **7.2 Sample での使い方**: 測るための `WallField`(多数の壁)を Sample3D に置いてよいか。HUD に描画命令の数などを出してよいか → **決定: よい(はい)**(実装済み)
9. **インスタンス描画(別の機能)**: LODより効果が大きい可能性があるが大工事。将来のロードマップ4として提案してよいか
10. **ロードマップの扱い**: 完成したら、README と各ロードマップの状態表を更新する(これまでと同じ)。この文書の状態の欄は、実装が終わった機能から「実装済み」に書き換える

---

## 9. Engine変更ファイルの一覧(ユーザーの了承が必要になる所)

### 7.1 経路探索(既存のEngineファイルは変えない。新規のみ)
| ファイル | 内容 |
|---|---|
| `Engine/Behavior/NavGrid.h/.cpp`(新規) | 格子のデータ・障害物の取り込み |
| `Engine/Behavior/PathFinder.h/.cpp`(新規) | A\* と経路の直線化 |
| `Engine/Behavior/NavGridComponent.h/.cpp`(新規) | 格子を持つComponent・焼き・デバッグ表示 |
| `Engine/Behavior/PathFollowComponent.h/.cpp`(新規) | 経路に沿って歩く(`MoveComponent` の子) |
| `Engine.vcxproj` / `Engine.vcxproj.filters` | 上の登録 |

### 7.2 LOD
| ファイル | 段階 | 内容 |
|---|---|---|
| `Engine/Render3D/RenderStats.h/.cpp`(新規) | 1 | 描画命令・三角形のカウンタ |
| `Engine/Render3D/Lod.h/.cpp`(新規) | 1 | `LodSettings`、距離から段階を選ぶ関数 |
| `Engine/Render3D/MeshRenderer.h` | 1・2 | 距離カット、メッシュの切り替え |
| `Engine/Render3D/ModelRenderer.h/.cpp` | 1・2 | 距離カット、モデルの切り替え |
| `Engine/Render3D/TerrainRenderer.h/.cpp` | 1・3 | 距離カット、チャンク分け |
| `Engine/Render3D/Terrain.h/.cpp` | 3 | 区画ごとのメッシュを作る関数 |
| `Engine/Render3D/Mesh.h/.cpp` | 1・2 | カウンタ、`CreateSphereLods` |
| `Engine/Render3D/SkinMesh.h/.cpp` | 1 | カウンタ |
| `Engine/Core/GameEngine.cpp` | 1 | `RenderStats::BeginFrame()` の1行 |
| `Engine/Animation/ModelAnimationComponent.h/.cpp` | 1(任意) | `SetUpdateInterval` |
| `Engine/Render/SpriteRenderer.h/.cpp` | 1(任意) | `SetMaxDrawDistance` |
| `Shader3D.hlsl` / `Shader3DSkin.hlsl` / `Material.h` | 4(任意) | ディザのフェード(`outlineParams.w`) |
| `Engine.vcxproj` / `Engine.vcxproj.filters` | 1 | 新規ファイルの登録 |

`PhysX.props` は、どの機能でも変えません。

---

## 10. 大きなリスク

### 7.1 経路探索
1. **「壁を置けば自動で避ける」の裏にある、焼く(`BakeFromPhysics`)のタイミング**: `OverlapBox` は出したばかりの物を次のフレームから見つけるので、`Init` で焼くと全部通れる格子になる。2回目の `Update` で焼く(実装済み。実測では、Staticの箱は `Physics::Update` の前でも見つかったが、安全のため2回目にした)。焼けるまでは敵は止まって待つ。
2. **体の大きさ(`agentRadius`)と通路の幅**: 半径を大きくしすぎると、細い通路が全部塞がり、敵が「通れるのに回り道する」「閉じ込められて経路なし」と見える。Sample の壁の間隔と敵の大きさで、実際の値を調整する必要がある。
3. **物理の動きと経路のずれ**: 敵は Dynamic な剛体で、敵どうしが押し合うため、経路の点にきっちり乗らない。詰まり検知と再探索で救うが、調整が必要。
4. **3Dの高さは2.5次元**: 同じXZに複数の床がある場所(橋の下・洞窟)は扱えない。今のSampleには無いが、将来そういうマップを作るなら作り直し。
5. **格子が有限**: 格子の外では経路探索が効かない(敵は止まる)。広い世界で全体をおおうと、マスが多くなる。
6. **敵が多い時の重さ**: 再探索を ±20% ばらしても、敵が数百体なら重くなる可能性。その時は段階4(フローフィールド)か、探索回数の上限を足す。

### 7.2 LOD
1. **効果が予想より小さい可能性が高い**: 現状のSampleは三角形が少ないので、メッシュ切り替え(段階2)・地形チャンク(段階3)の効果は小さいかもしれない。段階1のカウンタで測ってから決める。測らずに段階2・3を作ると、複雑さだけが残る。
2. **影の距離カットで、遠い物の影が消える**: 影の範囲(カメラ前方の箱)の外の物でも、光の向きによっては近くに影を落とす。短く切りすぎると、影が突然消える不具合になる。既定はOFF。
3. **ポップ(急な切り替え)**: ヒステリシスで「ちらつき」は防げるが、「切り替わった瞬間」の見た目の変化は残る。フェードはシェーダー変更が必要(段階4)。
4. **FBXのLODは材料の用意が重い**: パーツの名前・親子関係・骨を同じにした別ファイルが要る。自動生成は見送り。
5. **地形チャンクの継ぎ目**: スカートで隠す方式は簡単だが、段階の差が大きい所で壁が見えることがある。見た目の完璧さは求めない。
6. **`MeshRenderer` は物ごとに `Mesh` を持つ**: 段階2で複数の `Mesh` を持つと、物が多い時にメモリと生成コストが増える。立方体のような元から軽い形はLODしない方がよい。
7. **本当に効くのは「インスタンス描画」かもしれない**: 物が数百個ある場面では、描画命令の数が支配的で、LODより「まとめて描く」方が効く。ただし大工事で、LODの範囲を超えるので、別の機能として提案する(未確定事項 9番)。
