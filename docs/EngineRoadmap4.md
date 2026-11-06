# Engine 機能追加ロードマップ4(LAN通信)

## 1. はじめに

### この文書の目的
Engine(`3D_FreamWork/Engine/`)に追加したい**LAN通信(ネットワーク)機能**について、「どんな方式にするか」「どんな関数・Componentを作るか」「どんな順番で作るか」「Engineのどのファイルを変えるか」「Demoではどう確かめるか」をまとめた設計書です。同じLAN(同じWi-Fiや有線のネットワーク)につないだ2〜4台のPCで、1つのゲームを一緒に遊べるようにするのが目標です。チーム(C++初心者を含む)で分担して実装する時の資料として使います。

**(この文書は設計書として書いた時点の記述です。Part 4-1〜4-5 と 4-7 は実装済みで、実装の記録は 13章にあります。)** この文書は「実際のEngineのコードを読んで」書いたので、既存のクラス名・関数名・ファイル名・行番号は本物です。新しく作る名前(`Network` / `NetworkTransformComponent` など)だけが案で、実装しながら変わってよいものです。

### ユーザーと合意した方針(この設計書の前提)
1. **ホスト式**: 1台が**ホスト**になり、**ルールと物理(PhysX)の計算はホストだけ**が行う。**クライアント**は「入力」だけをホストへ送り、ホストから届いた**結果(位置・向き・状態)を表示**する。PhysXは別のPCで同じ結果になる保証が無いので、「みんなで同じ計算をして一致させる」方式(ロックステップ)ではなく、「結果を配る」方式(状態同期)にする。
2. **最大4人**を想定する。最大人数は**定数1か所**(`Network::maxPlayers`)にまとめ、変更しやすくする。**2人から試せる**こと(2台、または1台で2つ起動)。
3. **Transform同期**で対応する(2Dでも3Dでも同じ仕組みで使える)。
4. ライブラリは **ENet**(MITライセンス、小さい、UDPの上に「信頼できる通信」と「速いけれど落ちることもある通信」の両方がある、他のライブラリに依存しない)を `ThirdParty/` に入れる。PhysXと同じく、LICENSEと入手元を書いたREADMEを添える。**配布元・バージョン・ライセンスの確認は、ダウンロードの前にユーザーの許可が必要**なので、この設計書には「実装時に確認」とだけ書く(**設計書の作成中にダウンロードはしていません**)。
5. **2種類のゲームを作れる土台**にする: ①スマブラ風の対戦(位置を高い頻度で送り続ける) ②協力パズル(スイッチ・扉など、**変わった時だけ**「出来事」を送る)。
6. **Unity Netcode風**の使い心地: 同期したいオブジェクトに Component を付ける。Sceneには「何を置くか」だけを書く。**ゲーム側はENetを直接さわらない**。受け取った出来事は `EventBus` でも伝えられる。
7. 層は「通信(ENet) → 部屋/セッション(参加者・プレイヤー番号・ホスト探索) → 同期(`NetworkId`・`NetworkTransform`) → ゲーム側」の4つ。
8. スマブラ風の遅延対策(自分のキャラを先に動かす「クライアント予測」)は**最初は入れない**。後の段階(Part 4-6)に「任意・様子を見てから」と書く。

### `docs/EngineRoadmap.md` / `EngineRoadmap2.md` / `EngineRoadmap3.md` / `DemoPlan.md` との関係
- ロードマップ1〜3の機能は**もう使える前提**で書いています(ロードマップ2は6.x、ロードマップ3は7.x だったので、この文書の機能は **「8.1 LAN通信」** と番号を付けます。6章は用語、7章は全体構成と通信の中身、8章が機能ごとの設計)。書き方(目的 / ファイル案 / 関数 / 設計のポイント / 使うコード / 依存する機能 / 未確定の点)は同じにそろえました。
- 特に使う既存の物: `Transform`(位置・回転・大きさ)、`RigidbodyComponent`(Dynamic / Kinematic / Static)、`EventBus`、`ObjectPool`、`SceneManager`、`Time`、`Input`、`GameEngine::Run` のフレームの順番。
- 確かめる場所は **Demoプロジェクト**(`docs/DemoPlan.md`)です。Demoに新しいカテゴリ `Network` を足し、2台(または1台で2つ起動)で動かして確認します。Sample2D / Sample3D はネットワーク対応にしません(未確定事項 14番)。

### 状態
**Part 4-1〜4-5 と Part 4-7 は実装済み**(ユーザーの「承認は無視していい。一通り実装して Demo で試してみたい」という指示で、各Partの着手前の確認は省略。実装の記録は 13章)。**Part 4-6(遅延対策)は作っていません**(設計どおり、LANで遊んでから決める)。ユーザーによる2台での動作確認は、これからです。

### 進め方の方針(1つのPartずつ)
1つのPartごとに、次の流れで進めます。

1. **着手前の確認**: 何を作るか・どのEngineファイルを変えるか・両サンプルとDemoへの影響を、短く説明する(下の「重要」を参照)
2. **implementer(作業役)が実装** + Debug/Release のビルド確認
3. **reviewer(レビュー役)がレビュー**(Engineの変更・新しいファイル・ロジックの変更は必須)
4. **ユーザーが2台(または1台で2つ起動)で動かして確認**(キー入力・マウス操作の確認はユーザーにお願いする。合成したキー入力は送らない)
5. **コミット文を考える**(`commit-message` スキル。READMEも同じターンで更新)

> **重要: この文書の機能は、Engineを変える時にユーザーの了承が必要です。**
> `CLAUDE.md` の「docs/EngineRoadmap.md の機能を作る間の例外(了承を待たずに進めてよい)」の対象は、**`docs/EngineRoadmap.md` の機能だけ**です。この `EngineRoadmap4.md` の機能は対象に**入っていません**(`EngineRoadmap2.md` / `EngineRoadmap3.md` と同じ扱い)。そのため、Engine(`3D_FreamWork/Engine/` 以下、`Shader*.hlsl`、`PhysX.props`)を変える時は、通常のルールどおり **①何を変えるか ②なぜ必要か ③Sample2D・Sample3D 両方への影響** を説明して、ユーザーの了承をもらってから手を付けます。
>
> 「この設計書を読んで了承してもらうこと」を、各Partの着手前の確認として扱ってよいかは、**ユーザーが決めます**(9章の未確定事項 1番)。決まるまでは、安全側(Partごとに着手前の確認をする)で進めます。了承は「そのPart1つ分」だけです。
>
> **もう1つ、`CLAUDE.md` の文言を変える必要があります。** `CLAUDE.md` の設計の節には「シングルトン(`Get()`)はアプリ全体で1つのサービス(EventBus/SceneManager/Input/Audio)だけ」とあります。`Network` を5つ目のサービスにする案(8.1 の設計のポイント(a))は、この一覧に `Network` を足すことになります。`CLAUDE.md` は**ユーザー本人の了承が無ければ変えません**(未確定事項 3番)。

### 名前の付け方・ファイルを追加する時
`EngineRoadmap.md` の「2. 共通ルール」と同じです(`CLAUDE.md` の命名規則に従う / 新しいファイルは `Engine.vcxproj` と `Engine.vcxproj.filters` の両方に登録 / 日本語を含む `.h`・`.cpp` は UTF-8 BOM付き / Debug と Release の両方でビルド確認)。
Engine は機能別の11フォルダに分かれています(README の「Engineのフォルダ構成」)。ネットワークのファイルは数が多く、他の11フォルダのどれにも収まらないので、**12個目のフォルダ `Engine/Network/`** を作る案にしました(5.2 と未確定事項 2番。README の「11フォルダ」の説明を変えることになるので、ユーザーが決めます)。`.filters` のフィルターも同じ `Network` にします。

### 作ったらDemoで必ず確かめる
これまでのロードマップは「Sampleのコードを置き換える」約束でした。ネットワークはゲームの遊び方そのものを変えるので、Sample2D / Sample3D は変えず、**Demoプロジェクトの `Network` カテゴリに、Partごとに確認用のデモを足していく**ことにします(8.1 の「使うDemoのコード」と、各Partの「確認方法」)。

---

## 2. 作る順番の考え方

一覧表は 4章にあります。この章では、その順番にした理由を説明します。

- **下の層から順に作ります**(通信 → セッション → 同期 → 入力と出来事 → スポーン)。上の層は、下の層が動いていないと確かめられないからです。
- **Part 4-1 で「2台がつながって、文字を送り合える」ところまで**を最初に作ります。ここが動けば、ENetの導入(`.vcxproj`・ライブラリのリンク・ファイアウォール)という**一番つまずきやすい所**を、最初に片付けられます。ゲームの機能(位置の同期など)は、つながってから作ります。
- **Part 4-3(Transformの同期)でスマブラ風の土台**、**Part 4-4(出来事)で協力パズルの土台**ができます。Part 4-3 と 4-4 は独立に近いので、担当者が分かれているなら並行して作ってもかまいません(どちらも Part 4-2 のセッションが前提)。
- **Part 4-5(スポーン)を後にした理由**: スポーン(ゲーム中にオブジェクトを出す・消す)は、シーンの切り替え・途中参加・`ObjectPool` との相性など考えることが多く、位置の同期と出来事が安定してからの方が安全です。最初は「シーンに最初から置いてある物」だけを同期します(Part 4-3)。
- **Part 4-6(クライアント予測など)は任意**です。Part 4-5 までで、遊べる状態になります。実際にLANで遊んで「操作が重い」と感じたら作ります。
- **それぞれのPartに、確認用のデモを付けます**。画面を見て・操作して確かめるのが一番確実だからです。

---

## 3. 今のEngineで分かっている事実(コードを読んで確認したこと)

設計に関わる「今のコードの事実」です。各設計は、これを前提に書いています。パスは `3D_FreamWork/Engine/` からの相対です。

### フレームの順番と、時間
1. **`GameEngine::Run` の1フレームの順番**(`Core/GameEngine.cpp:44-52`): `Time::Update()`(44) → `Input::Update()`(46) → `CameraShake::Update`(47) → `ScreenEffect::Update`(48) → **`Physics::Update(dt)`(50)** → **`SceneManager::Get().Update(dt)`(51)** → `Audio::Update()`(52)。その後 `Image::BeginFrame()`(56)から描画。つまり**物理が先、シーンの `Update`(各Component)が後**です。
   - **ネットワークを呼ぶ場所**: ①**`Input::Update()` の直後・`Physics::Update` の前**で `Network::Update()`(受信。クライアントは受け取った位置をTransformへ反映、ホストはクライアントの入力を取り込む) ②**`Audio::Update()`(52)の後**で `Network::EndFrame()`(ホストは「このフレームの最終的な状態」を集めて送る。送信を実際に流す)。
   - **②がシーンの `Update` の後である理由**: Dynamicな剛体のTransformは、`RigidbodyComponent::Update`(シーンの `Update` の中)が PhysX の結果をTransformに書き込む(`Physics/RigidbodyComponent.cpp:206-207`)。ホストが送る状態は、**その後の値**でなければ1フレーム古くなる。
   - **①が `Physics::Update` の前である理由**: クライアントの「見せるだけのコピー」は Kinematic にする(下の「Rigidbody」)。Kinematic は `RigidbodyComponent::Update` が Transform → PhysX へ反映する(`RigidbodyComponent.cpp:215`)ので、その前にTransformを書き換えておけば、同じフレームの中で反映される。
2. **画面は垂直同期**(`Render/Image.cpp:294` の `Present(1, 0)`)なので、フレームレートはモニタ次第(60Hzが多いが、144Hzのモニタもある)。**送信の間隔はフレーム数ではなく、時間(秒)で数える**。
3. **`Time` の1フレームの上限は0.1秒**(`Core/Time.cpp:6`)。ネットワークの時間(送信の間隔など)は、スローやポーズの影響を受けない `Time::GetUnscaledDeltaTime()`(`Core/Time.h:9`)を使う。`SceneManager` の演出の間は `Time` の倍率が0になる(`Core/SceneManager.cpp:21`)が、通信は止めない。
4. **ウィンドウのタイトルバーをドラッグしている間は、WindowsのメッセージループがGameEngine::Runの外で回る**ので、ゲームの1フレームが止まる(PeekMessageのループに戻らない)。デバッガで止めた時も同じ。止まっている間、ENetの `service` が呼ばれない → 相手は「応答が無い」と見て切断する。**タイムアウトを長めに(10秒程度)する**必要がある(11章のリスク)。

### Transform
5. **`Transform`**(`Core/Transform.h`): `position`(12行目)・`scale`(13行目)・`rotate`(14行目。**度数、XYZそれぞれ**)の3つが公開メンバ。親は `private`(`parent`、107行目)で、`SetParent`(18行目)で設定する。2Dは主に `position.xy` と `rotate.z`、3Dは3つとも使う。`GetWorldTransform()`(61行目)の2D用の合成は「Z軸回転だけ」を合成する。→ **同期するのは「親を持たない(ルートの)オブジェクトの `position` / `rotate` / `scale`」だけ**にする(子オブジェクトの同期は作らない)。
6. 補間に使える関数は `Math::Lerp(float, float, t)` と `Math::Lerp(XMFLOAT3, XMFLOAT3, t)`(`Core/MathUtil.h:51-52`)。**角度の補間(360度をまたぐ時に近い向きに回る)は無い**ので、Networkの中に小さく作る必要がある。

### Rigidbody(物理)
7. **剛体の種類**(`Physics/RigidbodyComponent.h:19-23`): `Dynamic`(PhysXの結果がTransformに入る)/ `Kinematic`(自分のコードがTransformを動かす。他のDynamicを押せる)/ `Static`。**種類はコンストラクタで決まり**(`RigidbodyComponent.h:38,52`)、後から変えるsetterは無い(`GetBodyType()` だけ。116行目)。
8. **`RigidbodyComponent::Update`**(`RigidbodyComponent.cpp:190-218`): Dynamic は PhysX → Transform(`SyncTransformFromActor`、回転ロック中は位置だけ)。Kinematic は Transform → PhysX(`SyncActorFromTransform`)。作った直後・`ObjectPool` で再利用された直後(`justActivated`)は、Transform の位置へ**テレポート**し、速度を0に戻す(193-203, 210-213行目)。
   - → **「Dynamicな物はTransformを直接書き換えない」(CLAUDE.md)ので、ネットワークで位置を受け取るクライアント側のコピーは、Dynamicにしない。Kinematic にするか、Rigidbodyを付けない。** どちらにするかは、工場関数が決める(`Create○○(Scene&, const NetworkSpawnInfo&)` の中で `info.hasAuthority` を見て `BodyType` を選ぶ)。
   - Kinematic のコピーを選ぶ理由: `Physics::Raycast` / `OverlapBox` でクライアント側にも当たりが出る(マウスで選ぶ・影の判定など)。Rigidbodyを付けない選択の理由: 軽い・「クライアントでは当たり判定のコールバックが動かない」ことが一目で分かる。**当たり判定のコールバック(`OnTriggerEnter` など)に反応するゲームのルールは、ホストだけで動かす**(`HasAuthority()` を見て、クライアントでは何もしない)。
9. **`Physics::Update(dt)`**(`Physics/Physics.h:43`、`Physics.cpp:376-378`)は `dt` をそのまま `simulate(dt)` に渡す(可変の時間刻み)。→ ホストの物理は固定の時間刻みではなく、フレームレートで結果が少し変わる。**別のPCで同じ結果にならない**ので、状態同期にした(上の「合意した方針」1)。

### EventBus
10. **`EventBus`**(`Core/EventBus.h`): `Subscribe(name, Callback)` の `Callback` は `std::function<void()>`(**引数なし**、31行目)。`SubscribeObject` は `GameObject*` を1つ渡せる(32行目)。**数値や文字などの中身(ペイロード)は運べない**(69行目に「返したいイベントの引数はここに追加していく」とあるだけ)。ラムダが `this` をキャプチャしたら、Sceneの `Uninit()` で `Clear()` する決まり(25-28行目)。`Get()` で取る(34行目)。
    - → **中身の無い出来事は `EventBus::Publish(name)` でも伝え、中身のある出来事は `Network::Subscribe` で受ける**(EventBus自体は変えない。未確定事項 10番)。

### シーン・GameObject・ObjectPool
11. **`SceneManager::ChangeScene<T>()` はテンプレート**(`Core/SceneManager.h:31`)で、`Scene` を `new T()` で作る関数を渡す仕組み。Networkから「ホストが切り替えた先のシーンへ、クライアントも切り替える」には、**シーンの番号と切り替える関数をゲーム側が登録**する形になる(`Network::RegisterScene(id, fn)`)。切り替えの途中に呼んだ `ChangeScene` は**無視される**(`Core/SceneManager.cpp:10-13`)ので、Networkは切り替えが終わるまで待つ必要がある。
12. **シーンが変わる時の順番**(`SceneManager.cpp:25-41`): 古いシーンの `Uninit` → `ScreenEffect::OnSceneChanged` / `CameraShake::Stop` → 新しいシーンを `new` → `Init`。`Scene::Uninit`(`Core/Scene.cpp:64-67`)が全オブジェクトの `Uninit` → 全Componentの `Uninit` を呼ぶ。→ **`NetworkIdentityComponent::Uninit` で、`NetworkId → GameObject` の登録簿から外す**。**Network自体(接続)はシーンをまたいで残る**(アプリ全体で1つ)。
13. **`Scene::CreateObject`**(`Core/Scene.h:39-46`): `Update` 中に呼ぶと `pendingObjects` に入り、`Update` の終わりに `objects` へ移る。ポインタは作った時から有効。→ `Network::Update`(シーンの `Update` の外)で `Spawn` の工場関数を呼ぶ時は直接 `objects` に入り、シーンの `Update` の中で呼ぶ時は保留される。どちらでも安全。
14. **`GameObject::Destroy()`**(`Core/GameObject.h:105`)は「次の `Scene::Update` で破棄する」印(`Scene.cpp:15-21`)。`SetActive(false)` は破棄せず、全Componentの `OnActiveChanged(false)` を呼ぶ(98-102行目、`Core/Component.h:25`)。**`ObjectPool::Return` は `SetActive(false)` だけ**(`Core/ObjectPool.h:62-64`)。→ ネットワークの「消す(Despawn)」は、`Destroy()` と「プールに返す」の**両方**を扱えるようにする。`NetworkIdentityComponent::OnActiveChanged(false)` でも、登録簿から外す。
15. **`ObjectPool::Rent`**(`ObjectPool.h:45-59`)は、前に使った値が残ったまま返す。→ スポーンで使い回す時は、位置だけでなく `NetworkId` と所有者も毎回付け直す。

### サービスの形、ThirdParty、ビルド
16. **「アプリ全体で1つのサービス」の形は2種類ある**: `EventBus` / `SceneManager` は**クラス + `Get()`**(`EventBus.h:34`、`SceneManager.h:24`)。`Input` / `Audio` / `Time` / `Physics` は**名前空間 + 関数**(`Core/Input.h:94`、`Audio/Audio.h:11`、`Core/Time.h:7`、`Physics/Physics.h:41`)で、`Get()` は無い。→ **`Network` は後者(`namespace Network`)にする**(8.1 の設計のポイント(a))。
17. **ThirdParty の前例**: `ThirdParty/ufbx/`(`ufbx.c` と `ufbx.h`、`LICENSE`、`README.txt`)は、**C言語のソースを `Engine.vcxproj` に直接登録してビルド**している(`Engine/Engine.vcxproj:141-147`。`CompileAs=CompileAsC`、警告オフ、SDLチェックオフ)。インクルードの場所は `AdditionalIncludeDirectories` に足している(`Engine.vcxproj:52, 66`)。`ThirdParty/PhysX/` には `LICENSE.txt` と `README.txt` がある。→ **ENet も同じ形(`ThirdParty/enet/` にソースを置き、`Engine.vcxproj` でCとしてビルド)にできる**。ENetはCのソースが十数個と小さく、ライブラリ(`.lib`)を別に作ると Debug/Release・ランタイム(`/MTd` / `/MT`)の不一致が起きやすいので、ソースを直接ビルドする案をおすすめする。
18. **ランタイム(CRT)の設定**: `PhysX.props` が Debug は `MultiThreadedDebug`(19行目)、Release は `MultiThreaded`(29行目)にしている。ソースを `Engine.vcxproj` で直接ビルドすれば、この設定が自動で効き、不一致の心配が無い。**`PhysX.props` は変えない。**
19. **`#pragma comment(lib, ...)` の前例**: `Core/Input.h:5`(`Xinput.lib`)、`Audio/Audio.h:5`(`xaudio2.lib`)。→ `ws2_32.lib`(Winsock)と `winmm.lib`(ENetが時間の取得に使う)は、`Network` のソースファイルに `#pragma comment(lib, ...)` で書く。静的ライブラリ `Engine.lib` に記録され、実行ファイルのリンク時に自動で読まれる(実装時に確認)。Sample / Demo の `.vcxproj` は変えない。
20. **入力は、ウィンドウが選ばれている時だけ有効**: `Core/Input.cpp:52-54` が「自分のウィンドウが一番手前で、最小化されていない」かを調べる(今はカーソルの固定の解除に使われている)。1台で2つ起動した時に、**選んでいない側のウィンドウがキー入力を受け取るかどうかは、実装時に確かめる**(受け取らないなら、1台のテストでは「選んだ側だけ操作できる」ことになる)。

### Demo
21. **Demoのメニュー**: `DemoCategory` の `enum class`(`Demo/Game/Scenes/Common/DemoRegistry.h:9`)に値を足し、`DemoRegistry.cpp` の一覧に1行ずつ足すと、ボタンが並ぶ(例: 131行目の `Move`)。2D版・3D版は `open2D` / `open3D` に別の関数を入れる(`DemoRegistry.h:14-20`)。**Tab = 2D/3D 切り替え、Esc = 戻る、Q・E = ズーム**は共通部品が使う予約キー(`DemoPlan.md` 9.2)。→ ネットワークのデモの操作(ホストになる・参加する)は**ボタン(マウス)**で行い、キーは予約キー以外にする。

### 実装時に確認すること(今は分かっていない)
- ENetの最新の安定版・ライセンス・ファイル構成(`ThirdParty/enet/` に置く物)と、ENet 1.3 系が IPv4 のみかどうか(LANで困らないはず)
- ENetの「信頼しない」通信が、同じチャンネルで古いパケットを自動で捨てるか(「順序付きの非信頼」。8.1 の設計のポイント(e))
- 2つ目のウィンドウがキー入力を受け取るか(上の20)
- 学校のネットワークで、ブロードキャスト(ホスト探索)や端末どうしの通信が通るか(11章)

---

## 4. 作る順番(一覧表)

| 順 | Part | 内容 | 理由・依存 | 担当者 | 状態 |
|---|---|---|---|---|---|
| 1 | Part 4-1 | ENet導入 + 接続とメッセージの送受信(`Network` の土台。Demoに `Network` カテゴリと「つながる」デモ) | 単独。一番つまずきやすい所(ビルド設定・ファイアウォール)を最初に片付ける | | 完了(Engine。ビルド・スクラッチテスト済み。Demoの確認は4-7) |
| 2 | Part 4-2 | セッション(参加者名簿・プレイヤー番号・満員/バージョン違いの断り・切断の検知・LANホスト探索) | Part 4-1 | | 完了(Engine。ビルド・スクラッチテスト済み。Demoの確認は4-7) |
| 3 | Part 4-3 | `NetworkIdentityComponent` + `NetworkTransformComponent`(位置・向きの同期と補間。シーンに最初から置いた物だけ) | Part 4-2。**スマブラ風の土台** | | 完了(Engine。ビルド・スクラッチテスト済み。Demoの確認は4-7) |
| 4 | Part 4-4 | 入力送信(`NetworkInput`)と出来事(信頼メッセージ + EventBus + 共有の値) | Part 4-2(Part 4-3 と並行可)。**協力パズルの土台** | | 完了(Engine。ビルド・スクラッチテスト済み。Demoの確認は4-7) |
| 5 | Part 4-5 | スポーン / 破棄、シーン切り替えの同期、途中参加の状態送信 | Part 4-3・4-4 | | 完了(Engine。ビルド・スクラッチテスト済み。Demoの確認は4-7) |
| 6 | Part 4-6 | (任意)遅延対策: クライアント予測 / Owner権限 | Part 4-5。LANで遊んでから決める | | 先送り(任意。今回は作らない) |
| 7 | Part 4-7 | Demoの仕上げ(2D版・3D版)、README・EngineGuide | Part 4-5(4-6 は任意) | | 作業中(Demo・README・EngineGuide) |

---

## 5. 全体で共通する設計

### 5.1 シェーダーと `PhysX.props` は変えない
ネットワークは描画にも物理の設定にも触りません。`Shader*.hlsl` と `PhysX.props` は**変えません**。変えるEngineファイルは、`Engine.vcxproj` / `.filters` と `GameEngine.cpp` の3行だけです(10章)。

### 5.2 新しいEngineファイルの置き場所
| 新しいファイル | 置くフォルダ | 理由 |
|---|---|---|
| `Network` / `NetworkConfig` / `NetworkTransport` / `NetworkSession` / `NetworkMessage` / `NetworkInput` / `NetworkObjects` / `NetworkIdentityComponent` / `NetworkTransformComponent` / `LanDiscovery` | `Engine/Network/`(**新フォルダ**) | 10組ほど増える。Behavior などに混ぜると役割が分かりにくい。`.filters` のフィルターも `Network` にそろえる |
| ENet本体(Cのソース・ヘッダー・`LICENSE`・`README.txt`) | `ThirdParty/enet/`(Engineの外。`ufbx` と同じ) | 外部ライブラリは `ThirdParty/` に置く決まり |

README の「Engineのフォルダ構成」(11フォルダ)は12フォルダになります。決めるのはユーザーです(未確定事項 2番)。決まるまでは設計書のとおり `Engine/Network/` で進めます。

### 5.3 何もしなければ今まで通り(既定はオフライン)
ロードマップ2・3の流儀と同じです。`Network::StartHost` / `Join` を呼ばない限り、`Network` は**何もしません**(`Network::Update` / `EndFrame` の先頭で `if (オフライン) return;`)。ENetの初期化も、`StartHost` / `Join` / `StartSearch` を呼んだ時に初めて行います。Sample2D / Sample3D と、ネットワークを使わないDemoは、**見た目も速さも変わりません**。

**オフラインでも、ゲーム側のコードが同じに書ける**ようにします: `Network::GetLocalPlayerIndex()` は 0 を返し、`Network::GetInput(0)` は自分の入力(`SetLocalInput` で渡した物)を返し、`NetworkIdentityComponent::HasAuthority()` は true を返します。つまり**ネットワーク対応のゲームのコードは、1人用としてもそのまま動きます**(Part 4-3 以降のDemoは、オフラインでも動かして確認できる)。

### 5.4 確認用の表示
- `Network::GetStats()` で、送った/受け取ったバイト数・パケット数・ping(往復の時間)を数値で返します(HUDに出して確認する。`RenderStats` / `Visibility::GetCulledCount` と同じ流儀)。
- `Network::SetDebugLogEnabled(bool)` で、送受信したパケットの種類を `OutputDebugString` に出します(既定オフ)。通信の不具合を調べる時に使います。

---

## 6. 用語のやさしい説明

- **LAN(ラン)**: 同じWi-Fiや同じ有線のネットワークにつながった機器どうしの、狭いネットワーク。この機能は**LANだけが対象**で、インターネット越しの対戦(ルーター越し)は対象外。
- **ホスト**: 部屋を作る側のPC。**ルールと物理の計算をするのはホストだけ**。ホスト自身もプレイヤーの1人(プレイヤー番号 0)。
- **クライアント**: ホストに参加する側のPC。自分の**入力**をホストへ送り、ホストから届いた**結果**を画面に表示する。
- **セッション**: 「今つながっているみんな」のまとまり。参加者の名簿とプレイヤー番号を持つ。
- **プレイヤー番号(playerIndex)**: 参加者に付く 0, 1, 2, 3 の番号。ホストが 0。最大人数は `Network::maxPlayers`(既定4)。
- **Authority(オーソリティ、権限)**: 「そのオブジェクトの本当の状態を決める側」。**このEngineでは、基本はホストが全オブジェクトのAuthority**。クライアントにあるのは見せるためのコピー。
- **NetworkId**: 同期するオブジェクトに付ける、**ホストとクライアントで共通の番号**(整数)。「ホストの3番の箱」と「クライアントの3番の箱」を結びつける。
- **スナップショット**: 「ある瞬間の、同期する全オブジェクトの位置・向きの写真」。ホストが一定の間隔(30回/秒など)でみんなに送る。
- **補間**: スナップショットは30回/秒でしか届かないので、そのまま表示するとカクカクする。届いた2枚の写真の**間を滑らかにつないで**表示すること。少し遅れて(0.1秒ほど)表示する代わりに、動きが滑らかになる。
- **信頼できる通信(Reliable)**: 必ず届く・送った順に届く(落ちたら送り直す)。入室・スポーン・出来事など、1回でも落ちると困る物に使う。少し遅れることがある。
- **信頼しない通信(Unreliable)**: 送りっぱなしで、落ちることもある。**速い**。位置のスナップショットや入力のように、次々に新しい物が届くので、1つ落ちても困らない物に使う。
- **チャンネル**: ENetの中の「通信の通り道」。通り道が違えば、お互いを待たない(信頼する通信が遅れても、位置のスナップショットは遅れない)。
- **ペイロード**: パケットの中身(送りたいデータそのもの)。
- **ping(RTT)**: こちらから送って、返事が戻ってくるまでの往復の時間。LANなら数ミリ秒〜数十ミリ秒(Wi-Fiは揺れる)。
- **ブロードキャスト**: 「同じLANの全員に向けて」送る通信。ホストを探す時に「ホストの人、いますか?」と呼びかけるのに使う。
- **クライアント予測**: 自分のキャラだけは、ホストの返事を待たずに先に動かして、後でホストの結果とずれていたら直す方法(Part 4-6 の任意の案)。
- **ENet(イーネット)**: UDPの上に、信頼できる通信・チャンネル・接続の管理を作ってくれる小さなCのライブラリ。ゲーム向け。

---

## 7. 全体構成と通信の中身

### 7.1 4つの層

```
 ┌──────────────────────────────────────────────────────────────┐
 │ ④ ゲーム側(Sample / Demo の Game/ 以下)                       │
 │    Scene = 何を置くか / Objects = Create○○ / Components = ルール│
 │    PlayerController など(Network::GetInput を読む)            │
 ├──────────────────────────────────────────────────────────────┤
 │ ③ 同期  Engine/Network/                                        │
 │    NetworkIdentityComponent (NetworkId・所有者・Authority)     │
 │    NetworkTransformComponent(位置・向きの送受信と補間)          │
 │    NetworkObjects(NetworkId → GameObject の登録簿・スポーン)   │
 │    NetworkInput(入力) / 出来事(SendEvent・Subscribe・EventBus)│
 ├──────────────────────────────────────────────────────────────┤
 │ ② 部屋(セッション)  Engine/Network/                           │
 │    NetworkSession(参加者名簿・プレイヤー番号・Join/Accept)      │
 │    LanDiscovery(LANでホストを探す)                            │
 ├──────────────────────────────────────────────────────────────┤
 │ ① 通信  Engine/Network/ + ThirdParty/enet/                    │
 │    NetworkTransport(ENetを包む。ENetを知っているのはここだけ)  │
 │    NetworkMessage(パケットの種類・書き込み/読み取り)           │
 └──────────────────────────────────────────────────────────────┘
        窓口 = namespace Network(Network.h/.cpp)。ゲームが呼ぶのはここ
```

- **ゲーム側が触るのは `Network`(窓口)と、2つのComponent だけ**。`ENet` の型・関数は、`NetworkTransport.cpp` と `LanDiscovery.cpp` の中に閉じ込め、`Network.h` などの公開ヘッダーには出さない(`enet/enet.h` は `winsock2.h` を読み込むので、`windows.h` を先に読んでいるファイルと衝突する。ENetを読むファイルは、`#include "enet/enet.h"` を**最初の行**に書く)。
- **層ごとに独立した部品にする**(`NetworkTransport` / `NetworkSession` は、`Network` の外でも `new` できるクラスにする)。こうすると、画面なし・ウィンドウなしの**スクラッチテスト**(1つのプログラムの中に、ホスト役とクライアント役の2つを作って、localhostでつなぐ)で、ロードマップ3と同じように動作を確かめられる。

### 7.2 データの流れ(入力 → ホスト → 状態 → 全員)

```
 クライアント B                      ホスト A                       クライアント C
 ─────────────                      ───────                        ─────────────
 キー入力
   │ Network::SetLocalInput
   ▼
 [Input パケット] ───(60回/秒)───▶ 入力を保存           ◀─── [Input パケット]
 (信頼しない)                       │ Network::GetInput(1)       (信頼しない)
                                    ▼
                                  PlayerController(ホストだけ)
                                  SetVelocity → PhysX が計算
                                    │
                                    ▼
                                  RigidbodyComponent::Update で
                                  Transform に結果が入る
                                    │ Network::EndFrame
                                    ▼
 受け取った位置を    ◀──(30回/秒)── [Snapshot パケット] ──(30回/秒)──▶ 受け取った位置を
 補間して表示                       (全オブジェクトの                    補間して表示
 (Kinematic のコピー)               NetworkId + 位置 + 向き)            (Kinematic のコピー)

 ─ 出来事(扉が開いた、など)は、信頼できる [Event パケット] で、必ず届くように送る ─
```

- 入力 → ホストの計算 → 状態の配布、の一方通行です。**クライアントの画面の位置は、ホストが決めた結果**で、クライアント自身は位置を決めません(Part 4-6 の任意の案を除く)。
- ホスト自身のプレイヤー(番号0)も、同じ `Network::GetInput(0)` で入力を受け取ります(ネットワークを通らず、自分の入力がそのまま返る)。ゲームのコードは、ホストのプレイヤーとクライアントのプレイヤーを区別せずに書けます。

### 7.3 1フレームの順番(`GameEngine::Run` のどこで呼ぶか)

```
 Time::Update()
 Input::Update()
 ★ Network::Update()      ← 受信。ホスト: クライアントの入力・出来事を取り込む
                             クライアント: スナップショットを補間してTransformへ反映
 CameraShake::Update / ScreenEffect::Update
 Physics::Update(dt)       ← PhysX の計算(ホスト)/ Kinematic の反映(クライアント)
 SceneManager::Update(dt)  ← 各Componentの Update(PlayerController が Network::GetInput を読む。
                             RigidbodyComponent::Update が PhysX の結果を Transform に書く)
 Audio::Update()
 ★ Network::EndFrame()    ← 送信。ホスト: スナップショットを作って送る(30Hz)
                             クライアント: 入力を送る(60Hz)。ENetに溜めた送信を流す
 Image::BeginFrame() … 描画
```

- 根拠は 3章の 1 です(`GameEngine.cpp:46, 50, 51, 52`)。**変えるのは `GameEngine.cpp` に3行**(`Network::Update()`、`Network::EndFrame()`、`Uninit` の中の `Network::Shutdown()`)だけです。
- `Network::Shutdown()` は、`SceneManager::Get().Uninit()`(`GameEngine.cpp:75`)の**後**に呼びます(各Componentの `Uninit` が登録簿から外し終わってから、接続を閉じる)。ENetの終了処理(`enet_deinitialize`)とWinsockの後片付けもここで行います。

### 7.4 パケットの種類一覧(案)

パケットの最初の1バイトが種類(`enum class PacketType : uint8_t`)です。数や名前は実装しながら変わってよい案です。

| 種類 | 向き | 信頼 | チャンネル | 中身 | 使う場面 |
|---|---|---|---|---|---|
| `Join` | クライアント → ホスト | する | 0 | プロトコル番号(`protocolVersion`)、ゲームID、プレイヤー名 | 参加の申し込み |
| `Accept` | ホスト → クライアント | する | 0 | 自分のプレイヤー番号、ホストの時刻、今の参加者の名簿、最大人数 | 参加を許可 |
| `Reject` | ホスト → クライアント | する | 0 | 理由(満員 / バージョン違い / ゲームID違い) | 参加を断る |
| `PlayerJoined` / `PlayerLeft` | ホスト → 全員 | する | 0 | プレイヤー番号、(参加の時は)名前 | 名簿の更新 |
| `Leave` | クライアント → ホスト | する | 0 | (なし) | 自分から抜ける(無くても、切断で分かる) |
| `SceneChange` | ホスト → 全員 | する | 0 | シーン番号 | ホストがシーンを切り替えた |
| `SceneReady` | クライアント → ホスト | する | 0 | シーン番号 | 切り替えが終わった(この後に初期状態を送ってもらう) |
| `Spawn` | ホスト → 全員 | する | 0 | `NetworkId`、プレハブ番号、所有者、位置・向き | オブジェクトが現れた |
| `Despawn` | ホスト → 全員 | する | 0 | `NetworkId` | オブジェクトが消えた |
| `Event` | 誰か → 相手 | する | 0 | 出来事の名前、送り先の種類、中身(ペイロード) | 扉が開いた・スイッチが押された、など |
| `SharedValue` | ホスト → 全員 | する | 0 | キー(名前)、整数 | 「扉は開いている」のような**今の状態**。途中参加の人にも送る |
| `Ping` / `Pong` | 双方 | しない | 1 | 送った時刻 | pingの測定、ホストの時刻とのずれの推定 |
| `Input` | クライアント → ホスト | しない | 1 | 連番、軸(移動・視点)、ボタンの押している/押した瞬間 | 毎フレームの入力 |
| `Snapshot` | ホスト → クライアント | しない | 2 | 連番、ホストの時刻、(NetworkId + 位置 + 向き)の並び | 位置の同期(30Hz) |

LANの探索用(ゲーム本体とは**別のUDPポート**。`LanDiscovery`):

| 種類 | 向き | 中身 |
|---|---|---|
| `DiscoveryQuery` | 探す側 → LAN全体(ブロードキャスト) | 目印の文字(`3DFW`)、ゲームID、プロトコル番号 |
| `DiscoveryReply` | ホスト → 探した人だけ | 目印、ゲームID、ゲームのポート、ホスト名、今の人数 / 最大人数 |

- **チャンネルを分ける理由**: 信頼できる通信(チャンネル0)の再送で待たされても、位置(チャンネル2)や入力(チャンネル1)は待たない。
- **受け取った値は必ず検査する**(11章): パケットが短すぎる・種類が知らない物・プレイヤー番号が範囲外・数値が `NaN` / 異常に大きい・文字列が長すぎる物は捨てる。クライアントが送る `Input` の「プレイヤー番号」は信用せず、**ホストが接続(ENetの peer)から決める**。

### 7.5 送信の頻度と、信頼/非信頼の使い分け

| 送る物 | 頻度(既定) | 信頼 | 理由 |
|---|---|---|---|
| `Snapshot`(位置・向き) | 30回/秒(10〜60で変えられる) | しない | 次々に新しいのが届く。落ちた1枚は、次の1枚で埋まる。再送すると逆に古い位置が届いてしまう |
| `Input`(入力) | 60回/秒(毎フレーム。ただし上限60) | しない | 同上。**押した瞬間のボタンは、直近の数パケットに重ねて入れる**(1つ落ちても取りこぼさない)。重要なボタン(決定など)は、`SendEvent` で信頼して送る |
| `Event` / `Spawn` / `Despawn` / `SharedValue` / 名簿 / シーン | 起きた時だけ | する | 1回でも落ちると、ゲームの状態がずれて戻らない |
| `Ping` | 1秒に1回 | しない | 往復の時間を測る。連続で返ってこなければ「つながりが悪い」と分かる |

- 時間の計り方: `Time::GetUnscaledDeltaTime()` を足していき、間隔を超えたら送る。フレームレートが60でも144でも、送信の回数は同じになる(3章の2)。
- ENetの同じチャンネルの「信頼しない」パケットは、**古い物が新しい物の後に届いたら自動で捨てる**(順序付き)はず(実装時に確認)。念のため、`Snapshot` には連番を付け、**受け取った連番が今までの最大より小さければ捨てる**(自分でも確かめる)。

### 7.6 Transformの補間(受け取り側で滑らかに見せる)

- クライアントは、オブジェクトごとに**直近のスナップショットを数枚**(例: 4枚)覚えます(ホストの時刻 + 位置 + 向き)。
- **表示の時刻 = 「最新のスナップショットの時刻 - 補間の遅れ」**。補間の遅れの既定は**スナップショットの間隔の2倍**(30Hzなら約0.067秒 + ゆとりで 0.1秒)。表示の時刻をはさむ2枚を見つけて、`Math::Lerp` でつなぐ。
- **向き(`rotate`、度数)**: 360度をまたぐ時(350度 → 10度)に、遠回りしないよう「近い向きに回る」補間を自作する(3章の6。`Math::Lerp` は使えない)。2Dは `rotate.z` だけ、3Dは3軸それぞれ。
- **途中でスナップショットが足りない時**: 表示の時刻が最新より後(通信が遅れた)なら、最新の位置で**止める**(予測で動かさない)。0.25秒を超えて何も届かなければ「つながりが悪い」とみなす。
- **ワープ(リスポーンなど)**: 前の位置との距離が `teleportDistance`(既定 500)を超えたら、補間せずに**その位置へ飛ぶ**。
- 補間の遅れの分だけ、クライアントの画面は**ホストの画面より少し(0.1秒ほど)遅れて**見える。これはこの方式の代償で、対戦の「操作の重さ」の原因になる(Part 4-6 が任意の対策)。協力パズルなら気にならない。

### 7.7 帯域の目安(4人)

1つのオブジェクトの `Snapshot` 内のデータ量(案): `NetworkId` 2バイト + 同期する項目の印 1バイト + 位置 12バイト(float×3) + 向き 6バイト(角度を16ビットの整数にする。float×3 のままなら12バイト) = **約21バイト**。大きさ(`scale`)は、変わった時だけ追加で送る(ほとんどの物は不変)。

| 場面 | 計算 | 結果 |
|---|---|---|
| 4人 + 物20個 = 24個を30Hzで配る(ホスト → クライアント1人あたり) | 24 × 21 + ヘッダー約10 = 約510バイト/パケット × 30 | 約**15KB/秒**(+ UDP/IPのヘッダー約40バイト × 30 = 約1.2KB/秒) |
| ホストの送信の合計(クライアント3人) | 約16KB/秒 × 3 | 約**50KB/秒**(約0.4Mbps) |
| クライアントの送信(入力。約16バイト × 60Hz) | (16 + 40) × 60 | 約**3.4KB/秒** |
| 物が100個に増えた時 | 100 × 21 = 2.1KB。1パケット(約1.4KB以下)に収まらないので**2つに分けて送る** | クライアント1人あたり約65KB/秒 |

LAN(特に有線)では、どれも**ほとんど問題にならない量**です。Wi-Fi(学校や自宅)では、量よりも**揺れ(パケットが遅れる・落ちる)**が問題になりやすいので、補間の遅れを少し長めにできるようにしておく(`Network::SetInterpolationDelay(seconds)`)。最初は float のまま送って、足りなくなってから量子化する(未確定事項 9番)。

---

## 8. 機能ごとの設計

各節は「目的 / 方式の比較とおすすめ / ファイル案 / 関数(API案) / 設計のポイント / 段階分け / 両サンプルへの影響 / 使うDemoのコード / 依存する機能 / 未確定の点」の順に書いています。「両サンプルへの影響」には、**ユーザーの了承が必要になる Engine の変更点**(変えるファイル)をまとめて書いています。

---

### 8.1 LAN通信

**目的**: 同じLANの2〜4台のPCで、1つのゲームを一緒に遊べるようにする。ホストがルールと物理を計算し、クライアントは入力を送って結果を見る。**スマブラ風の対戦**(位置の高頻度の同期)と**協力パズル**(変わった時だけ送る出来事の同期)の両方を作れる土台にする。2D(Sample2D風)でも3D(Sample3D風)でも、同じ仕組みで使えること。初心者が読んで分かる作りにすること。

**状態: Part 4-1〜4-5 と 4-7 を実装済み(Part 4-6 は先送り)。実装の記録は 13章。**

#### 方式の比較とおすすめ

**(1) 通信ライブラリ**

| 方式 | 仕組み | 良い点 | 悪い点 | 初心者向け |
|---|---|---|---|---|
| **ENet** | UDPの上に、接続・チャンネル・信頼/非信頼を作る | 小さい(Cのソースが十数個)。MIT。依存が無い。ゲーム向けで定番。1つの窓口でホスト/クライアントを作れる | IPv4中心(LANなら困らない)。マルチスレッドは自分で気を付ける | **◎** |
| Winsock(UDP)を直接 | OSの機能で自分で全部作る | 外部ライブラリが要らない | 信頼できる通信・再送・接続の管理を自作することになり、バグりやすい | × |
| Winsock(TCP) | 必ず届く通信だけ | かんたん | 1つ落ちると全部が止まる(位置の同期が遅れて溜まる)。ゲームの位置同期に向かない | △ |
| RakNet / GameNetworkingSockets など | 高機能な大きなライブラリ | 機能が多い | 大きい・ビルドが重い・今回の範囲を超える | × |

**おすすめ: ENet。** 理由: ①合意済み(ユーザーの方針) ②ソースを `Engine.vcxproj` に直接登録できる(`ufbx` と同じ形。3章の17)ので、`.lib` の Debug/Release やランタイム(`/MTd` / `/MT`)の不一致が起きない ③信頼する/しないを**パケットごとに選べる**(スマブラ風の位置と、協力パズルの出来事の両方に使える)。

**(2) 同期の方式**

| 方式 | 仕組み | 良い点 | 悪い点 |
|---|---|---|---|
| **状態同期(ホストが結果を配る)** | ホストが計算し、位置・状態を送る | PhysXが別PCで同じ結果にならなくても関係ない。ずれても次のスナップショットで直る | クライアントの画面が少し遅れる。送るデータが物の数に比例する |
| ロックステップ(入力だけ配り、みんなで同じ計算) | 全員が同じ入力で同じ計算をする | 送るデータが少ない | **PhysXで同じ結果になる保証が無い**ので使えない |
| Owner権限(自分のキャラは自分のPCで決める) | 所有者のPCが位置を決めて送る | 自分の操作が軽い | 当たり判定の不一致(ずるもできる)。LANの信頼できる相手前提なら使える(Part 4-6 の案) |

**おすすめ: 状態同期(ホストが唯一のAuthority)。** 最初はこれだけ作り、遅延対策(クライアント予測 / Owner権限)は Part 4-6(任意)で様子を見て決める。

#### ファイル案
新規(すべて新しいファイル。フォルダは 5.2 のとおり):

**Engine(`Engine/Network/`)**
- `Network.h/.cpp` : **窓口サービス**(`namespace Network`)。ホストを始める / 探す / 参加する、状態の取得、入力、出来事、スポーン。ゲームが呼ぶ関数はここに集める
- `NetworkConfig.h` : 定数を1か所に集める(`maxPlayers`、`protocolVersion`、`defaultPort`、`discoveryPort`、`snapshotRate` の既定など)
- `NetworkTransport.h/.cpp` : ENetを包むクラス(接続・送信・受信)。**ENetを知っているのはこのファイルと `LanDiscovery.cpp` だけ**
- `NetworkMessage.h/.cpp` : `PacketType`、`NetworkWriter`(数値・文字を並べて書く)、`NetworkReader`(読む。範囲外を読もうとしたら失敗を返す)
- `NetworkSession.h/.cpp` : 参加者の名簿、プレイヤー番号の割り当て、`Join` / `Accept` / `Reject`、切断の検知、ping
- `NetworkInput.h` : 入力の構造体 `NetworkInput`(軸 + ボタン)。ヘッダーだけ
- `NetworkObjects.h/.cpp` : `NetworkId → GameObject` の登録簿、プレハブの登録、`Spawn` / `Despawn`
- `NetworkIdentityComponent.h/.cpp` : 同期するオブジェクトに付ける印(`NetworkId`・所有者・Authority)
- `NetworkTransformComponent.h/.cpp` : 位置・向き(・大きさ)の同期と補間
- `LanDiscovery.h/.cpp` : LANでホストを探す(ブロードキャスト)/ ホストが「ここにいます」と答える

**ThirdParty(`ThirdParty/enet/`)**
- ENetのソース(`callbacks.c` `compress.c` `host.c` `list.c` `packet.c` `peer.c` `protocol.c` `win32.c` など。**実際の構成は取得時に確認**)、`include/enet/*.h`、`LICENSE`、`README.txt`(入手元URL・バージョン・取得日・ライセンス・どう使っているか。`ufbx` / `PhysX` の `README.txt` と同じ)

**ビルド設定**
- `Engine.vcxproj` と `Engine.vcxproj.filters` に、上の `Engine/Network/` の10組(.h だけの物を除く)と、ENetのCソースを登録(フィルター `Network` と `ThirdParty`。ENetのCは `ufbx.c` と同じ書き方: `CompileAs=CompileAsC`、`WarningLevel=TurnOffAllWarnings`、`SDLCheck=false`。`inet_ntoa` などの警告が出る物には `_WINSOCK_DEPRECATED_NO_WARNINGS` を、`PreprocessorDefinitions` に足す)
- `Engine.vcxproj` の `AdditionalIncludeDirectories`(Debug / Release の2か所。`Engine.vcxproj:52, 66`)に `$(ProjectDir)..\..\ThirdParty\enet\include` を足す
- `ws2_32.lib` と `winmm.lib` は、`NetworkTransport.cpp` と `LanDiscovery.cpp` に `#pragma comment(lib, ...)` で書く(3章の19)
- **`PhysX.props` は変えない。Sample2D / Sample3D / Demo の `.vcxproj` も変えない**(Engineが静的ライブラリなので、ENetのインクルードとリンクはEngine側で閉じる。Engineのヘッダーに `enet/enet.h` を出さないので、ほかのプロジェクトのインクルードパスは増えない)

**既存のEngineファイルの変更**: `Core/GameEngine.cpp` に3行(7.3 のとおり)。ほかの既存ファイル(`EventBus` / `SceneManager` / `Transform` / `Rigidbody*` / `ObjectPool` など)は**読むだけで、変えない**。

**Demo(Engine ではない。ユーザーの了承は不要)**: `Demo/Game/Scenes/Network/` `Objects/Network/` `Components/Network/` と、`DemoRegistry.h/.cpp`(`DemoCategory::Network` の追加と一覧の行)。詳しくは「段階分け」と「使うDemoのコード」。

#### 関数(API案)

```cpp
// ─── Engine/Network/NetworkConfig.h ─── 定数は全部ここ(最大人数を変える時は1か所)
namespace Network {
    static constexpr int      maxPlayers      = 4;       // 最大人数(ホスト含む)。2〜4で試せる
    static constexpr int      protocolVersion = 1;       // 通信の仕様の版。違う版とはつながらない(Rejectする)
    static constexpr int      defaultPort     = 47200;   // ゲーム用のUDPポート(案。実装時に決める)
    static constexpr int      discoveryPort   = 47201;   // ホスト探索用のUDPポート(案)
    static constexpr int      defaultSnapshotRate = 30;  // 位置の送信回数/秒
    static constexpr float    defaultInterpolationDelay = 0.1f;   // 補間の遅れ(秒)
}

// ─── Engine/Network/Network.h ─── ゲームが呼ぶ窓口(namespace。Input / Audio / Time と同じ形で、Get() は無い)
enum class NetworkState { Offline, Hosting, Connecting, Connected };   // Connected = クライアントとして参加中
enum class NetworkTarget { Host, All, Others };   // 出来事の送り先(All = 自分を含む全員、Others = 自分以外)

struct NetworkStats {             // 確認用の数(HUDに出す)
    int bytesSent = 0, bytesReceived = 0;       // 直近1秒の送受信バイト数
    int packetsSent = 0, packetsReceived = 0;   // 直近1秒の送受信パケット数
};

struct NetworkHostInfo {          // LANで見つかったホスト1台分
    std::string name;             // ホスト名(表示用)
    std::string address;          // "192.168.0.12" のようなIPアドレス
    int port = 0;
    int playerCount = 0;          // 今の人数
    int maxPlayers = 0;
};

namespace Network {
    // ─── 準備(Scene::Init より前、または最初のシーンで) ───
    void SetGameId(const std::string& gameId);   // 同じIDのゲームどうしだけつながる("Demo" など。ほかの人のゲームと混ざらない)

    // ─── ホストになる / 探す / 参加する / やめる ───
    bool StartHost(const std::string& playerName, int port = defaultPort);   // 部屋を作る。自分は playerIndex 0。失敗(ポート使用中など)はfalse
    void StartSearch();                                        // LANのホスト探索を始める(ブロードキャストを一定間隔で送る)
    void StopSearch();
    const std::vector<NetworkHostInfo>& GetFoundHosts();       // 見つかったホスト一覧(毎フレーム更新される。消えたホストは数秒で消える)
    bool Join(const std::string& address, int port, const std::string& playerName);   // 参加を申し込む(結果は GetState() / イベントで分かる)
    void Disconnect();                                         // 抜ける / 部屋を閉じる(ホストなら全員が切断される)

    // ─── 状態を調べる ───
    NetworkState GetState();
    bool IsOnline();               // ホスト中または参加中
    bool IsHost();                 // ホスト中か。オフラインの時も true(1人で遊ぶ時はあなたがホスト扱い。5.3)
    bool IsClient();               // 参加中か
    int  GetLocalPlayerIndex();    // 自分のプレイヤー番号(オフライン・ホストは 0)
    int  GetPlayerCount();         // 今の人数
    bool IsPlayerConnected(int playerIndex);
    std::string GetPlayerName(int playerIndex);
    float GetPing();               // ホストとの往復の時間(秒)。ホストは 0
    NetworkStats GetStats();       // 送受信のバイト数・パケット数(確認用)

    // ─── 入力(クライアントは送る。ホストは受け取る。ホスト自身の入力もここを通る) ───
    void SetLocalInput(const NetworkInput& input);   // 自分の入力を渡す(毎フレーム。ゲーム側のComponentが呼ぶ)
    const NetworkInput& GetInput(int playerIndex);   // そのプレイヤーの最新の入力(ホストが読む。オフラインの 0 番は SetLocalInput の値)

    // ─── 出来事(信頼して送る) ───
    void SendEvent(const std::string& name, NetworkTarget target = NetworkTarget::All);                       // 中身の無い出来事。受け取り側では EventBus::Publish(name) も呼ばれる
    void SendEvent(const std::string& name, const NetworkWriter& payload, NetworkTarget target = NetworkTarget::All);   // 中身つき
    using EventHandler = std::function<void(NetworkReader& payload, int senderPlayerIndex)>;
    void Subscribe(const std::string& name, EventHandler handler);   // 受け取る(Scene::Uninit で ClearHandlers する。EventBus と同じ決まり)
    void ClearHandlers();

    // ─── 共有の値(「今の状態」。ホストが変えると全員に届き、途中参加の人にも渡る) ───
    void SetSharedValue(const std::string& key, int value);          // ホストだけが呼べる(クライアントが呼ぶと出力ウィンドウに知らせて何もしない)
    int  GetSharedValue(const std::string& key, int defaultValue = 0);

    // ─── プレイヤーの出入り(ゲームが反応したい時) ───
    void SetPlayerJoinedHandler(std::function<void(int playerIndex)> handler);
    void SetPlayerLeftHandler(std::function<void(int playerIndex)> handler);
    // EventBus にも "NetworkPlayerJoined" / "NetworkPlayerLeft" / "NetworkConnected" / "NetworkDisconnected" を Publish する(引数なし。誰かは GetPlayerCount などで調べる)

    // ─── スポーンとシーン(Part 4-5) ───
    using PrefabFactory = std::function<GameObject*(Scene& scene, const NetworkSpawnInfo& info)>;
    void RegisterPrefab(int prefabId, PrefabFactory factory);        // 「この番号はこの工場関数で作る」を登録(ホストもクライアントも同じ登録をする)
    GameObject* Spawn(Scene& scene, int prefabId, int ownerPlayerIndex, XMFLOAT3 position);   // ホストだけ。作って全員に知らせる
    void Despawn(GameObject* object);                                // ホストだけ。消して全員に知らせる(プールに返す物は SetDespawnHandler)
    void SetDespawnHandler(int prefabId, std::function<void(GameObject*)> handler);   // 既定は object->Destroy()。ObjectPool の物は Return を入れる
    void RegisterScene(int sceneId, std::function<void()> changeScene);   // 「この番号のシーンへの切り替え方」を登録([]{ SceneManager::Get().ChangeScene<GameScene>(); } など)
    void ChangeSceneForAll(int sceneId);                             // ホストだけ。全員をそのシーンへ
    void NotifySceneReady(int sceneId);                              // 各シーンの Init の最後で呼ぶ(「準備できた」。ホストはこれを待って初期状態を送る)

    // ─── 調整 ───
    void SetSnapshotRate(int hz);                  // 10〜60
    void SetInterpolationDelay(float seconds);     // 補間の遅れ
    void SetDebugLogEnabled(bool enabled);         // 送受信のパケットの種類を出力ウィンドウに出す(既定オフ)

    // ─── Engine内部用(GameEngineだけが呼ぶ。ゲーム側からは呼ばない) ───
    void Update();      // 受信(Input::Update の直後)
    void EndFrame();    // 送信(Audio::Update の後)
    void Shutdown();    // 終了(SceneManager::Uninit の後)
}

// ─── Engine/Network/NetworkInput.h ─── 入力の中身(2D/3Dどちらにも使える汎用の形)
struct NetworkInput {
    float moveX = 0.f, moveY = 0.f;     // 移動の軸(-1〜1)。2Dは (左右, 上下)、3Dは (左右, 前後)
    float lookX = 0.f, lookY = 0.f;     // 視点・狙いの軸(-1〜1)。使わなければ 0
    unsigned short buttons = 0;         // ボタンの押している状態(ビット。最大16個)
    unsigned short pressed = 0;         // 「押した瞬間」のビット(ホストが読むまで溜まる)

    void SetButton(int bit, bool down);          // bit は 0〜15(ゲーム側で enum に決める: 0=Jump, 1=Attack ...)
    bool IsButtonDown(int bit) const;
    bool WasButtonPressed(int bit) const;
};

// ─── Engine/Network/NetworkIdentityComponent.h ─── 同期するオブジェクトに付ける印
class NetworkIdentityComponent : public Component {
public:
    void SetSceneId(int id);              // シーンに最初から置く物の番号(1〜999。ホストとクライアントで同じ番号にする)
    int  GetNetworkId() const;            // 0 = まだ番号が無い。スポーンした物は 1000 以上(ホストが付ける)
    int  GetOwnerPlayerIndex() const;     // 所有者(この物を操作するプレイヤー)。-1 = 誰のでもない(ホストの物)
    bool IsOwner() const;                 // 所有者が自分か
    bool HasAuthority() const;            // 本当の状態を決める側か(ホスト、またはオフライン)。クライアントは false
    void Uninit() override;               // 登録簿から外す
    void OnActiveChanged(bool active) override;   // 非アクティブ(プールに返した)になったら登録簿から外す
};

// ─── Engine/Network/NetworkTransformComponent.h ─── 位置・向き・大きさの同期
struct NetworkTransformSettings {
    bool syncPosition = true;
    bool syncRotation = true;
    bool syncScale = false;               // 大きさは変わる物だけ true(変わった時だけ送る)
    bool is2D = false;                    // true なら position.xy と rotate.z だけ送る(量が減る)
    float teleportDistance = 500.f;       // これ以上離れたら補間せずに飛ぶ
};

class NetworkTransformComponent : public Component {
public:
    explicit NetworkTransformComponent(const NetworkTransformSettings& settings = NetworkTransformSettings{});
    // ホスト(Authority): EndFrame の時に Transform を読んでスナップショットに入れる。クライアント: Network::Update の時に補間して Transform に書く
    // (Update では何もしない。順序の影響を受けないよう、Network が登録簿を通じて呼ぶ。設計のポイント(g))
    void Uninit() override;
};

// ─── スポーン用(Part 4-5) ───
struct NetworkSpawnInfo {
    int networkId = 0;
    int prefabId = 0;
    int ownerPlayerIndex = -1;
    XMFLOAT3 position{};
    XMFLOAT3 rotate{};
    bool hasAuthority = false;            // この PC が本当の状態を決める側か(工場関数が Dynamic / Kinematic を選ぶ)
};
```

使い方の約束:
- **関数名は動詞から始める**(`StartHost` `Join` `Disconnect` `SendEvent` `Subscribe` `Spawn`)。`bool` を返す物は `Is○○` / `Has○○`。`Get○○` / `Set○○`。定数は `static constexpr` の camelCase(`maxPlayers`)。`enum class` の値は PascalCase(`NetworkState::Hosting`)。出来事の名前は PascalCase の文字列(`"DoorOpened"`)。Componentの語尾は `○○Component`(どのオブジェクトにも付けられる汎用の機能。`RigidbodyComponent` と同じ)。
- ホストの物にも、クライアントのコピーにも、**同じComponentを付ける**(違いは `HasAuthority()` が決める)。

#### 設計のポイント(落とし穴を含む)

**(a) `Network` を「アプリ全体で1つのサービス」にしてよいか(`CLAUDE.md` との整合)**
- `CLAUDE.md` は「シングルトン(`Get()`)はアプリ全体で1つのサービス(EventBus/SceneManager/Input/Audio)だけ」と決めています。`Network` も**アプリ全体で1つしか要らない**(1つのPCが同時に2つの部屋に入ることは無い)、**シーンをまたいで残る**(ロビーのシーンで接続し、ゲームのシーンで遊ぶ)ので、条件に合います。
- 形は **`namespace Network`(関数の集まり)**にします(`Input` / `Audio` / `Time` / `Physics` と同じ。`Get()` は付けない。3章の16)。中の状態は `Network.cpp` の無名名前空間に隠します。`Network::Get()` というシングルトンのクラスにしない理由: ゲーム側が書く `Network::IsHost()` が短く読めるのと、`Get()` のシングルトンを増やさない方針に合うため。
- `CLAUDE.md` の一覧に `Network` を足す文言の変更は、**ユーザー本人の了承が必要**です(未確定事項 3番)。了承が出るまで、`CLAUDE.md` は変えません。
- 一方、`NetworkTransport` / `NetworkSession` は**普通のクラス**(`new` できる)にして、`Network` はそれを1つずつ持つだけにします。これはテストのためで、サービス(シングルトン)ではありません。

**(b) 誰が何を決めるか(Authority)**
- **ホスト = 全オブジェクトのAuthority**。ホストだけが、①入力を読んで物を動かす ②当たり判定のコールバックに反応する ③`SetSharedValue` を変える ④`Spawn` / `Despawn` する、を行う。
- クライアントは、①入力を送る ②受け取った状態を表示する ③出来事を送る(`SendEvent(..., NetworkTarget::Host)`: 「スイッチを押した」とホストに知らせる。ホストが判断して、結果を `SharedValue` や `Event` で返す) ④見た目だけの演出(音・パーティクル)を、出来事を受けて出す、だけ。
- **ゲームのルールの書き方の約束**: ルールを書く Component の `Update` / `OnTriggerEnter` の最初に `if (!identity->HasAuthority()) return;` と書く(クライアントではルールを動かさない)。見た目だけの Component は、このチェックをしない。

**(c) クライアント側のコピーの物理(Dynamicにしない)**
- 3章の 7・8 のとおり、クライアントの見せるだけのコピーは **Kinematic にするか、Rigidbodyを付けない**。**Dynamicにすると、クライアントのPhysXも勝手に物を動かし(落ちる・転がる)、ホストの位置とずれる。**また、Dynamicなオブジェクトの Transform を `NetworkTransformComponent` が直接書き換えるのは、CLAUDE.md の決まりに反する。
- 工場関数の書き方(`NetworkSpawnInfo::hasAuthority` を見て選ぶ。「使うDemoのコード」の例): `BodyType body = info.hasAuthority ? BodyType::Dynamic : BodyType::Kinematic;`。シーンに最初から置く物も同じ(`Create○○(Scene&, bool hasAuthority)` の引数で渡す。`hasAuthority` の値は `Network::IsHost()` を見る)。
- **Kinematic のコピーと、クライアントに残るDynamicな物の関係**: クライアントのPhysXは動いているので、クライアントにだけ存在するDynamicな物(見た目だけの破片など)は、Kinematicのコピーに押される。ゲームの状態に関わるDynamicな物は**ホストの物のコピーとして、必ずKinematicか非物理**にする(約束)。
- PhysX は、Kinematic が Transform の位置を急に変えると「ワープ」として扱う。補間で毎フレーム少しずつ動かすので、通常は問題にならない。

**(d) 途中退出・切断の扱い**
- **クライアントが落ちた / 抜けた**: ホストは `PlayerLeft` を全員に送る。そのプレイヤーが所有するオブジェクト(`ownerPlayerIndex` が一致する物)は、**既定で自動 `Despawn`**(`SetPlayerLeftHandler` で自分で処理することもできる。未確定事項 12番)。名簿の空きができ、**次に参加した人が、一番若い空き番号**をもらう。
- **ホストが落ちた / 閉じた**: クライアントは「ホストが消えた」ことを、①ENetの切断の通知 ②一定時間(タイムアウト)パケットが来ない、のどちらかで知る。`NetworkState::Offline` に戻り、`EventBus::Publish("NetworkDisconnected")` を呼ぶ。**ホストの引き継ぎ(ホスト移譲)は作らない**(全員セッション終了)。ゲーム側は、`"NetworkDisconnected"` を受けて「タイトルに戻る」などをする。
- **タイムアウトは長め(10秒程度)**にする(3章の4。ウィンドウをドラッグ中やデバッガの停止で、一時的に応答できなくなるため)。ENetの `enet_peer_timeout` で設定する(実装時に値を決める)。
- 切断された時の同期オブジェクトは、シーンにそのまま残る(Networkが勝手に消さない)。シーンを切り替えれば、`Uninit` で登録簿から外れる。

**(e) 信頼/非信頼とチャンネル(ENet)**
- 7.4 の表のとおり、チャンネルは 0(信頼)/ 1(入力・ping、信頼しない)/ 2(スナップショット、信頼しない)。
- ENetの「信頼しない」の既定は**順序付き**(同じチャンネルで古いパケットが後から届いたら捨てる)なので、スナップショットには向く。ただし**パケットが大きくなると**(1つの `MTU` 約1.4KB を超えると)ENetは自動で分割して送り、**分割のどれか1つが落ちると全体が捨てられる**。→ スナップショットは、**1つのパケットが約1.2KBを超えないよう、物の数で複数のパケットに分けて**送る(`Snapshot` の中に「この組の何個目 / 全部で何個」を入れる)。
- 受け取った側は、`Snapshot` の連番で古い物を捨てる(7.5)。**知らない `NetworkId` が入っていたら無視する**(スポーンのパケットより先に届くことがある。信頼するチャンネル0と、信頼しないチャンネル2の間では、届く順が保証されない)。

**(f) `NetworkId` の付け方**
- **シーンに最初から置く物**: ゲーム側が `NetworkIdentityComponent::SetSceneId(10)` のように**番号を明示**する(1〜999)。ホストとクライアントが**同じシーンのコード(`Init`)**で同じ番号を付けるので、一致する。「作った順で自動で付ける」案は、`Init` の中で `Network::IsHost()` によって作る物が変わると番号がずれるので、使わない(この設計での案)。
- **ゲーム中にスポーンした物**: ホストが 1000 以上の番号を付けて、`Spawn` パケットで全員に知らせる。番号は16ビット(最大65535)。消した番号は、**しばらく(数秒)使わない**ようにして再利用する(古い `Snapshot` が、新しい物に当たらないように)。
- `NetworkObjects` が `NetworkId → GameObject*` の登録簿を持つ。`NetworkIdentityComponent` の `Init` / `Uninit` / `OnActiveChanged` で登録・解除する。**シーンが変わる(`Uninit`)と、登録簿から自動で消える**ので、前のシーンのポインタが残ることは無い。

**(g) 受信したTransformを、いつ・どこで書くか(順序の落とし穴)**
- `NetworkTransformComponent::Update` の中で書くと、**同じオブジェクトの `RigidbodyComponent`(Kinematic)の `Update` との順番(`AddComponent` した順)で、1フレーム遅れる**ことがある(Kinematic の反映が先に走ると、古い位置が PhysX に渡る)。
- そのため、**`Network::Update`(`Input::Update` の直後、`Physics::Update` の前)で、登録簿を通じて全ての `NetworkTransformComponent` に「補間して書く」を呼ぶ**。`Network::Update` はシーンの `Update` より前なので、書いた値が、同じフレームの `RigidbodyComponent::Update` / 描画に反映される。
- ホスト側の「読む」は、逆に**シーンの `Update` の後(`Network::EndFrame`)**で行う(3章の1)。

**(h) 入力(`NetworkInput`)の流れ**
- ゲーム側に、**ローカルの入力を集める Component**(例: `PlayerInputController`)を作る。`Update` で `Input::GetKeyPress(KEY_A)` などを読み、`NetworkInput` に詰めて `Network::SetLocalInput(input)` を呼ぶ(毎フレーム)。キーの割り当てはゲームによって違うので、この Component は **Engine ではなくゲーム側(Demo / Sample の `Components/`)**に置く。
- クライアントでは、`Network::EndFrame` が最新の `NetworkInput` を `Input` パケットで送る。ホストは、受け取った入力を**プレイヤー番号ごとに**保存する。ホスト自身のプレイヤー(0)は、`SetLocalInput` の値がそのまま `GetInput(0)` になる。
- ホストの `PlayerController`(ホストだけが動かす)が、`Network::GetInput(playerIndex)` を読んで `RigidbodyComponent::SetVelocity()` で動かす。**Transformを直接書き換えない**(CLAUDE.md の約束。3章の8)。
- `pressed`(押した瞬間)は、ホストが `GetInput` を読むまで溜める(1フレームに複数回押しても、`WasButtonPressed` は取りこぼさない)。クライアント側では、直近の数パケットに重ねて送る(7.5)。
- 入力が一定時間(例: 1秒)届かなかったら、ホストはそのプレイヤーの入力を**ゼロに戻す**(ボタンを押しっぱなしのまま止まらない、を防ぐ)。
- 順序の落とし穴: ホストの `PlayerController` と `PlayerInputController` が同じオブジェクトにある場合、**`AddComponent` の順で `Update` が回る**。`PlayerInputController` を先に付ける(遅れても1フレームだけなので、致命的ではない)。

**(i) 出来事(`SendEvent`)と EventBus の連携**
- `SendEvent(name, payload, target)` は、信頼できるチャンネルで送る。`target`: `Host`(ホストだけ。クライアントの「スイッチを押した」)/ `All`(自分を含む全員。ホストが「扉が開いた」)/ `Others`(自分以外)。クライアントが `All` を指定したら、ホストが受け取って全員へ中継する(送った本人にも戻る)。
- 受け取る側: `Network::Subscribe("DoorOpened", [this](NetworkReader& r, int sender) { ... })`。**中身(ペイロード)が空の出来事は、`EventBus::Publish(name)` も自動で呼ぶ**ので、既存の `EventBus` を使うゲームのコード(`EventBus::Get().Subscribe("DoorOpened", ...)`)が、ネットワーク越しでもそのまま動く。中身のある出来事は、`EventBus` が引数を運べない(3章の10)ので、`Network::Subscribe` を使う。
- **`this` をキャプチャしたラムダを登録したら、Sceneの `Uninit()` で `Network::ClearHandlers()`**(`EventBus::Clear()` と同じ決まり。`EventBus.h:25-28` の「dangling pointer」の注意と同じ)。
- 出来事の名前は文字列で送る(1パケット数バイト〜数十バイトで、頻度が低いので許容)。送る名前の長さは 32 文字までに制限する。

**(j) 共有の値(`SetSharedValue`)と途中参加**
- 協力パズルの「扉は開いている」「スイッチ3は押されている」のような**今の状態**は、`Event` だけだと、**途中から参加した人が知らない**(出来事は過去に起きたことなので)。そこで、ホストが `SetSharedValue("Door1", 1)` で値を持つ。変わる度に全員へ信頼して送り、**途中参加の人には `Accept` の後でまとめて送る**。
- 見た目(扉が開いている)は、`GetSharedValue("Door1")` を毎フレーム見る、または `"SharedValueChanged"` の出来事を受けて更新する。出来事は「その瞬間の演出(音)」、共有の値は「今の状態」、と使い分ける。

**(k) スポーン / 破棄と `ObjectPool`**
- 準備: `Network::RegisterPrefab(prefabId, 工場関数)` を、ホストもクライアントも**シーンの `Init` の最初に同じ内容で**呼ぶ。工場関数は `Create○○(Scene&, const NetworkSpawnInfo&)` の形(CLAUDE.md の `Create○○`)。
- `Spawn`(ホストだけ): `NetworkId` を決めて工場関数を呼び、`Spawn` パケットを全員に送る。クライアントは `Spawn` を受けたら同じ工場関数を呼ぶ(`hasAuthority = false`)。作った物には、工場関数が `NetworkIdentityComponent` と `NetworkTransformComponent` を付ける。
- `Despawn`(ホストだけ): 登録簿から外し、`SetDespawnHandler` の関数(既定は `Destroy()`)を呼び、`Despawn` パケットを送る。`ObjectPool` の物は、`SetDespawnHandler(prefabId, [&](GameObject* o){ pool.Return(o); })` のように `Return` を入れる。
- **`ObjectPool` と組み合わせる時**: `Rent` で借りた物は前の値が残っている(3章の15)ので、工場関数ではなく**借りた直後の「初期化」で、位置・`NetworkId`・所有者を毎回付け直す**。`Return`(`SetActive(false)`)されたら、`NetworkIdentityComponent::OnActiveChanged(false)` が登録簿から外す。
- プレハブの番号 → 工場関数の登録は、プログラムの途中で変えない(シーンごとに登録し直す)。ゲーム側は `namespace Prefab { static constexpr int player = 1; ... }` のように番号に名前を付ける。

**(l) シーンの切り替えと接続**
- **接続はシーンをまたいで残る**(`Network` はアプリ全体で1つ)。ロビーのシーンで接続し、ゲームのシーンへ移っても切れない。
- ホストが `ChangeSceneForAll(sceneId)` を呼ぶと、`SceneChange` が全員に届き、各PCで登録された関数(`RegisterScene`)が `SceneManager::Get().ChangeScene<T>()` を呼ぶ。`SceneManager` が切り替え中に呼ばれた `ChangeScene` を無視する(3章の11)ので、Network は**切り替えが終わるのを待ってから**呼ぶ。
- 各シーンの `Init` の最後で `Network::NotifySceneReady(sceneId)` を呼ぶ。クライアントの `SceneReady` を受けたホストが、そのクライアントに**今のシーンの初期状態**(`Spawn` の一覧と `SharedValue`)を送る(途中参加のときも同じ道を通る)。
- シーンの切り替え中に届いた `Spawn` / `Snapshot` は、新しいシーンの準備ができる(`NotifySceneReady`)まで**捨てるか保留**する。古いシーンの `NetworkId` に新しいシーンの物が当たらないように、`SceneChange` ごとに「シーンの世代番号」を進め、`Snapshot` / `Spawn` に世代を入れて比べる。
- 切り替えの演出(`FadeTransition`)で `Time` の倍率が0になる間も、通信は止めない(3章の3)。
- `Scene::Uninit` では、`EventBus::Clear()` に加えて `Network::ClearHandlers()` を呼ぶ。同期オブジェクトの登録簿からは、各Componentの `Uninit` が自動で外す。

**(m) LANでホストを探す(`LanDiscovery`)**
- ゲーム本体のポートとは**別のUDPポート**(`discoveryPort`)を使う。ホストは、このポートを待ち受け(ENetのソケットのAPIを使う。生のWinsockを触らない)、`DiscoveryQuery` を受けたら、**送ってきた相手だけ**に `DiscoveryReply`(ゲームID・ポート・ホスト名・人数)を返す。
- 探す側は、`255.255.255.255`(全体向けブロードキャスト)に `DiscoveryQuery` を1秒おきに送り、`DiscoveryReply` を `GetFoundHosts()` に溜める(数秒返事が無いホストは消す)。
- **ゲームIDが違う物(ほかの人が同じLANで動かしている別のゲーム)は無視する**。
- 落とし穴(11章): ①学校や公共のWi-Fiは、**ブロードキャストや端末どうしの通信を禁止している**ことがある(AP isolation)。その場合は探索が動かない → **IPアドレスを手入力で直接つなぐ**(`Join(address, port, name)`)を、必ず残す。②複数のネットワークアダプタ(VPN・仮想アダプタ)があると、`255.255.255.255` は1つにしか出ない場合がある → 実装時に、各アダプタの「サブネットのブロードキャスト」(`192.168.0.255` など)に送る案を確かめる。③ホスト画面には、**自分のIPアドレスを表示する**(手入力で参加する人に教えるため)。
- 1台で2つ起動して試す時、2つ目は**ホストにならずクライアントとして**探す/参加する(同じポートを2つのプロセスが待ち受けられない)。

**(n) 安全性(信頼できるLANの相手が前提)**
- インターネットに出ることは考えない。**同じLANの信頼できる相手が前提**で、ずる(入力の偽装・位置の偽装)や、暗号化・認証は作らない。
- ただし、**壊れたパケットでクラッシュしない**ことは守る: `NetworkReader` は範囲外を読まず失敗を返す / パケットのサイズ・種類・プレイヤー番号・オブジェクトの数・文字列の長さの上限を検査する / 浮動小数点は `isfinite` を確かめ、位置は範囲(±1,000,000)に収める / 知らない `NetworkId` は無視する。クライアントの `Input` の送り主は、ホストがENetの接続から決める(パケット内のプレイヤー番号を信用しない)。

**(o) テストしやすさ**
- `NetworkTransport` / `NetworkSession` / `NetworkWriter` / `NetworkReader` は、`Network` の外で `new` できる。**画面もウィンドウも無いスクラッチテスト**で、1つのプログラムの中にホスト役とクライアント役を作り、`127.0.0.1` でつないで、①参加・断り・切断 ②パケットの書き込み/読み取りの往復 ③不正なパケットを送っても落ちない ④スナップショットの補間の計算(時刻と位置から期待の値)を確かめる(ロードマップ3の確認と同じやり方)。
- ゲームの見え方・操作感(ラグ・補間の滑らかさ)は、実際に動かさないと分からないので、**ユーザーに2台(または1台で2つ)で確認**してもらう。

#### 段階分け

| Part | 内容 | 作る物 | 変えるEngineファイル | 確認方法(Demo) |
|---|---|---|---|---|
| **4-1 ENet導入と接続・メッセージ送受信** | ENetをビルドに入れる。`StartHost` / `Join` / `Disconnect`。文字(テキスト)を送り合う。`Ping` / `Pong` | `ThirdParty/enet/`、`NetworkConfig.h`、`NetworkTransport`、`NetworkMessage`(`NetworkWriter` / `NetworkReader`)、`Network`(一部)、`GameEngine.cpp` に3行 | `Engine.vcxproj(.filters)`、`GameEngine.cpp` | Demoに `Network` カテゴリと **`NetworkBasic`** デモ(ボタン: Host / Join 127.0.0.1 / Send / Disconnect。接続状態・ping・受け取った文字を表示)。1台で2つ起動して確認 |
| **4-2 セッション** | プレイヤー番号・参加者名簿・満員/バージョン違いの断り・切断の検知・LANホスト探索 | `NetworkSession`、`LanDiscovery`、`Network`(`StartSearch` `GetFoundHosts` `GetPlayerCount` ほか)、`EventBus` への通知 | なし(`Engine.vcxproj(.filters)` の登録のみ) | `NetworkBasic` を拡張: ロビー画面(見つかったホストの一覧・参加者名簿)。1台で2つ / 2台のLANで探索 |
| **4-3 NetworkTransform** | 位置・向きの同期と補間。シーンに最初から置いた物だけ。ホストがAuthority | `NetworkIdentityComponent`、`NetworkTransformComponent`、`NetworkObjects`(登録簿)、スナップショットの送受信 | なし(登録のみ) | **`NetworkTransform`** デモ(2D版・3D版): ホストが動かす箱を、クライアントが補間して表示。ホスト側のDynamicな球が転がる様子がクライアントに映る。`GetStats()` で帯域を表示 |
| **4-4 入力と出来事** | `NetworkInput` の送信、`SendEvent` / `Subscribe` / EventBus連携、`SetSharedValue` | `NetworkInput.h`、`Network`(入力・出来事・共有の値) | なし(登録のみ) | **`NetworkInput`** デモ(クライアントの入力でホスト側の箱が動く: スマブラ風の最小形)と、**`NetworkEvent`** デモ(スイッチを押す → 扉が開く。途中参加でも扉が開いたまま: 協力パズルの最小形) |
| **4-5 スポーン・シーン同期** | `Spawn` / `Despawn`、`ObjectPool` との組み合わせ、`ChangeSceneForAll`、途中参加の初期状態送信 | `NetworkObjects`(プレハブ・スポーン)、`Network`(スポーン・シーン) | なし(登録のみ) | **`NetworkSpawn`** デモ: ホストが球を撃ち出す(プール使用)、全員の画面に出て消える。ロビー → ゲームのシーン移動を全員で行う。途中参加の人にも今の物が見える |
| **4-6 (任意)遅延対策** | A: クライアント予測(自分のキャラだけ先に動かし、ホストの結果で補正)。B: Owner権限(自分のキャラの位置は自分で決める)。**LANで遊んでから、必要ならどちらかを選ぶ** | `NetworkTransformComponent` の拡張(`NetworkAuthority::Owner` など) | なし(登録のみ) | `NetworkInput` デモに「予測 ON/OFF」の切り替えを付けて、操作の軽さを比べる |
| **4-7 Demoの仕上げ** | 2D版・3D版をそろえる。操作説明・エラー表示。README・EngineGuide | Demo の各シーン・ボタン、README の節、EngineGuide の `Network` ページ | なし | 全デモを2台 / 1台で通して動かす |

- **Demoの追加**: Part 4-1 で `DemoCategory` に `Network` を足し(`DemoRegistry.h:9`)、デモごとに `DemoRegistry.cpp` の一覧へ1行足す(例: `{ DemoCategory::Network, L"NetworkBasic", L"2台でつなぐ・文字を送る", &OpenScene<NetworkBasicDemoScene>, &OpenScene<NetworkBasicDemoScene> }`。2D/3D の区別が無いデモは両方に同じ関数を入れる)。`DemoGuideController::ResetSettings` に、デモを抜ける時の `Network::Disconnect()` と `Network::ClearHandlers()` を足す(Demoの `Uninit` で必ず切る)。
- **段階ごとに、ユーザーが2台で確認**してから次へ進む(1台でのテストでは、遅延が0に近く、ラグの問題が見えないので、Part 4-3 以降は **LANの2台での確認を1度は行う**ことをおすすめする)。

#### 1台でのテスト、LANの2台でのテスト

| 方法 | 手順 | 分かること | 分からないこと |
|---|---|---|---|
| **1台で2つ起動(localhost)** | ①ビルドした `Demo.exe` を**2回起動**する(Visual Studioのデバッグ起動を1つ + エクスプローラーから `Demo.exe` をもう1つ、など) ②片方で Host ボタン、もう片方で `Join 127.0.0.1` ボタン(**マウスで押す**。キーは選んだウィンドウだけに入る。3章の20) ③ウィンドウを横に並べる | 接続・参加・断り・同期・補間・出来事・スポーンの**動作** | 遅延・パケットが落ちる状況(localhostでは起きない)・ファイアウォールの問題・ホスト探索(2つ目はホストを探せるが、同じPCなので参考程度) |
| **LANの2台** | ①2台を同じWi-Fi / 有線につなぐ ②1台でホスト。画面に出るIPアドレスを確認 ③もう1台で `Find`(探索)または IP手入力で参加 ④**初回はWindowsのファイアウォールの確認が出る** → 「プライベートネットワーク」を許可(11章) | ラグ・補間の滑らかさ・本当の操作感・ファイアウォール・ホスト探索 | |

- 1台で2つ起動した時、**選んでいない側のウィンドウがキー入力を受け取るか**は実装時に確かめる(3章の20)。受け取らない場合、**操作の確認は「ウィンドウを切り替えながら」**行う(片方を操作して、もう片方で見る。ホストのキャラを操作してクライアントの画面で見る、が基本の確認)。
- Debug のデバッガでブレークすると通信が止まって切断されるので、**ネットワークの確認はデバッガ無し(Releaseの `Demo.exe` の起動)**でもできるようにしておく。

#### 両サンプルへの影響(変えるEngineファイル。了承が必要)
- **新規**: `Engine/Network/` の10組ほど、`ThirdParty/enet/`(Engineの外)
- **既存のEngineファイルの変更**: `Engine.vcxproj` / `Engine.vcxproj.filters`(新ファイルとENetのCソースの登録、インクルードパスの追加)、`Core/GameEngine.cpp`(`Network::Update()` / `Network::EndFrame()` / `Network::Shutdown()` の3行 + `#include "../Network/Network.h"`)
- `Shader*.hlsl` と `PhysX.props` は**変えません**。`EventBus` / `SceneManager` / `Transform` / `RigidbodyComponent` / `ObjectPool` などは**読むだけ**です。
- **Sample2D / Sample3D**: `Network::Update` / `EndFrame` の先頭で「オフラインなら何もしない」ので、**見た目も動きも変わりません**(`if` 1つぶんのわずかな追加だけ)。ソースは変えません。`Engine.lib` が少し大きくなり、`ws2_32.lib` / `winmm.lib` がリンクされます(どちらもWindowsの標準)。
- **Demo**: `Network` カテゴリの追加(`DemoRegistry.h/.cpp`、`ResetSettings`)と、デモのファイルの追加。Engineではないので、了承の対象ではありません。

#### 使うDemoのコード

```cpp
// ─── Demo/Game/Scenes/Network/NetworkTransformDemo3DScene.cpp(新規。Part 4-3)───
// Sceneには「何を置くか」だけを書く(CLAUDE.md)。ルールと動きは各オブジェクトの Component に書く
void NetworkTransformDemo3DScene::Init() {
    CreateDemoGuide(*this, { /* 説明 4 行 */ }, OpenNetworkCategory);
    CreateDemoLights(*this);
    CreateDemoGround(*this);
    CreateNetworkLobby(*this);               // Host / Join / Find / Disconnect のボタンと、名簿・ping の表示
    CreateNetworkBox(*this, 10, {0, 100, 0});    // 同期する箱。10 は NetworkId(シーンに置く物の番号)
}

// ─── Demo/Game/Scenes/Network/... ─── Objects: 同期する箱(新規)───
GameObject* CreateNetworkBox(Scene& scene, int sceneId, XMFLOAT3 position) {
    bool hasAuthority = Network::IsHost();        // ホスト(とオフライン)が本当の状態を決める
    GameObject* box = scene.CreateObject("NetworkBox", "Box");
    box->transform.position = position;
    box->transform.scale = { 100.f, 100.f, 100.f };
    box->AddComponent<MeshRenderer>(Mesh::CreateCube());
    // ホストはDynamic(PhysXが計算)、クライアントのコピーはKinematic(Transformを書き換えて動かす)
    BodyType body = hasAuthority ? BodyType::Dynamic : BodyType::Kinematic;
    box->AddComponent<BoxRigidbodyComponent>(body, XMFLOAT3{ 100.f, 100.f, 100.f });
    box->AddComponent<NetworkIdentityComponent>()->SetSceneId(sceneId);
    box->AddComponent<NetworkTransformComponent>();   // 位置・向きを同期(ホスト → クライアント)
    return box;
}

// ─── ゲームの移動(ホストだけが動かす。Demo/.../Components/Network/NetworkBoxController.cpp)───
void NetworkBoxController::Update(float dt) {
    if (!identity->HasAuthority()) return;                                    // クライアントではルールを動かさない
    const NetworkInput& in = Network::GetInput(identity->GetOwnerPlayerIndex());   // ホスト自身もクライアントも同じ読み方
    rigidbody->SetVelocity({ in.moveX * speed, currentY, in.moveY * speed });      // Transformは直接書かず SetVelocity で動かす
}

// ─── ローカルの入力を集める(ゲーム側。Demo/.../PlayerInputController.cpp)───
void PlayerInputController::Update(float dt) {
    NetworkInput input;
    if (Input::GetKeyPress(KEY_A)) input.moveX -= 1.f;
    if (Input::GetKeyPress(KEY_D)) input.moveX += 1.f;
    input.SetButton(0, Input::GetKeyPress(KEY_SPACE));        // ボタン 0 = Jump(ゲーム側で決める)
    Network::SetLocalInput(input);                            // ホストなら自分の入力、クライアントならホストへ送られる
}

// ─── 参加する(Lobby のボタンの処理)───
Network::SetGameId("Demo");
Network::StartHost("Player1");                      // ホスト側のボタン
Network::Join("127.0.0.1", Network::defaultPort, "Player2");   // 同じPCの2つ目 / 別のPCは表示されたIPアドレス
Network::StartSearch();                             // LANのホストを探す。見つかったら GetFoundHosts() に並ぶ

// ─── 出来事(協力パズル。Part 4-4)───
// スイッチ(クライアントでも押せる): ホストに知らせる
Network::SendEvent("SwitchPressed", NetworkTarget::Host);
// ホスト側の扉の管理(ホストだけ判断して、結果を共有の値と出来事で全員へ)
Network::Subscribe("SwitchPressed", [this](NetworkReader&, int sender) {
    Network::SetSharedValue("Door1", 1);             // 途中参加の人にも「開いている」が伝わる
    Network::SendEvent("DoorOpened", NetworkTarget::All);   // 音やパーティクルの演出用
});
// 全員の扉(見た目): EventBus でも受けられる(中身が無い出来事は EventBus にも届く)
EventBus::Get().Subscribe("DoorOpened", [this]() { PlayOpenAnimation(); });
// Scene::Uninit で: EventBus::Get().Clear(); Network::ClearHandlers();

// ─── スポーン(Part 4-5)───
namespace Prefab { static constexpr int bullet = 1; }
Network::RegisterPrefab(Prefab::bullet, [](Scene& scene, const NetworkSpawnInfo& info) {
    return CreateNetworkBullet(scene, info);       // info.hasAuthority を見て Dynamic / Kinematic を選ぶ
});
GameObject* bullet = Network::Spawn(*this, Prefab::bullet, ownerPlayer, firePosition);   // ホストだけ
Network::Despawn(bullet);                          // ホストだけ(全員の画面から消える)
```
- 対象(Demo): 新しい `Demo/Game/Scenes/Network/` `Objects/Network/` `Components/Network/` のファイルと、`DemoRegistry.h/.cpp`(`DemoCategory::Network` と一覧の行)、`DemoGuideController` の `ResetSettings`。
- ファイル名・クラス名は、CLAUDE.md の規則に従う(工場関数 `CreateNetworkBox` ↔ `Objects/Network/NetworkBox.h/.cpp`、動き `NetworkBoxController` ↔ `Components/Network/NetworkBoxController.h/.cpp`。Componentと同じ名前のファイルは作らない)。Demo内でファイル名を重複させない(`.obj` の衝突。`DemoPlan.md` 2章)。

#### 依存する機能
`Transform`・`RigidbodyComponent`(BodyType)・`EventBus`・`ObjectPool`・`SceneManager`・`Time`(`GetUnscaledDeltaTime`)・`Input`・`Math::Lerp`、Demoプロジェクト(`DemoRegistry`、`DemoGuide`)

#### 未確定の点(ユーザーに聞きたいこと)
- (a) ホスト式 + 状態同期 + ENet でよいか(1章の「ユーザーと合意した方針」の再確認)
- (b) `Network` を「アプリ全体で1つのサービス」にし、`CLAUDE.md` の一覧に足してよいか(`CLAUDE.md` の変更はユーザー本人の了承が必要)
- (c) 新フォルダ `Engine/Network/` と `ThirdParty/enet/` を作ってよいか(README の「11フォルダ」が変わる)
- (d) ENetの入手(配布元・バージョン・ライセンスの確認、ダウンロードの許可)を、いつ誰が行うか。ソースを `Engine.vcxproj` に直接登録する形でよいか
- (e) 既定のポート番号(案: ゲーム 47200 / 探索 47201)でよいか
- (f) Part 4-6(クライアント予測 / Owner権限)を作るか。LANで遊んでから決めてよいか
- (g) 同期するのは「親を持たないオブジェクト」だけ(子オブジェクトの同期は作らない)でよいか
- (h) 向きは度数のXYZ(Transformと同じ)で送り、各軸を近い向きに補間する方式でよいか(クォータニオンは作らない)
- (i) 位置は最初は float のまま送り、足りなくなってから量子化するのでよいか
- (j) 中身のある出来事は `Network::Subscribe` で受け、`EventBus` は変えない(中身の無い出来事だけ `EventBus` にも届く)でよいか
- (k) プレイヤーが抜けたら、そのプレイヤーのオブジェクトは自動で `Despawn` してよいか
- (l) ホストが落ちたら全員終了(ホスト移譲しない)でよいか
- (m) シーン切り替えの同期は、ゲーム側が `RegisterScene` / `NotifySceneReady` を呼ぶ方式でよいか(`SceneManager` を変えない)

---

## 9. 全体の未確定事項(ユーザーに決めてほしいこと)

1. **この設計書の了承の扱い**: 「この設計書を読んで了承してもらうこと」を、各Part(4-1〜4-7)の着手前の確認として扱ってよいか(`EngineRoadmap3.md` の8章 1番と同じ質問)。決まるまでは、Partごとに着手前の確認をする。
2. **新フォルダ**: `Engine/Network/`(12個目のフォルダ)と `ThirdParty/enet/` を作ってよいか。README の「11フォルダ」の説明を12に変えてよいか。それとも `Engine/Core/` などに入れるか。
3. **`Network` をサービスにする**: `namespace Network`(アプリ全体で1つ。`Get()` なし)にしてよいか。`CLAUDE.md` の「シングルトンは EventBus/SceneManager/Input/Audio だけ」の一覧に `Network` を足す文言の変更を了承してよいか(`CLAUDE.md` は**ユーザー本人**の了承が無いと変えられない)。
4. **ENetの入手**: 配布元・バージョン・ライセンス(MIT)の確認をしてから、ダウンロードしてよいか(**ダウンロードはユーザーの許可が必要**。ファイル名・入手元・大きさを示してから聞く)。ソースを `ThirdParty/enet/` に置いて `Engine.vcxproj` に直接登録する形(おすすめ)か、`.lib` を作る形か。
5. **ポート番号**: 案(ゲーム 47200 / 探索 47201)でよいか。ほかのソフトと重なる場合の変え方(画面で入力できるようにするか)。
6. **Part 4-6 を作るか**: クライアント予測 / Owner権限は、LANで遊んでから決めてよいか。
7. **同期の範囲**: 親を持たないオブジェクトだけ(子の同期は作らない)でよいか。大きさ(`scale`)の同期は「変わった時だけ」でよいか。
8. **向きの補間**: 度数のXYZを近い向きに補間する方式(Transformと同じ)でよいか。それともクォータニオンにするか。
9. **位置の精度**: 最初は float のまま送り、帯域が足りなくなってから量子化するのでよいか。
10. **EventBus との連携**: `EventBus` に引数(ペイロード)を足さず、中身の無い出来事だけ `EventBus` にも届ける方式でよいか(EventBus を変えると、全サンプルに影響するEngine変更になる)。
11. **途中参加を許可するか**: ゲームが始まった後に入ってくる人を、許可する(おすすめ。Part 4-5 で初期状態を送る)か、ロビーにいる間だけにするか。
12. **抜けた人のオブジェクト**: 自動で消す(おすすめ)か、残すか。
13. **ホストが落ちた時**: 全員セッション終了(おすすめ。ホスト移譲は作らない)でよいか。
14. **Sample への導入**: Sample2D / Sample3D はネットワーク対応にしない(確認はDemoだけ)でよいか。それとも、どちらかのSampleに「協力プレイモード」のようなものを足すか(足す場合は、別途設計する)。
15. **Demo の `Network` カテゴリ**: カテゴリ名 `Network` でよいか。デモの内容(`NetworkBasic` `NetworkTransform` `NetworkInput` `NetworkEvent` `NetworkSpawn`)と、それぞれ2D版・3D版を作ってよいか。
16. **名前の入力**: プレイヤー名は自動(`Player1` `Player2`…)でよいか。それとも文字入力のUIを作るか(今のEngineに文字入力のUIは見当たらない。実装時に確認)。
17. **探索が使えないネットワーク**: 学校のネットワークでブロードキャストが通らない場合に備えて、IPアドレスの手入力(画面にIPを表示 + 入力欄またはボタンでの選択)を必ず付ける案でよいか。
18. **Windowsのファイアウォール**: 初回の許可ダイアログで「プライベートネットワークを許可」する手順を、READMEに書いてよいか。
19. **ロードマップの扱い**: 完成したら、README と各ロードマップの状態表を更新する(これまでと同じ)。この文書の状態の欄は、実装が終わったPartから「実装済み」に書き換える。

---

## 10. Engine変更ファイルの一覧(ユーザーの了承が必要になる所)

### 8.1 LAN通信
| ファイル | Part | 内容 |
|---|---|---|
| `Engine/Network/NetworkConfig.h`(新規) | 4-1 | 定数(`maxPlayers` など) |
| `Engine/Network/NetworkTransport.h/.cpp`(新規) | 4-1 | ENetを包むクラス |
| `Engine/Network/NetworkMessage.h/.cpp`(新規) | 4-1 | `PacketType`、`NetworkWriter` / `NetworkReader` |
| `Engine/Network/Network.h/.cpp`(新規) | 4-1〜4-5 | 窓口サービス(Partごとに関数が増える) |
| `Engine/Network/NetworkSession.h/.cpp`(新規) | 4-2 | 参加者名簿・プレイヤー番号 |
| `Engine/Network/LanDiscovery.h/.cpp`(新規) | 4-2 | LANでホストを探す |
| `Engine/Network/NetworkIdentityComponent.h/.cpp`(新規) | 4-3 | 同期する物の印 |
| `Engine/Network/NetworkTransformComponent.h/.cpp`(新規) | 4-3 | 位置・向きの同期と補間 |
| `Engine/Network/NetworkObjects.h/.cpp`(新規) | 4-3 / 4-5 | 登録簿(4-3)、プレハブ・スポーン(4-5) |
| `Engine/Network/NetworkInput.h`(新規) | 4-4 | 入力の構造体 |
| `Engine/Core/GameEngine.cpp` | 4-1 | `Network::Update()` / `EndFrame()` / `Shutdown()` の3行と `#include` |
| `Engine.vcxproj` / `Engine.vcxproj.filters` | 4-1〜 | 新ファイルとENetのCソースの登録、インクルードパス |
| `ThirdParty/enet/`(新規。Engineの外) | 4-1 | ENetのソース・`LICENSE`・`README.txt` |

`Shader*.hlsl` / `PhysX.props` は、どのPartでも変えません。Sample2D / Sample3D / Demo の `.vcxproj` も変えません。

---

## 11. 大きなリスク

1. **Windowsのファイアウォール**: 初回に「通信を許可しますか」のダイアログが出る。**許可しないとUDPが届かず、参加できない**(「つながらない」の一番多い原因)。学生には「プライベートネットワークを許可」を案内する(READMEに書く)。会社・学校のPCは、管理者の設定で許可できないことがある。
2. **学校・公共のネットワークの制限**: 端末どうしの通信を禁止している(AP isolation)・ブロードキャストが通らない・UDPが制限されている、ことがある。**ホスト探索が動かなくても、IPの手入力でつながる**ようにしておく。それでも通らなければ、スマートフォンのテザリングなど別のネットワークで試す。
3. **ポート番号**: 既定のポートが他のソフトと重なると `StartHost` が失敗する(ポート使用中)。失敗を画面に出し、別のポートを選べるようにする。**1台で2つ起動した時、2つのプロセスが同じポートを待ち受けられない**ので、2つ目は必ずクライアントにする(探索用ポートも同じ)。
4. **1台で2つ起動した時の入力とウィンドウ**: 入力はウィンドウが選ばれている時だけ有効のはず(`Input.cpp:52-54`)。2つのウィンドウが重なって、片方が隠れる。ウィンドウを並べて、操作は選んだ側だけに入る。**操作の確認は、ユーザーに手でお願いする**(合成したキー入力は送らない)。Debugで2つ目を起動すると、同じ出力ファイル(exe・pdb)を使うので、**2つ目はエクスプローラーから起動する**(Visual Studioで2つ目のデバッグ起動をすると、ビルドの書き込みが失敗する場合がある)。
5. **PhysXの非決定性**: 別のPCで同じ計算が同じ結果になる保証が無い。→ 状態同期にしたので、**問題にならない**(位置はホストだけが決める)。クライアントのPhysXは、Kinematicのコピーにだけ使う(3章の7・8)。
6. **クライアント側のDynamic**: クライアントのコピーを誤ってDynamicにすると、勝手に落ちる・転がる・Transformを書き換える(CLAUDE.md違反)。→ 工場関数で `hasAuthority` を見て `BodyType` を選ぶ約束と、reviewerのチェック項目にする。
7. **Debugビルドとランタイムの整合**: ENetのソースを `Engine.vcxproj` で直接ビルドすれば、`PhysX.props` の `RuntimeLibrary`(Debug=`MultiThreadedDebug`、Release=`MultiThreaded`)が自動で効き、不一致は起きない(3章の18)。`.lib` を別に用意する場合は、x64のDebug/Releaseそれぞれと `/MTd` `/MT` を合わせる必要がある(おすすめしない理由)。
8. **リンク**: `ws2_32.lib` と `winmm.lib` が必要。`#pragma comment(lib, ...)` で書き、`Engine.lib` に記録されることを実装時に確認する。リンクエラー(LNK2019 など)が出たら、build スキルの表に従って調べる。
9. **ヘッダーの衝突**: `enet/enet.h` は `winsock2.h` を読み込む。`windows.h` を先に読んだファイルでは `winsock2.h` との重複定義エラーが出る。→ ENetを読む `.cpp` は、`#include "enet/enet.h"` を**最初の行**に書き、`Network.h` などの公開ヘッダーには出さない。
10. **ウィンドウのドラッグ・デバッガの停止で切断される**: 一時的にゲームが止まると、相手から応答なしと見られる。タイムアウトを長め(10秒程度)にする。それでも切れた時の表示を用意する。
11. **ラグ(遅れ)**: 補間の遅れ(約0.1秒)+ 往復の時間 + 入力の送信の遅れで、クライアントの操作は **0.15〜0.3秒ほど重く**感じる可能性がある(Wi-Fiはさらに揺れる)。協力パズルなら問題にならないが、**スマブラ風の対戦では気になる**。→ Part 4-6(任意)の予測 / Owner権限で対策できる。最初は入れない。
12. **パケットが大きい時の分割**: 物が多いと1つの `Snapshot` が大きくなり、ENetが分割して送る。分割のどれか1つが落ちると全部が捨てられる。→ 約1.2KBごとに自分で複数のパケットに分ける(8.1 の設計のポイント(e))。
13. **シーン切り替え中のパケット**: 切り替え中に古い/新しいシーンの `NetworkId` が混ざる。→ シーンの世代番号と `SceneReady` の合図で防ぐ(8.1 の設計のポイント(l))。実装が難しい所で、Part 4-5 が一番バグが出やすい。
14. **`SceneManager` が切り替え中の `ChangeScene` を無視する**(`SceneManager.cpp:10-13`): ホストの `SceneChange` が届いた時に、クライアントが別の切り替え中だと失敗する。→ Network が待って、切り替えが終わってから呼ぶ。
15. **セキュリティ**: 同じLANの信頼できる相手が前提で、認証・暗号化・ずる対策は作らない。壊れたパケットでクラッシュしないよう、範囲チェック(サイズ・種類・プレイヤー番号・オブジェクトの数・文字列の長さ・`isfinite`)だけは入れる。公共のWi-Fiで他人がいる所では使わない(README に書く)。
16. **ルーター越し(インターネット)は対象外**: ポート開放・NAT越えを作らない。LANだけ。
17. **ENetのバージョン・ライセンス**: ダウンロード前に確認が必要(ユーザーの許可)。MITライセンスだが、配布時に `LICENSE` を同梱する。ENetのソースに、警告やCの古い書き方がある(`ufbx` と同じように警告を切る)。
18. **ビルド時間・ファイル数**: Cのソースが十数個と `Engine/Network/` の20ファイルほどが増える。ENetは小さいので影響は小さい。
19. **ゲームの設計への影響**: 「ルールはホストだけ」「見た目はクライアントでも」の区別(`HasAuthority()`)を、ゲームを書く人が覚える必要がある。→ Demoの例と README で説明し、reviewerのチェック項目に入れる。

---

## 12. EngineGuide・README への反映方針

実装が終わった(または Part ごとに区切りがついた)時に、次を行います。

- **README**: 
  - 「Engineのフォルダ構成」(§0)を12フォルダにし、`Network` の説明を足す。
  - **新しい節「ネットワーク(Network)」**を、§38(デモ)の後に足す(番号は既存の節を変えない)。書く内容: 使い方(ホスト / 参加 / 探索)、`NetworkTransformComponent` の付け方、入力と出来事、スポーン、ホストと `HasAuthority` の約束、**1台でのテスト方法・LANのテスト方法**、ファイアウォール、よくあるつまずき。
  - §38「デモ」に `Network` カテゴリのデモ一覧を足す。EventBus(§7)・ObjectPool(§8)の節に「ネットワーク越しにも使える」を一言足す。
- **EngineGuide(`docs/EngineGuide/`)**: `tools/src/Network.txt` を新しく作り(書式は `tools/README.txt` のとおり。`=page Network` `=file` `=table` `=code` `=demo` `=readme`)、`tools/build.ps1` でページを作り、`tools/check.ps1 -Fix` で検査する。`index.html`・`architecture.html`(フレームの図に `Network::Update` / `EndFrame` を足す)・`glossary.html`(ホスト・クライアント・Authority・スナップショット・補間などの用語)・ファイル索引(`files.html`)も更新する。`tools/make-diagrams.ps1` で、通信の流れの図を足す案もある。
- **CLAUDE.md**: `Network` をサービスに加える文言(「シングルトンは EventBus/SceneManager/Input/Audio だけ」の一覧)と、Engineのフォルダ数が変わる記述は、**ユーザー本人の了承が出てから**更新する(こちらでは変えない)。
- **この文書(`docs/EngineRoadmap4.md`)**: 実装が進んだら、4章の表の「状態」と、8.1 の「状態」、各Partの「実装したこと・決めたこと」を足して更新する(`EngineRoadmap3.md` と同じ)。
- **`docs/DemoPlan.md`**: Demo に `Network` カテゴリが増えたことを、デモの一覧表(6章)と状態の表に足す。

---

## 13. 実装の記録(Part 4-1〜4-5、4-7)

### ENet の取得
- 配布元: <https://github.com/lsalzman/enet> の最新の安定タグ **v1.3.18**(`archive/refs/tags/v1.3.18.zip`、約93KB)。取得日 2026-11-06。ライセンスは **MIT**(Copyright (c) 2002-2024 Lee Salzman)。
- `ThirdParty/enet/` に、`include/enet/*.h`、`callbacks.c compress.c host.c list.c packet.c peer.c protocol.c win32.c`(`unix.c` は Windows では使わない)、`LICENSE`、`README.txt`(入手元・バージョン・ライセンス・使い方)を置いた。展開物のスクリプトや実行ファイルは実行していない。
- `Engine.vcxproj` に C のソースとして直接登録(`CompileAsC`、警告オフ、SDLチェックオフ、`_WINSOCK_DEPRECATED_NO_WARNINGS` など)。ランタイムは `PhysX.props`(Debug=`/MTd`、Release=`/MT`)に自動で従うので、不一致は起きなかった。`ws2_32.lib` / `winmm.lib` / `iphlpapi.lib` は `#pragma comment(lib, ...)` で、Sample / Demo の `.vcxproj` は変えていない。

### 作ったEngineのファイル(設計書の10章どおり)
- 新規: `Engine/Network/` の `NetworkConfig.h`、`NetworkMessage.h/.cpp`、`NetworkTransport.h/.cpp`、`NetworkSession.h/.cpp`、`LanDiscovery.h/.cpp`、`NetworkInput.h`、`NetworkObjects.h/.cpp`、`NetworkIdentityComponent.h/.cpp`、`NetworkTransformComponent.h/.cpp`、`Network.h/.cpp`(18ファイル)、`ThirdParty/enet/`。
- 変更: `Core/GameEngine.cpp`(`#include` と `Network::Update()` / `EndFrame()` / `Shutdown()` の3行)、`Engine.vcxproj` / `Engine.vcxproj.filters`(登録とインクルードパス)。`Shader*.hlsl` / `PhysX.props` / `EventBus` / `SceneManager` / `Transform` / `Rigidbody*` / `ObjectPool` は**変えていない**。
- Demo(Engine ではない): `Network` カテゴリ、`Scenes/Network/` `Objects/Network/` `Components/Network/` の26組、`DemoRegistry`、`DemoGuideController::ResetSettings`(接続を閉じる)。

### 設計書から決めたこと・変えたこと
- `Network::NotifySceneReady(int sceneId)` は **`NotifySceneReady(Scene& scene, int sceneId)`** にした(クライアントがスポーンの工場関数に渡す `Scene` が必要で、`SceneManager` に現在のシーンを聞く関数が無いため。`SceneManager` は変えない約束)。`ClearHandlers()` はこの記憶も消す。
- `SceneManager` に「切り替え中か」を聞く関数が無いので、クライアントは切り替えを頼んで2秒たっても `NotifySceneReady` が来なければ、最大3回やり直す。
- スナップショットは、1件ごとに長さ(1バイト)を付けた(知らない NetworkId の件を安全に読み飛ばすため)。時刻は `double`。
- クライアントの「ホストの時刻の推定」は、毎フレーム進めながら、届く度に15%ずつ合わせる方式にした(届いた時だけ表示の時刻を動かすと、30Hzの段差が出るため)。
- 入力の「押した瞬間」は、ホストでは1フレームだけ立つ(`EndFrame` で消す)。クライアントは直近3回の送信に重ねて入れ、ホストは連番の差で取り込む。
- `NetworkIdentityComponent` は、スポーンした物が非アクティブになると NetworkId を手放す(`ObjectPool` で使い回した時に、古い番号で登録し直されて別の物とぶつかるのを防ぐ)。
- 接続したけれど 5 秒 `Join` が来ない相手は、ホストが切る。参加の申し込みは 8 秒で諦める。
- `Network::Disconnect()` はホスト探索も止める。
- Demo は「Host / Join を同じシーンで押す」ので、役割が変わった時に同期する物を作り直す(`NetworkWorldController`)。ロビーのシーンで先につなぐ作りなら不要。

### スクラッチテスト(ウィンドウ無し。Debug)
`NetTest.exe`(scratchpad。リポジトリには入れていない)を3つのモードで実行し、**全て合格**した。
- `unit`(1つのプロセスの中): 52件合格 / 0件失敗。書き込み/読み取り(切れたパケット・NaN・長すぎる文字は失敗を返し、落ちない)、補間の計算(中間値・360度またぎ・最新で止まる・ワープ・古い写真を捨てる・壊れた写真を弾く・範囲に収める)、ENetの接続と送受信(信頼100/100が順番どおり、信頼しないは100/100、1100バイトも欠けない、切断)、セッション(4人まで参加・プレイヤー番号1,2,3・5人目は満員で拒否・ゲームID違い拒否・プロトコル違い拒否・抜けると名簿更新・次は一番若い空き番号・ホストが閉じると全員Offline・同じポートはStartHost失敗・応答なしは2.0秒で切断)、参加前の相手からのでたらめなパケット600個以上、LAN探索(名前・ポート・人数2/4が見つかる・ゲームID違いは見えない・同じポートの2つ目は失敗・止めると消える)。
- `host` + `client`(2つのプロセスで、127.0.0.1): ホスト25件 / クライアント26件、全て合格。接続(番号1)・途中参加で共有の値が届く・ping(約18〜27ms)・出来事(Host/All/Others の届き方・送り主の番号・中身つき・中身なしがEventBusにも届く)・クライアントが共有の値を変えようとすると拒否・Transform同期(クライアントの遅れの平均0.085秒、後ろに戻らない、360度またぎで急な回転なし、ワープの途中の位置なし)・入力(軸が届く、ボタン5回が5回、1秒途絶えると約1.02秒でゼロに戻る)・スポーン3個→Despawn(ハンドラ1回)→プール(SetActive(false)の再利用)で新しいNetworkId・クライアントが抜けるとその人の物が自動で消える・途中参加で今の物(スポーン)と共有の値が届く・`ChangeSceneForAll` でクライアントが登録した関数が呼ばれ、新しいシーンにスポーンが再送される・ホストが閉じるとクライアントがOffline+"NetworkDisconnected"・ホストを立て直すとクライアントが再接続・**壊れたパケット(でたらめ364個)をホストへ、400個を偽のホストからクライアントへ送っても落ちない**。
- Release のビルドも成功した(`build.ps1`)。

### 先送り・未実装
- **Part 4-6(クライアント予測 / Owner権限)**: 作っていない。`NetworkAuthority::Owner` の土台も入れていない。LANで遊んでから決める。
- シーン切り替え(`RegisterScene` / `ChangeSceneForAll`)は、Engineに実装してスクラッチテストで確かめたが、**Demoでは使っていない**(切り替え用のデモは作っていない)。
- 量子化(float のまま送っている)、子オブジェクトの同期、`SceneManager` の変更は、設計どおり作っていない。
- `CLAUDE.md` の「シングルトン(`Get()`)は EventBus/SceneManager/Input/Audio だけ」に `Network` を足す文言と、Engineのフォルダ数の記述は、ユーザー本人の了承が必要なので**変えていない**(案は完了報告に書いた)。