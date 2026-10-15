# Engine 機能追加ロードマップ

## 1. はじめに

### この文書の目的
Engine(`3D_FreamWork/Engine/`)に今後追加したい16個の機能について、「どんな関数を作るか」「どの順番で作るか」「Sampleではどう使えるか(使用例)」をまとめた設計書です。チーム(C++初心者を含む)で分担して実装する時の資料として使います。

### 状態
**ここに書いてある内容は「設計案」です。実装済みのものは、4章の表の状態が「完了」になっています(今は計算系、時間とタイマー、リソースの使い回し、イージング、移動系、物理の問い合わせと力、カメラ機能)。**
関数名・引数は実装しながら変わってよいものです。実際に作る時は、この文書を出発点にして、下の「共通ルール」どおりに1機能ずつ、着手する時に説明してから進めてください(了承は待ちません)。実装したら、この文書の「作る順番」の表の状態を更新してください。

---

## 2. 共通ルール

### Engineの変更は1機能ずつ、説明してから進める(了承は待たない)
Engineを変えると Sample2D と Sample3D の**両方**に影響します(`CLAUDE.md` のルール)。ただし、**このロードマップの機能を作る間は例外として、ユーザーの了承を待たずに進めてよい**ことになりました(`CLAUDE.md` の「docs/EngineRoadmap.md の機能を作る間の例外」)。対象は、その機能を作るためのEngineの変更(新しいファイル、既存のEngineファイルの書き換え、`Shader*.hlsl`・`PhysX.props` を含む)です。

1機能ごとに、着手する時に次の3つを短く説明してから作り始めます(説明だけで、了承は待ちません)。

1. **何を**作るか(追加・変更するファイルと関数)
2. **どのEngineファイル**を変えるか
3. **Sample2D / Sample3D への影響**(どちらのどのコードが変わるか、見た目や動きが変わるか)

- 作り終わったら、変更したEngineファイルを報告します
- reviewer(レビュー役)のレビューは今まで通り必ず受けます
- **ロードマップに無いEngineの変更**(バグ修正・ついでの改良など)は、今まで通り①何を ②なぜ ③両サンプルへの影響 を説明して了承をもらいます
- ロードマップがすべて終わったら、この例外(`CLAUDE.md`・`.claude/` の設定)を元に戻します

### このロードマップではEngineの実装を優先する
今回はEngineを大きく書き換えるので、**このロードマップの機能はEngineに作ることを優先します**。
- `CLAUDE.md` の「まずSample側(`Game/` のComponent)だけで解決できないか検討する」という考え方は、**このロードマップの機能には当てはめません**(Sample側で済ませる検討は不要)
- ただし、上の「着手する時に説明してから進める」「レビューを受ける」ルールは守ります

### 名前の付け方
名前の付け方は `CLAUDE.md` の「命名規則」に従います(略語・命名の例外もそこに書いてあります)。この文書で特に関係するもの:

| 種類 | 付け方 | この文書での例 |
|---|---|---|
| どのオブジェクトにも付けられる汎用のComponent | `○○Component` | `TweenComponent`, `ChaseComponent`, `AudioSourceComponent` |
| 描画するComponent | `○○Renderer` | `GaugeRenderer`, `ParticleRenderer`, `ModelRenderer` |
| 関数だけのまとまり(namespace) | PascalCase(`Image` / `Audio` / `Physics` と同じ) | `Math`, `Time`, `Resources`, `Ease`, `SaveData` |
| `enum class` とその値 | PascalCase | `MovePlane::XZ`, `ForceMode::Impulse`, `Anchor::TopLeft` |
| ファイル名 | 中のクラス名と同じ | `ChaseComponent.h/.cpp` |

### 作ったら必ずSampleのコードを置き換える
1機能作ったら、**Sampleの該当するコードを必ず置き換えます**。各機能の節にある「置き換えるSampleのコード」がその対象です。
「いつか使うかもしれない関数」を作りっぱなしにしません。使われていない関数は、壊れていても誰も気付かないからです。

### ファイルを追加する時
- `Engine.vcxproj` と `Engine.vcxproj.filters` の**両方**に登録する
- 日本語を含む `.h`/`.cpp` は **UTF-8 BOM付き**で保存する
- Debug と Release の両方でビルドを確認する

---

## 3. 今のEngineで分かっている問題

設計に関わる「今のコードの事実」です。各機能はこれを直すために作ります。

1. **`Image::LoadTexture` は呼ばれる度に画像を読み直す**(→ 5.3 で解決済み)
   同じ画像を共有する仕組みが無く、`SpriteRenderer` を作る度にファイルを読み込みます。敵を100体出すと `player.png` を100回読むことになります(`MeshRenderer::SetTexture` も同じ)。
2. **コマ番号と色が「画像1枚ごとのデータ」に入っている**(→ 5.3 で解決済み)
   `SetSpriteIndex` のコマ番号と `SetColor` の色は、`Image.cpp` の `TextureData`(画像1枚ぶんのデータ)に入っています。今は「1体ごとに画像を読み直している」ので困っていませんが、1の問題を直して画像を共有すると、**同じ画像を使う全員のコマと色が連動してしまいます**(1体を赤くすると全員赤くなる)。そのため、コマ番号と色は `SpriteRenderer` 側へ移す必要があります。
3. **同じ効果音を重ねて鳴らせない**
   `Audio` は1つの音に再生口(`IXAudio2SourceVoice`)を1つしか持たず、`Audio::Play` の度に前の音を `Stop` + `FlushSourceBuffers` で止めてから鳴らし直します。連射の音が途切れてしまいます。
4. **各Componentがタイマーを手書きしている**(→ 5.2 で解決済み)
   `spawnTimer`(EnemyManager)、`invincibleTimer`(PlayerController)、`animationTimer`(Sample3D EnemyController)など、「時間を引いて0以下になったら…」という同じ処理があちこちにあります。
5. **HUDの座標が直書き**
   `HUDController` の `Text::Draw(buf, -560.f, 320.f, 28.f)` のように、画面の左上を数字で決め打ちしています。画面サイズが変わるとずれます。
6. **方向・距離・角度の計算が散らばっている**(→ 5.1 で解決済み)
   `atan2f`(Sample2D PlayerController)、`sqrtf`(両サンプルの EnemyController)、`sinf/cosf`(両サンプルの BulletManager / EnemyManager、Sample3D PlayerController)が各Componentにそのまま書かれています。

### コードを読んで追加で分かったこと
- **UIモードの `SpriteRenderer` も2Dカメラと一緒にスクロールする**。`Sprite.cpp` はUIモードでも `Image::GetCamera()` の位置を引いて描画しています(Sample2Dの敵がスクロールするのはこのため)。画面に固定したいHPバーなどには、カメラの影響を受けない描き方(→ 5.10 の `Image::DrawRect`)が要ります。
- **画面の座標のY方向が関数によって逆**。`Text::Draw` は「画面中央が(0,0)、Y+=上」、`Input::GetMousePosition` は「画面中央が(0,0)、Y+=下」です。この文書の新しい関数は「画面中央が(0,0)、Y+=上」(`Text::Draw` と同じ)にそろえます。
- **`Text::Draw` は指定した座標に文字の「中心」を合わせる**。左寄せにはできません(→ 5.11)。
- **文字はいつもゲーム画面の一番上に出る**。`GameEngine::Run` は `Text::BeginDraw` 〜 `Text::EndDraw` の間でシーンを描画していて、Direct2Dの文字は `EndDraw` の時にまとめて描かれます。画面エフェクト(5.8)で暗転しても、今のままでは文字だけ見えたままになります。
- **`Text::Draw` は呼ばれる度にフォントの設定(`IDWriteTextFormat`)を作り直している**(重い)。なお、`IDWriteTextLayout` を `Release` していなかったメモリリークは修正済み。
- **`ButtonController` は Sample2D と Sample3D に、まったく同じ内容でコピーされている**。
- **`Physics::Update` は `dt <= 0` の時は何もしない**。時間の倍率を0(ポーズ)にしてもPhysXは安全に止まります(→ 5.2)。

---

## 4. 作る順番

| 順 | 機能 | 理由・依存 | 担当者 | 状態 |
|---|---|---|---|---|
| 1 | [計算系](#51-計算系) | すべての土台。ほかの機能のほとんどが使う | | 完了 |
| 2 | [時間とタイマー](#52-時間とタイマー) | イージング・画面エフェクト・アニメーションが使う | | 完了 |
| 3 | [リソースの使い回し](#53-リソースの使い回し) | 画像を共有する前に「データ」と「1体ごとの状態」を分けておく(アニメ・音・モデルの前提) | | 完了 |
| 4 | [イージング](#54-イージング) | カメラ・UI・画面エフェクトが使う | | 完了 |
| 5 | [移動系](#55-移動系) + [物理の問い合わせと力](#56-物理の問い合わせと力) | 加速度で動く移動は `AddForce` を使うので、同じ段の中では物理(5.6)を先に作る | | 完了 |
| 6 | [カメラ機能](#57-カメラ機能) | イージング・計算系を使う | | 完了 |
| 7 | [画面エフェクト](#58-画面エフェクト) | 時間(Unscaled)・イージングを使う | | 未着手 |
| 8 | [2Dアニメーション](#59-2dアニメーション) | 3(コマ番号をSpriteRendererへ移す)が前提 | | 未着手 |
| 9 | [UI](#510-ui) ・ [テキスト](#511-テキスト) + [セーブ/ロード](#512-セーブロード) | スライダーの値(音量など)を保存するため、セーブ/ロードも一緒に作る | | 未着手 |
| 10 | [音(立体音響・グループ音量)](#513-音の発生源と聞く位置立体音響) | 3(`Resources` の仕組み。音のデータ共有 `LoadSound` はここで足す)・9(音量スライダー・保存)を使う | | 未着手 |
| 11 | [CSV](#514-csv) | ほかに依存しないので、いつでも並行して作れる | | 未着手 |
| 12 | [パーティクル](#515-パーティクル) | 時間・計算系・リソースを使う | | 未着手 |
| 13 | [FBX読み込み → 3Dアニメーション](#516-fbx読み込みと3dアニメーション) | 一番大きい。外部ライブラリの追加とシェーダーの追加がある | | 未着手 |

**CSV(11)とセーブ/ロード(9)は、ほかの機能にほぼ依存しないので、学生の分担・練習に向いています。** 順番を待たずに先に始めて構いません。

---

## 5. 機能ごとの設計

各節は「目的 / ファイル案 / 関数 / 設計のポイント / 置き換えるSampleのコード(使用例) / 依存する機能」の順に書いています。「対象」に書いたSampleのコードは、その機能を作った時に必ず置き換えます。

---

### 5.1 計算系

> **実装済み**: `Engine/MathUtil.h`(演算子と、乱数以外の関数をinlineで)と `Engine/MathUtil.cpp`(乱数)。下の「対象」のSampleのコードも置き換え済み。使い方は `README.md` の「9. 計算系(Math)」を参照。
> 設計から変えた所:
> - `Clamp` / `RandomRange` の引数名を `min` / `max` から `minValue` / `maxValue` にした(`windows.h` の `min` / `max` マクロとぶつかる心配をなくすため)
> - `Normalize` は長さが「ほぼ0」(1e-6未満)の時も0ベクトルを返す(とても短いベクトルで割って値が大きくなりすぎないように)
> - `RandomRange` は min > max で渡されても入れ替えて動く
> - `Forward3D` は水平(`rotate.y`)だけを使う。右方向の関数(`Right3D`)は作らず、`Math::Cross({0,1,0}, 正面)` で求める
> - Sample2D `EnemyController` は、設計の使用例(XMFLOAT3の `DirectionTo`)ではなく `ToXY` してからXMFLOAT2版を使った(今の動きと同じく、Zの差を無視するため)

**目的**: 方向・距離・角度の計算を1か所にまとめて、各Componentから `atan2f` / `sqrtf` / `sinf` / `cosf` を消す。0で割ってしまう、角度の向きを間違える、といったミスを無くす。

**ファイル案**: `Engine/MathUtil.h`(中身は `namespace Math`)。短い関数ばかりなので、`.cpp` は乱数の部分だけ(`Engine/MathUtil.cpp`)。

**関数**:
```cpp
// ── 演算子(namespace DirectX の中で定義する) ──
XMFLOAT2 operator+(XMFLOAT2 a, XMFLOAT2 b);      // 足し算
XMFLOAT2 operator-(XMFLOAT2 a, XMFLOAT2 b);      // 引き算
XMFLOAT2 operator*(XMFLOAT2 v, float s);         // 何倍かする
XMFLOAT3 operator+(XMFLOAT3 a, XMFLOAT3 b);      // 足し算
XMFLOAT3 operator-(XMFLOAT3 a, XMFLOAT3 b);      // 引き算
XMFLOAT3 operator*(XMFLOAT3 v, float s);         // 何倍かする

namespace Math {
    // ── ベクトル ──
    float    Length(XMFLOAT3 v);                       // 長さ
    float    Distance(XMFLOAT3 a, XMFLOAT3 b);         // 2点の距離
    float    DistanceSq(XMFLOAT3 a, XMFLOAT3 b);       // 距離の2乗(大小を比べるだけならsqrt不要で速い)
    XMFLOAT3 Normalize(XMFLOAT3 v);                    // 長さ1にする(長さ0なら(0,0,0)を返す。0で割らない)
    XMFLOAT3 DirectionTo(XMFLOAT3 from, XMFLOAT3 to);  // fromからtoへの向き(長さ1)
    float    Dot(XMFLOAT3 a, XMFLOAT3 b);              // 内積(同じ向きなら+、逆なら-)
    XMFLOAT3 Cross(XMFLOAT3 a, XMFLOAT3 b);            // 外積(2つに垂直な向き)
    XMFLOAT2 ToXY(XMFLOAT3 v);                         // XMFLOAT3のx,yだけ取り出す(2D用)
    // ※ Length/Distance/DistanceSq/Normalize/DirectionTo は XMFLOAT2 版も用意する

    // ── 補間・制限 ──
    float    Lerp(float a, float b, float t);                         // aとbの間(t=0でa、t=1でb)
    XMFLOAT3 Lerp(XMFLOAT3 a, XMFLOAT3 b, float t);                   // 位置の補間
    float    Clamp(float value, float min, float max);                // min〜maxに収める
    XMFLOAT3 MoveTowards(XMFLOAT3 current, XMFLOAT3 target, float maxDistance); // 最大maxDistanceだけ近づく(行き過ぎない)

    // ── 角度(すべて「度」) ──
    float    DeltaAngle(float current, float target);                 // 角度の差(-180〜180。350度と10度の差は20度)
    float    MoveTowardsAngle(float current, float target, float maxDelta); // 最大maxDeltaだけ回る(回転速度の上限)
    float    AngleToTarget2D(XMFLOAT2 from, XMFLOAT2 to);             // toの方を向くrotate.zの値(2D)
    float    AngleToTargetY(XMFLOAT3 from, XMFLOAT3 to);              // toの方を向くrotate.yの値(3Dの水平の向き)
    XMFLOAT2 Forward2D(const Transform& t);                           // 2Dの正面(rotate.z=0で(0,1))
    XMFLOAT3 Forward3D(const Transform& t);                           // 3Dの正面(rotate.y=0で(0,0,1))

    // ── 乱数 ──
    float    RandomRange(float min, float max);   // min〜maxのランダムな小数
    int      RandomRange(int min, int max);       // min〜maxのランダムな整数(maxも含む)
    XMFLOAT2 RandomOnCircle(float radius);        // 半径radiusの円周上のランダムな点
}
```

**設計のポイント**:
- 演算子は `namespace DirectX` の中で定義します。`XMFLOAT3` は `DirectX` の型なので、C++の「引数の型のnamespaceから関数を探す」仕組み(ADL)で、どこから使っても確実に見つかります。DirectXMathにある `XMVECTOR` 用の演算子とは別物なので、ぶつかりません。
- 角度の向きは今のコードに合わせます。
  - `AngleToTarget2D` = `XMConvertToDegrees(atan2f(-dx, dy))`(Sample2D PlayerController と同じ式)
  - `Forward2D` = `rotate.z` を `-rotate.z` ラジアンにして `(sin, cos)`(Sample2D BulletManager と同じ式)
  - `AngleToTargetY` / `Forward3D` = `rotate.y` の正面が `(sin, 0, cos)`(Sample3D の `CameraController::GetYaw()` と同じ向き)
- `Forward3D` が `rotate.x`(上下の向き)も使うかは未決定。まずは水平(`rotate.y`)だけにする。
- 乱数は `rand()` ではなく `std::mt19937` を使う(`rand()` は偏りがあり、`RAND_MAX` が小さい)。
- `Lerp` の `t` は0〜1に制限しない(行き過ぎた値も計算できる)。制限したい時は `Clamp` と組み合わせる。

**置き換えるSampleのコード(使用例)**:
```cpp
// Sample2D PlayerController: マウスの方を向く(atan2fを置き換え)
XMFLOAT2 mouseWorldPos = Input::GetMouseWorldPosition();
if (Math::DistanceSq(Math::ToXY(t.position), mouseWorldPos) > 1.0f) {
    t.rotate.z = Math::AngleToTarget2D(Math::ToXY(t.position), mouseWorldPos);
}

// Sample2D EnemyController: 追いかける(sqrtfと割り算を置き換え)
XMFLOAT3 dir = Math::DirectionTo(t.position, target->transform.position);
rigidbody->SetVelocity(dir * speed);

// Sample2D BulletManager: 撃った本人の向きへ(sinf/cosfを置き換え)
Fire(shooter->transform.position, Math::Forward2D(shooter->transform));

// Sample2D EnemyManager: Playerの周りのランダムな位置に出す
XMFLOAT2 offset = Math::RandomOnCircle(spawnDistance);
```
対象: Sample2D の `PlayerController`(atan2)、`EnemyController`(距離計算)、`BulletManager`(sin/cos)、`EnemyManager`(出現位置)。Sample3D の同じ名前のComponentも同様。

**依存する機能**: なし(すべての土台)

---

### 5.2 時間とタイマー

> **実装済み**: `Engine/Time.h/.cpp`(`namespace Time`)と `Engine/Timer.h`(`struct Timer`、ヘッダーだけ)。`GameEngine` の `CalcDeltaTime()` は `Time::Update()` に移した。下の「対象」のSampleのコードも置き換え済み。使い方は `README.md` の「10. 時間とタイマー(Time / Timer)」を参照。
> 決まったこと:
> - **A. ポーズ中の入力**: Engineはポーズ中も各Componentの `Update` を止めない(`dt=0` で呼ばれ続ける)。`Time::IsPaused()`(倍率が0ならtrue)を用意し、入力を止めたいControllerが `Update` の先頭で `if (Time::IsPaused()) return;` と書く。「ポーズ中も動くGameObject」の印は作らない
> - **B. dtの上限**: 1フレームの時間に上限0.1秒を付けた(ウィンドウのドラッグ中などに、すり抜け・ワープしないように)。上限は倍率をかける前の本当の時間にかけるので、`GetUnscaledDeltaTime()` も上限付き
> - **`Timer::Complete()` を追加**: すぐに「終わった状態」にする(`elapsed = duration`)。無敵時間のように「最初は終わっていて、きっかけがあったら `Reset()` で数え始める」使い方のため
>
> 設計から変えた・足した所:
> - `Time::Init()` / `Time::Update()`(Engine内部用。`GameEngine` だけが呼ぶ)を足した
> - `SetTimeScale` に負の値を渡すと0(ポーズ)になる
> - `Timer` にコンストラクタ `Timer(float duration = 1.0f, bool loop = false)` を付けた。`loop` で1フレームに何周分も進んだ時は、trueは1回だけ返し、`elapsed` は余り(duration未満)に戻す。`duration <= 0` でも0で割らない
> - `Timer` に `finished`(終わったか)を足した。0秒のタイマー(loopなし)も、作った直後・`Reset()` 直後は終わっておらず、次の `Update` で1回だけtrueを返すようにするため。loopで0秒以下の時は `Update` の度に毎回true
> - Sample3D `EnemyController::Spawn()` で歩きアニメーションを最初のコマに戻す(プールで再利用された敵が前回の続きから歩き始めないように)
> - 置き換え対象に、両サンプルの `BulletController`(寿命)と Sample3D `EnemyController`(歩きアニメーションのコマ送り)も加えた
> - `EnemyManager` の出現タイマーは、余った時間を次へ持ち越すようになった(前は捨てていたので、1回ごとに最大1フレーム分だけ間隔が長かった)。最初の1体が1.0秒で出るのは前と同じ(`elapsed` の初期値を `spawnInterval - firstSpawnDelay` にしている)

**目的**: 「1フレームの時間」をどこからでも取れるようにして、スロー・ポーズを作れるようにする。各Componentで手書きしているタイマーを1つの型にまとめる。

**ファイル案**: `Engine/Time.h/.cpp`(`namespace Time`)、`Engine/Timer.h`(`struct Timer`、短いのでヘッダーだけ)

**関数**:
```cpp
namespace Time {
    float GetDeltaTime();           // 1フレームの時間(秒)。スロー・ポーズの影響を受ける
    float GetUnscaledDeltaTime();   // 1フレームの本当の時間。ポーズ中も動かしたいUI・画面エフェクト用
    float GetTime();                // 起動してからの時間(秒。倍率の影響を受ける)
    void  SetTimeScale(float scale);// 時間の倍率(1=通常、0.3=スロー、0=ポーズ)。負の値は0にする
    float GetTimeScale();           // 今の倍率
    bool  IsPaused();               // ポーズ中か(倍率が0の時true)

    void  Init();                   // Engine内部用: 時間の計測を始める
    void  Update();                 // Engine内部用: 毎フレームのdtを計算する(上限0.1秒・倍率)
}

struct Timer {
    float duration = 1.0f;          // 何秒ごとか
    bool  loop = false;             // 繰り返すか
    float elapsed = 0.0f;           // 経過時間
    bool  finished = false;         // 終わったか(loopでない時だけ使う)

    Timer(float duration = 1.0f, bool loop = false);
    bool  Update(float dt);         // 時間が来たフレームだけtrue(loopなら余った時間を次へ持ち越す)
    void  Reset();                  // 最初からやり直す
    void  Complete();               // すぐに「終わった状態」にする
    float GetProgress() const;      // 進み具合(0〜1)
    bool  IsFinished() const;       // 終わったか(loopなら常にfalse)
};
```

**設計のポイント**:
- `GameEngine::Run` の `CalcDeltaTime()` の計算を `Time` に移します。`Physics::Update` と `SceneManager::Get().Update` には、倍率をかけたdt(`GetDeltaTime()`)を渡します。
- 倍率0(ポーズ)の時、`Physics::Update` は `dt <= 0` で何もしないので安全です。ただし `SceneManager` の `Update(0)` は呼ばれ続けるので、**ポーズ中にキー入力で弾が出てしまう**ことがあります。→ **決定(A)**: 入力を止めるのは各Controllerの仕事にする。止めたいControllerが `Update` の先頭で `if (Time::IsPaused()) return;` と書く。
- ウィンドウをドラッグしている間などにdtが極端に大きくなると、物がすり抜けたり飛んだりします。→ **決定(B)**: dtに上限0.1秒を付ける(倍率をかける前の本当の時間にかけるので、`GetUnscaledDeltaTime()` も上限付き)。
- `Timer::Complete()` で「終わった状態」から始められます(無敵時間: 最初は `Complete()`、ダメージを受けたら `Reset()`、無敵中かは `!IsFinished()`)。
- `Timer` は `loop` の時、余った時間を捨てずに次の周へ持ち越します(置き換え前の Sample3D EnemyController の `animationTimer -= loopTime` と同じ考え方)。
- **「○秒後に関数を呼ぶ」機能は今回は作りません。** ラムダが、もう消えたオブジェクトを触ってしまう危険があるからです(EventBusと同じ問題)。作るなら、呼び出し元のGameObjectに付くComponentとして作り、オブジェクトが消えたら自動でキャンセルされる形にします(5.4 の `TweenComponent` の `SetDelay` + `SetOnComplete` でも代用できます)。

**置き換えるSampleのコード(使用例)**:
```cpp
// EnemyManager.h
Timer spawnTimer{ 2.5f, true };   // 2.5秒ごとに出現

// EnemyManager::Update
if (spawnTimer.Update(dt)) Spawn(...);
```
対象: 両サンプルの `EnemyManager` の `spawnTimer`、`PlayerController` の `invincibleTimer`、`BulletController` の寿命(`lifeTimer`)。Sample3D `EnemyController` の歩きアニメーション(`frameTimer`)。(すべて置き換え済み)

**依存する機能**: なし

---

### 5.3 リソースの使い回し

> **実装済み(画像だけ)**: `Engine/Resources.h/.cpp`(`namespace Resources`)。`Image::TextureData` から `sheet`(分割数・コマ番号)と `color` を外し、`SpriteRenderer` のメンバへ移した。`MeshRenderer` も `Resources` を使う。下の「対象」のSampleのコード(`BackgroundController` / `GroundController`)も置き換え済み。使い方は `README.md` の「11. リソース(Resources、画像の使い回し)」を参照。
> 設計から変えた・決まった所:
> - **音(`LoadSound` / `ReleaseSound`)は今回は作らず、5.13 で作る。** 今の `Audio` は1つの音に再生口(`IXAudio2SourceVoice`)が1つしか無いので、音を共有すると、ある1体が鳴らした音を別の1体が止めてしまう(再生が干渉する)。5.13 で「音のデータ(共有)」と「再生口(鳴らす度に用意)」を分ける時に、一緒に `Resources` へ足す
> - **`LoadModel` も今回は作らず、5.16 で作る**(`Model` クラスがまだ無いため)
> - **参照カウントが0になっても、すぐには解放しない。** 0になった画像は `UnloadUnused()` でまとめて解放する。弾・敵のように出たり消えたりするものが、その度に読み直さないようにするため(上の関数案の「0になったら解放」から変えた)
> - `UnloadUnused()` は `SceneManager` が、シーン切り替えで**新しいシーンの `Init()` が終わった後**に呼ぶ(古いシーンの `Uninit()` の後。両方のシーンで使う画像を読み直さないように)
> - Engine内部用に `Resources::UnloadAll()` を足した。`GameEngine::Uninit` が `Image::Uninit` の前に呼び、参照カウントに関係なくすべて解放する(返し忘れがあれば出力ウィンドウに表示する)
> - パスは区切り文字(`\` と `/`)と大文字小文字の違いを同じものとして扱う(`"Assets/Player.png"` と `"assets\player.png"` は同じ画像)
> - 読み込みに失敗したパスは登録しない(次に呼ばれた時にもう一度読みにいく)。戻り値は `UINT_MAX`
> - `Image::LoadTexture` / `Image::ReleaseTexture` は「Engine内部用(Resourcesだけが使う)」としてコメントを付けて残した。`Image::LoadTexture` の `cols` / `rows` 引数は無くした
> - `Image::SetSpriteIndex` / `SetColor` / `GetColor` は無くした。`Image::Draw` は `Draw(transform, texID, color = 白, sheet = SpriteSheet(), worldSpace = false, lockX, lockY, lockZ)` の形にした(`Sprite::Draw` と同じ「色 → コマ」の順。コマは既存の `SpriteSheet` 構造体 = 横・縦の分割数とコマ番号)
> - `SpriteRenderer::SetTexture` を呼び直した時は、コマ番号は0に戻り、色はそのまま残る(前は画像を読み直していたので色も白に戻っていた)。分割数に0以下を渡すと1にする(0で割らないように)

**目的**: 同じ画像・音・モデルを何度も読み込まないようにする。あわせて「みんなで共有するデータ」と「1体ごとの状態」を分ける。

**ファイル案**: `Engine/Resources.h/.cpp`(`namespace Resources`)

**関数**:
```cpp
namespace Resources {
    unsigned int LoadTexture(const std::wstring& path);  // 画像を読む(同じパスは2回目以降、同じIDを返す)
    unsigned int LoadSound(const std::wstring& path);    // 音(.wav)を読む(同じパスは同じIDを返す)
    std::shared_ptr<Model> LoadModel(const std::wstring& path); // 3Dモデルを読む(同じパスは同じものを返す)

    void ReleaseTexture(unsigned int id);  // 使い終わった(参照カウントを1減らし、0になったら解放)
    void ReleaseSound(unsigned int id);    // 使い終わった(同上)
    void UnloadUnused();                   // 誰も使っていないものをまとめて解放(シーン切り替えの時)
}
```

**設計のポイント**:
- **一番大事なのは、「データ(画像・音・モデル)」と「使う人ごとの状態(コマ番号・色・再生位置)」を分けることです。**
  - 共有するデータ: 画像そのもの(SRV)と大きさ、音の波形データ、モデルの頂点
  - 1体ごとの状態: コマ番号、色、横・縦の分割数(`cols`/`rows`)、音の再生位置・音量
- そのため、今 `Image::TextureData` に入っている `sheet`(分割数とコマ番号)と `color` を、`SpriteRenderer` のメンバへ移します。`Image::Draw` は色とコマを引数で受け取る形に変えます。
- `SpriteRenderer` / `MeshRenderer` / `Audio` が中で `Resources` を使うようにするので、**ゲーム側のコード(`AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2)` など)はほぼ変わりません**。
- 今の `SpriteRenderer::Uninit` は `Image::ReleaseTexture` で画像を消しています。共有すると「1体が消えたら全員の画像が消える」ので、参照カウント(何人が使っているか)で管理します。
- 仕様の `Release` は、テクスチャと音のIDがどちらも `unsigned int` で区別できないため、`ReleaseTexture` / `ReleaseSound` に分けました。モデルは `MeshRenderer` と同じく `shared_ptr` で持つので、誰も持たなくなれば自動で解放されます。

**置き換えるSampleのコード(使用例)**:
ゲーム側の書き方は変わりません。Sample2D の `EnemyManager` で敵を100体出しても、`player.png` を読むのは1回だけになり、1体だけ色を変えても他の敵に影響しないことを確認します。
対象: `Image::LoadTexture` / `ReleaseTexture` を直接使っている Sample2D `BackgroundController`、Sample3D `GroundController` を `Resources` に置き換える。(置き換え済み)

**依存する機能**: なし(ただし 5.9 2Dアニメーション・5.13 音・5.16 FBX の前提になる)

---

### 5.4 イージング

> **実装済み**: `Engine/Ease.h`(`namespace Ease` と `EaseFunction`、ヘッダーだけ)と `Engine/TweenComponent.h/.cpp`。`MeshRenderer` に `GetColor()` を足した。下の「対象」のSampleのコード(両サンプルの `StartButton`)も置き換え済み。使い方は `README.md` の「12. イージング(Ease / TweenComponent)」を参照。
> 決まったこと:
> - **`SetUnscaled(bool)` を足した**(直前の動きに効く)。trueの動きはポーズ中も `Time::GetUnscaledDeltaTime()` で進む。既定はfalseで、`Update` の `dt`(= `Time::GetDeltaTime()`)で進む
> - **`ColorTo` は `SpriteRenderer` があればその色、無ければ `MeshRenderer` の色を変える。** どちらも無ければ何もしない(出力ウィンドウに表示する)。そのため `MeshRenderer` に `GetColor()` を足した
>
> 設計から変えた・決めた所:
> - `Ease` の関数は、t が0〜1の外で渡されたら0〜1に収めて(Clampして)から計算する。`OutBack` / `OutElastic` / `OutBounce` は、小数の誤差で両端が0・1ぴったりにならないので、t=0とt=1は決め打ちで0・1を返す
> - 好きな値を動かす関数は、仕様の `Value` から `TweenValue` に名前を変えた
> - 開始値は「動き出した時(`SetDelay` の待ち時間が終わった時)の今の値」。`TweenValue` だけは引数の `from` から始まる
> - 同じ種類(位置・大きさ・回転・色)の動きを始めると、前の動きは消える(前の動きのOnCompleteは呼ばない)。**`TweenValue` は上書きせず、いくつでも同時に持てる**(毎回別の値を動かすため)
> - `duration <= 0` の動きは、次の `Update` で(`SetDelay` があれば待ってから)すぐ1周終わった状態にする。普通の動きは目標値で完了扱い(OnCompleteも呼ぶ)、`SetLoop(false, true)` は開始値で完了扱い(OnCompleteも呼ぶ)、`SetLoop(true, ...)` は終わらない。呼んだ瞬間には値を変えない(後から `SetDelay` / `SetOnComplete` をつなげられるように)
> - `SetLoop(true, ...)` の動きは終わらないので、**OnCompleteは呼ばない**(毎周も呼ばない)。`pingPong` は「行って戻る」(durationの2倍)を1周とする。`SetLoop(false, true)` は1回だけ行って戻って終わる(OnCompleteも呼ぶ)
> - `Stop()` は今の値のまま止める(OnCompleteは呼ばない)
> - コールバック(`SetOnComplete` / `TweenValue` の関数)の中で動きを始めた時は、一旦別のリストに入れて、そのフレームの最後に加える(回している途中のリストを書き換えない)。新しい動きは次のフレームから動く。コールバックの中で `Stop()` しても、オブジェクトを `Destroy()` しても落ちない。`Destroy()` された後は、残りの動きを進めない
> - `RotateTo` は数値をそのまま補間する(近い方向へは回らない)
> - `Timer` は使わなかった(pingPongで「1周 = durationの2倍」を扱うため、経過時間を自分で持った)
> - Sampleの `ButtonController`(両サンプル)で、文字の大きさをボタンの高さに合わせるようにした(ボタンが大きくなる途中で、文字だけ最初から大きく出ないように)。作った時の高さを基準にするので、出終わった後の文字の大きさは今までと同じ

**目的**: 「最初は速く、最後はゆっくり止まる」のような気持ちいい動きを、1行で付けられるようにする。

**ファイル案**: `Engine/Ease.h`(`namespace Ease`)、`Engine/TweenComponent.h/.cpp`

**関数**:
```cpp
namespace Ease {
    // すべて 0〜1 を受け取って 0〜1 を返す(途中で範囲を少しはみ出すものもある)
    float Linear(float t);     // 一定の速さ
    float InQuad(float t);     // ゆっくり始まる
    float OutQuad(float t);    // ゆっくり止まる
    float InOutQuad(float t);  // ゆっくり始まって、ゆっくり止まる
    float OutCubic(float t);   // OutQuadより強くゆっくり止まる
    float OutBack(float t);    // 少し行き過ぎてから戻る
    float OutElastic(float t); // ばねのように揺れて止まる
    float OutBounce(float t);  // ボールのように跳ねて止まる
}
using EaseFunction = float(*)(float);  // イージング関数の型

class TweenComponent : public Component {
public:
    TweenComponent& MoveTo(XMFLOAT3 target, float duration, EaseFunction ease = Ease::OutQuad);   // 位置を動かす
    TweenComponent& ScaleTo(XMFLOAT3 target, float duration, EaseFunction ease = Ease::OutQuad);  // 大きさを変える
    TweenComponent& RotateTo(XMFLOAT3 target, float duration, EaseFunction ease = Ease::OutQuad); // 回転させる(度)
    TweenComponent& ColorTo(XMFLOAT4 target, float duration, EaseFunction ease = Ease::OutQuad);  // SpriteRendererの色を変える
    TweenComponent& TweenValue(float from, float to, float duration,
        std::function<void(float)> onUpdate, EaseFunction ease = Ease::OutQuad);                  // 好きな値を動かす

    TweenComponent& SetDelay(float seconds);                       // 始まるまで待つ時間
    TweenComponent& SetLoop(bool loop, bool pingPong = false);     // 繰り返す(pingPong=行って戻る)
    TweenComponent& SetOnComplete(std::function<void()> callback); // 終わった時に呼ぶ
    void Stop();                                                   // すべての動きを止める
};
```

**設計のポイント**:
- `SetDelay` / `SetLoop` / `SetOnComplete` は「直前に始めた動き」に効きます。自分自身を返すので、`tween->MoveTo(...).SetDelay(0.2f).SetOnComplete(...)` のようにつなげて書けます。
- 1つの `TweenComponent` で、位置と色を同時に動かすなど、複数の動きを同時に持てるようにします(同じ種類の動きを2つ始めたら、後から始めた方で上書き)。
- Componentなので、オブジェクトが消えると一緒に消えます。**`SetOnComplete` のラムダが、消えたオブジェクトを触る心配がありません**(EventBusより安全)。
- 動かす時間は `Time::GetDeltaTime()`(ポーズで止まる)を使います。ポーズ中も動かしたいUIのために、Unscaledを使う設定(`SetUnscaled(true)` など)を足すかは未決定。
- カメラ追従(5.7)や画面エフェクト(5.8)も、中で `Ease` の関数を使います。

**置き換えるSampleのコード(使用例)**:
```cpp
// タイトルのボタンを、ぴょこっと大きくして出す
auto* tween = button->AddComponent<TweenComponent>();
button->transform.scale = { 0.f, 0.f, 1.f };
tween->ScaleTo({ 300.f, 80.f, 1.f }, 0.4f, Ease::OutBack);
```
対象: 両サンプルの `Title` / `StartButton` の出し方(今は一瞬で表示されている)。(`StartButton` は置き換え済み。`Title` の文字は `TitleController::Draw` が `Text::Draw` で直接描いていてTransformを持たないので、今回は置き換えていない)

**依存する機能**: 5.2 時間とタイマー

---

### 5.5 移動系

> **実装済み**: `Engine/MoveComponent.h/.cpp`(親クラスと `enum class MovePlane`)、`Engine/ChaseComponent.h/.cpp`、`Engine/AccelerateChaseComponent.h/.cpp`、`Engine/HomingComponent.h/.cpp`。下の「対象」のSampleのコード(両サンプルの `EnemyController` の移動処理)も置き換え済み。使い方は `README.md` の「13. 移動(MoveComponent ほか)」を参照。
> 決まったこと(下の「未決定」):
> - **Rigidbodyが無い時の動かし方**: Rigidbody必須にはしない。DynamicのRigidbodyなら `SetVelocity`、Kinematic・Rigidbody無しなら `transform.position` を dt 分だけ直接動かす、Staticなら動かさない(出力ウィンドウに1回だけ知らせる)
> - **2Dと3D**: 1つのComponentで `SetPlane(MovePlane::XY / XZ)` を使う(既定XY)。XZの時はY方向の速度(`GetVelocity().y`、重力)を残す。XYの時はZ方向の速度を0にする
>
> 設計から変えた・足した所:
> - `ApplyVelocity` に `dt` の引数を足した(`ApplyVelocity(velocity, dt)`。transformを直接動かす時に使う)
> - 目的地は「`SetTarget` の相手 → `SetTargetPosition` の場所」の順。どちらも無ければ止まる(速度0)。**相手が非アクティブ(`SetActive(false)`・`Destroy()` 済み)の間は追わずに止まる**。相手が `Destroy()` されて本当に消えた後のポインタは見分けられないので、呼ぶ側の責任(先に `SetTarget(nullptr)` する)とし、コメントに書いた
> - 距離・向きは平面の上だけで計算する(XYならZ、XZならYの差を無視。置き換え前の両サンプルの `EnemyController` と同じ)。`StopDistance` の既定は1.0(置き換え前の「`DistanceSq < 1.0` で止まる」と同じ)
> - `ChaseComponent` は、1フレームで残りの距離より先へは進まない(行き過ぎて往復しないように)。Sampleの敵はPlayerと当たり判定でぶつかるので、この制限がかかる距離まで近づかず、動きは置き換え前と同じ
> - `AccelerateChaseComponent`: `SetMaxSpeed` は `SetSpeed` と同じもの(最高速度)にした。Dynamicなら `AddForce(dir * acceleration, ForceMode::Acceleration)`、Kinematic・Rigidbody無しなら自分で速度を持って積分する。空気抵抗(`SetDrag`)は、Rigidbodyの `SetDrag`(PhysXの減衰。重力で落ちる速さにも効く)は使わず、平面の上の速度だけに自分でかける(XZでYの速度を残すため)。`StopDistance` より近い間は加速しない(勢いで行き過ぎて戻ってくる)。Dynamicでは1フレーム分の加速だけ最高速度を超えることがある。ポーズ中(dt=0)は `AddForce` しない(PhysXが進まない間に力が貯まって、再開時に飛び出さないように)
> - `HomingComponent`: 進行方向(度)を持ち、`Math::MoveTowardsAngle` で1フレームに最大 `turnSpeed * dt` 度だけ曲げる。向きは `rotate` と同じ(XYは `rotate.z` = `AngleToTarget2D` / `Forward2D`、XZは `rotate.y` = `AngleToTargetY` / `Forward3D`)。最初の進行方向は、出した後の最初の `Update` の時の `rotate`。`SetTurnSpeed` の既定は180度/秒、負の値は0にする
> - `ObjectPool` で再利用された時(`OnActiveChanged(true)`)に、`AccelerateChaseComponent` は速度を0に、`HomingComponent` は進行方向を `rotate` から読み直す。相手・速さなどの設定は残る
> - `GetTarget()` / `GetSpeed()` を足した。`RigidbodyComponent` に `GetBodyType()` を足した(`ApplyVelocity` が剛体の種類を見るため)
> - Sampleの `EnemyController::Spawn` は、追いかける相手を受け取らなくなった(`Spawn(pool)`)。敵の速さ(60)は `EnemyManager` の `enemySpeed` に移した

**目的**: 「ターゲットに向かって動く」処理をComponentにして、敵・弾・ミサイルなどで使い回す。

**ファイル案**: `Engine/MoveComponent.h/.cpp`(親クラス)、`Engine/ChaseComponent.h/.cpp`、`Engine/AccelerateChaseComponent.h/.cpp`、`Engine/HomingComponent.h/.cpp`

**関数**:
```cpp
enum class MovePlane { XY, XZ };   // 動く平面(2D=XY、3Dの地面=XZ)

// 移動系の親クラス(単体ではAddComponentしない。コンストラクタはprotected)
class MoveComponent : public Component {
public:
    void SetTarget(GameObject* target);          // 追いかける相手
    void SetTargetPosition(XMFLOAT3 position);   // 決まった場所へ向かう(相手がいない時)
    void SetSpeed(float speed);                  // 速さ
    void SetStopDistance(float distance);        // これより近づいたら止まる
    void SetPlane(MovePlane plane);              // 動く平面
    bool HasArrived() const;                     // 着いたか(StopDistanceより近いか)
protected:
    void ApplyVelocity(XMFLOAT3 velocity);       // 速度を反映する(Rigidbodyがあれば物理で、無ければtransformで)
};

// 一定の速さでまっすぐ向かう(今のEnemyControllerの動き)
class ChaseComponent : public MoveComponent { };

// 加速して向かう。勢いがあるので行き過ぎて戻ってくる
class AccelerateChaseComponent : public MoveComponent {
public:
    void SetAcceleration(float acceleration);    // 加速の強さ
    void SetMaxSpeed(float maxSpeed);            // 最高速度
    void SetDrag(float drag);                    // 空気抵抗(大きいほどすぐ減速する)
};

// 曲がれる角度に上限がある追尾(ミサイル・車・魚向け)
class HomingComponent : public MoveComponent {
public:
    void SetTurnSpeed(float degreesPerSecond);   // 1秒に何度まで曲がれるか(急に向きを変えない)
    void SetFaceMovement(bool enabled);          // 見た目の向き(rotate)も進む方向に合わせる
};
```

**設計のポイント**:
- `ApplyVelocity` は、Dynamicの `RigidbodyComponent` が付いていれば `SetVelocity()` で動かします(CLAUDE.mdの「Dynamicな剛体はTransformを直接書き換えない」ルール)。
- `MovePlane::XZ` の時は、**Y方向の速度(重力で落ちる速さ)を `GetVelocity().y` から読んでそのまま残します**。今の Sample3D EnemyController がやっている工夫を、ここ1か所にまとめます。
- `AccelerateChaseComponent` は 5.6 の `AddForce(..., ForceMode::Acceleration)` で動かします。
- 将来、巡回(決まった点を回る)・逃げる・周回(相手の周りを回る)も、同じ親クラスから増やせます。
- **未決定**:
  - Rigidbodyが無い時の動かし方(`transform.position` を直接動かすか、Rigidbody必須にするか)
  - 2Dと3Dを1つのComponent(`SetPlane`)で兼ねるか、`Chase2DComponent` のように分けるか

**置き換えるSampleのコード(使用例)**:
```cpp
// EnemyManagerのSetupEnemy / Spawn
auto* chase = enemy->AddComponent<ChaseComponent>();
chase->SetTarget(player);
chase->SetSpeed(60.f);
```
対象: 両サンプルの `EnemyController::Update` の移動処理(`EnemyController` には「弾に当たったら倒れる」処理だけが残る)。

**依存する機能**: 5.1 計算系、5.6 物理の問い合わせと力

---

### 5.6 物理の問い合わせと力

> **実装済み**: `Engine/Physics.h/.cpp`(`struct RaycastHit`、`enum class ForceMode`、`Physics::Raycast` / `OverlapSphere` / `OverlapBox`)と `Engine/RigidbodyComponent.h/.cpp`(`AddForce` / `AddTorque` / `SetMass` / `SetDrag`)。下の「対象」のSampleのコード(Sample2Dの被弾時のノックバック)も置き換え済み。使い方は `README.md` の「17. PhysX」の「物理の問い合わせと力」を参照。
> 決まったこと(下の「未決定」):
> - **トリガーを含めるか**: 既定では含めない(視線の判定が飛んでいる弾などにさえぎられないように)。含めたい時用の引数 `QueryTriggers triggers = QueryTriggers::Ignore` を、`Raycast` / `OverlapSphere` / `OverlapBox` の最後に足した。アイテムやゴールのようなトリガーそのものを探す時は `QueryTriggers::Include`(例: `Physics::OverlapSphere(pos, 200.f, "Item", QueryTriggers::Include)`)。タグを指定してもトリガーは無視のまま
>
> 設計から変えた・足した所:
> - `enum class QueryTriggers { Ignore, Include }` を `Physics.h` に足した。最初は `bool includeTriggers = false` にしたが、呼ぶ側で `..., "Item", true)` と書くと `true` が何の意味か読めないので、名前で分かる `enum class` に変えた(`ForceMode` と同じ形)
> - トリガー同士が重なっても `OnTriggerEnter2D` / `OnTriggerExit2D` は呼ばれない(PhysX 4.1の仕様。`PxShape.h` / `PxSimulationEventCallback.h` に記載。Unityとは違う)。トリガー同士の重なりを知りたい時は、範囲の側が `Physics::OverlapBox(..., "Bullet", QueryTriggers::Include)` などで自分から探す(README「当たり判定」の注意に記載)
> - `RaycastHit` と `ForceMode` は `Physics.h` に置いた(`RigidbodyComponent.h` は `Physics.h` を読み込む)
> - 当たったアクターから `GameObject*` を取り出す方法は、当たり判定のコールバックと同じ(`actor->userData` の `Collider2D` の `owner`)。探す物はPhysXの `PxQueryFilterCallback`(preFilter)で選ぶ: タグ・トリガー・**非アクティブ/Destroy済み**(`SetActive(false)` したアクターがシーンから外れるのは次の `Physics::Update()` なので、その間も返さないように `GameObject` の状態で確かめる)。シーンから外れたアクターはPhysXが探さない
> - `Raycast` は、始点が中に入っている物には当たらない(PhysXは「距離0で当たった」と返すので、postFilterで外す。Unityと同じ。自分の中心から線を飛ばしても自分に当たらない)。`direction` は中で長さ1にする(長さ0・`maxDistance <= 0` ならfalse)。tag指定時は、そのタグの物のうち一番近い物を返す
> - `OverlapSphere` / `OverlapBox` は、見つかった数に上限が無い(64件ずつ受け取ってvectorに集める)。同じGameObjectは1回だけ入る。半径・大きさが0以下なら空。箱は回転しない
> - **`AddForce` / `AddTorque` は、PhysXのコールバックの中(`OnCollisionStay2D` など)で呼んでもよい**。PhysXはコールバックの中でのSDKへの書き込みを禁止している(`PxSimulationEventCallback` の注意書き)ので、`fetchResults()` の途中で呼ばれた時は `Physics` が預かり、`fetchResults()` とシーンへの追加/削除の予約が終わった直後に加える(Engine内部用 `Physics::ApplyForce`)。`CancelQueuedSceneChange` は預かった力も取り消す
> - ロックした軸(`SetFreezePositionX/Y/Z`・`SetFreezeRotation`)の向きの成分は、加える前に0にする(PhysXに任せず、確実にその向きへ動かないように)
> - Dynamic以外で呼ばれた時の「効きません」の表示は、Componentごとに1回だけ(毎フレーム呼ばれても出力ウィンドウが埋まらないように)。非アクティブ(シーンに入っていない)間の `AddForce` は何もしない
> - `SetMass` は `PxRigidBodyExt::setMassAndUpdateInertia`(回りにくさも計算し直す)。0以下は無視。`SetDrag` は `setLinearDamping`(負の値は0)。この2つはコールバックの中からは呼ばない(コメントに注意書き)
> - `GetMass()` を足した。Sample2DのPlayerは密度100で重さが約5200万あり、設計の使用例の `away * 500.f`(Impulse)ではほとんど動かないため、ノックバックの強さを「弾かれる速さ(800)× `GetMass()`」にした
> - Sample2Dのノックバック: 被弾してHPが残っている時だけ弾く(倒れた時は `Update` で速度0になるため)。弾かれている0.2秒間(`Timer`)は、`Update` で移動キーの `SetVelocity` をしない

**目的**: 「この線の先に何があるか」「この範囲に敵がいるか」を調べられるようにする。ノックバック・ジャンプなど、力で動かせるようにする。

**ファイル案**: 既存の `Engine/Physics.h/.cpp` と `Engine/RigidbodyComponent.h/.cpp` に追加

**関数**:
```cpp
struct RaycastHit {
    GameObject* object = nullptr;  // 当たったオブジェクト
    XMFLOAT3 point;                // 当たった場所
    XMFLOAT3 normal;               // 当たった面の向き
    float distance = 0.f;          // 当たるまでの距離
};

namespace Physics {
    // originからdirectionへ線を飛ばし、最初に当たった物を調べる(tagを指定するとそのタグだけ)
    bool Raycast(XMFLOAT3 origin, XMFLOAT3 direction, float maxDistance,
        RaycastHit& hit, const std::string& tag = "");
    // 球の中にいる物を全部探す(2Dでは円として使う)
    std::vector<GameObject*> OverlapSphere(XMFLOAT3 center, float radius, const std::string& tag = "");
    // 箱の中にいる物を全部探す
    std::vector<GameObject*> OverlapBox(XMFLOAT3 center, XMFLOAT3 size, const std::string& tag = "");
}

enum class ForceMode {   // Unityと同じ4種類
    Force,          // 力(質量で割る、時間をかける): 押し続ける
    Impulse,        // 衝撃(質量で割る、一瞬): ノックバック
    Acceleration,   // 加速度(質量を無視、時間をかける)
    VelocityChange  // 速度の変化(質量を無視、一瞬): ジャンプ
};

// RigidbodyComponentに追加
void AddForce(XMFLOAT3 force, ForceMode mode = ForceMode::Force);    // 力を加える
void AddTorque(XMFLOAT3 torque, ForceMode mode = ForceMode::Force);  // 回す力を加える
void SetMass(float mass);                                            // 重さ(今は密度から自動計算)
void SetDrag(float drag);                                            // 空気抵抗(速度が自然に落ちる)
```

**設計のポイント**:
- `AddForce` / `AddTorque` は **Dynamicにしか効きません**。Kinematic・Staticの時は何もせず、`OutputDebugStringA` で「Dynamicではないので効きません」と知らせます(今の `SetVelocity` も Dynamic専用)。
- PhysXの `PxForceMode` がUnityと同じ4種類なので、そのまま対応させます。
- 当たったPhysXの形から `GameObject*` を取り出す方法は、今の当たり判定のコールバックと同じ仕組み(`Physics.cpp`)を使います。
- トリガー(弾など、すり抜ける当たり判定)を `Raycast` / `Overlap` の対象に含めるかは未決定(含めるかを選ぶ引数を足す案がある)。
- `OnTriggerEnter2D` などのコールバックの中から呼んでも、読むだけなので安全です。

**置き換えるSampleのコード(使用例)**:
```cpp
// Sample2D PlayerController::OnCollisionStay2D: 敵に当たったら後ろに弾かれる
XMFLOAT3 away = Math::DirectionTo(info.other->owner->transform.position, t.position);
rigidbody->AddForce(away * 500.f, ForceMode::Impulse);
```
対象: Sample2D の被弾処理(ノックバックを付ける)。5.5 の `AccelerateChaseComponent` でも使う。

**依存する機能**: 5.1 計算系

---

### 5.7 カメラ機能

> **実装済み**: `Engine/CameraFollowComponent.h/.cpp`、`Engine/CameraShake.h/.cpp`(`namespace CameraShake`)、`Engine/Image.h/.cpp` に座標の変換(`WorldToScreen2D` / `ScreenToWorld2D` / `WorldToScreen3D` / `ScreenPointToRay3D`)。`Input::GetMouseWorldPosition()` は `Image::ScreenToWorld2D` を使う形にした(結果は今までと同じ)。`GameEngine::Run` に揺れの呼び出しを足した。下の「対象」のSampleのコード(Sample2D `PlayerController` のカメラ追従3行と、被弾時の揺れ)も置き換え済み。使い方は `README.md` の「14. カメラ(CameraFollowComponent / CameraShake / 座標の変換)」を参照。
> 決まったこと(下の「未決定」):
> - **2Dと3Dの座標の変換**: `Camera` に2D/3Dを持たせず、`Image` の関数を2D用・3D用で分けた(`Camera` クラスへの追加はしていない)。2D用は `Image::GetCamera()`、3D用は `Image::GetCamera3D()` を使う
>
> 設計から変えた・決めた所:
> - 画面座標は「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」(`Text::Draw` と同じ)
> - `WorldToScreen2D` は UIモードの `Sprite` と同じ計算((ワールド - カメラ位置) × 遠近の倍率 `focalLength / (z - カメラのz)`。カメラより手前は等倍)。`ScreenToWorld2D` はその逆で、Z=0 の平面の上の点を返す(戻り値は `XMFLOAT3`、zは0)
> - `WorldToScreen3D` は `Mesh` と同じ view/proj(アスペクト比も同じ)を掛けて w で割る。戻り値は `XMFLOAT3` で、**z = カメラからの奥行き(w)**。z が0以下はカメラの後ろで、x,y は0を返す(使う前に `z > 0` を確かめる。Unityの `WorldToScreenPoint` と同じ考え方)
> - `ScreenPointToRay3D` は view×proj の逆行列で、手前の面(深さ0)と奥の面(深さ1)の点に戻して線を作る。origin は nearZ の面の上の点、direction は長さ1
> - `CameraShake`: **描画の間だけ**カメラをずらす。`GameEngine::Run` で `Image::BeginFrame()`(3Dカメラを2Dカメラに合わせる処理)の後に `CameraShake::BeginDraw()` でずらし、シーンと文字を描き終わったら `CameraShake::EndDraw()` で**ずらす前に覚えた値をそのまま戻す**(足した分を引くのではないので、誤差も残らない)。揺れを進める `CameraShake::Update(Time::GetUnscaledDeltaTime())` は `Input::Update()` の直後(シーンのUpdateより前。そのフレームに `Shake` された揺れは、そのフレームの描画で一番強く出る)
> - `CameraShake`: 2D・3Dの両方のカメラを揺らす。3Dはカメラから見た右・上の向きに position と target を同じだけずらす。ずらす量は毎フレームばらばら(x,yそれぞれ -強さ〜+強さ)で、強さは `強さ × (1 - Ease::OutQuad(経過/時間))` で弱まる
> - `CameraShake`: 揺れている途中に `Shake` が呼ばれたら、今の強さ(弱まった後)と比べて、新しい方が強いか同じなら置き換える(最初から数え直す)。弱ければ無視する。`strength` か `duration` が0以下なら何もしない。`IsShaking()` を足した
> - `CameraFollowComponent`: `SetSmoothTime` は「残りの距離を1秒あたり一定の割合で縮める」式(`1 - e^(-3·dt/smoothTime)`。smoothTime秒で残り約5%)で、dtの刻み方によらない。`Update` の `dt` で進む(ポーズ中は止まる)
> - `CameraFollowComponent`: `SetBounds` は**画面に映る範囲**(Z=0の平面。カメラのzから画面の半分の大きさを求めて考える)。範囲が画面より狭い向きは真ん中に固定。min/maxが逆でも並べ直す。`SetDeadZone` は、対象がいつも映る位置を中心とした四角で、出た分だけカメラの行き先を動かす
> - `CameraFollowComponent`: 最初の `Update` と `SetActive(true)` で戻った時は、なめらかに追わずに一気に合わせる。動かすのはカメラの position の x,y だけ(z・target はそのまま)。対象の位置は `GetWorldTransform()`
> - Sample2D `Player.cpp`: Componentの順番を「`SpriteRenderer` → `CircleRigidbodyComponent` → `CameraFollowComponent` → `PlayerController`」にした(物理の結果が反映された位置へカメラを動かしてから、そのカメラでマウスのワールド座標を出す。置き換え前は `PlayerController` が先で、カメラが1フレーム古いPlayerの位置を追っていた)。SmoothTimeは0(置き換え前と同じピッタリ)
> - Sample2D の被弾時の揺れは `CameraShake::Shake(6, 0.2)`(`PlayerController.h` の `damageShakeStrength` / `damageShakeDuration`)
> - 両サンプルの `ButtonController::Draw` の文字の位置を `Image::WorldToScreen2D` に置き換えた(作った関数を使う所を作るため。ボタンはZ=0なので結果は同じ)
> - `WorldToScreen3D` / `ScreenPointToRay3D` は、今はSampleで使っていない(5.10 `GaugeRenderer`・5.11 `TextRenderer` で使う予定)。5.10・5.11 の「`WorldToScreen`」は、2Dなら `WorldToScreen2D`、3Dなら `WorldToScreen3D` のこと
> - Sample3D の `CameraController`(3Dのカメラ操作)は変えていない

**目的**: カメラの追従・揺れ・座標の変換を、毎回手書きしなくて済むようにする。

**ファイル案**: `Engine/CameraFollowComponent.h/.cpp`、`Engine/CameraShake.h/.cpp`(`namespace CameraShake`)、既存の `Engine/Camera.h` に追加

**関数**:
```cpp
// 追いかけてほしいオブジェクト(Playerなど)に付ける。2Dカメラ(Image::GetCamera())を動かす
class CameraFollowComponent : public Component {
public:
    void SetOffset(XMFLOAT2 offset);           // 対象からどれだけずらして映すか
    void SetSmoothTime(float seconds);         // 追いつくまでの時間(0=ピッタリ追従)
    void SetBounds(XMFLOAT2 min, XMFLOAT2 max);// カメラが動ける範囲(ステージの外を映さない)
    void SetDeadZone(XMFLOAT2 size);           // この範囲内の動きではカメラを動かさない(画面酔い対策)
};

namespace CameraShake {
    void Shake(float strength, float duration);  // 画面を揺らす(強さ, 秒)
}

// Cameraクラスに追加
XMFLOAT2 WorldToScreen(XMFLOAT3 worldPosition) const;   // ワールド座標→画面座標(3Dの敵の頭上にHPバー)
XMFLOAT3 ScreenToWorld(XMFLOAT2 screenPosition) const;  // 画面座標→ワールド座標
void ScreenPointToRay(XMFLOAT2 screenPosition,
    XMFLOAT3& origin, XMFLOAT3& direction) const;       // 画面の点から奥へ伸びる線(Raycastと組み合わせてクリックした物を探す)
```

**設計のポイント**:
- **揺れはカメラの `position` に直接足しません。** 描画する時だけずらします。直接足すと、`CameraFollowComponent` の追従とぶつかって、カメラがガタガタしたり、元の位置に戻らなくなったりするためです。
- 揺れの時間はポーズの影響を受けない `Time::GetUnscaledDeltaTime()` を使います。
- 画面座標は「画面中央が(0,0)、Y+=上」(`Text::Draw` と同じ)にそろえます。`WorldToScreen` の結果をそのまま `Text::Draw` に渡せます。
- `ScreenToWorld` は、今の `Input::GetMouseWorldPosition()`(2Dカメラ専用)を一般化したものです。
- 2D用カメラ(`Image::GetCamera()`、疑似的な正射影)と3D用カメラ(`Image::GetCamera3D()`、透視投影)では計算式が違います。`Camera` に「2Dか3Dか」を持たせるか、`Image::WorldToScreen2D/3D` のように分けるかは未決定。
- 3D用のカメラ操作は、今は Sample3D の `CameraController` が担当しています。`CameraFollowComponent` はまず2Dカメラ用として作ります。

**置き換えるSampleのコード(使用例)**:
```cpp
// Sample2D Player.cpp(今はPlayerController::Updateにカメラ追従の3行がある)
auto* follow = player->AddComponent<CameraFollowComponent>();
follow->SetSmoothTime(0.15f);

// 敵に当たった時
CameraShake::Shake(8.f, 0.2f);
```
対象: Sample2D `PlayerController` のカメラ追従3行(`Camera& camera = Image::GetCamera();` と `position.x/y` の代入)。

**依存する機能**: 5.1 計算系、5.2 時間とタイマー、5.4 イージング、5.6(`ScreenPointToRay` と `Raycast` の組み合わせ)

---

### 5.8 画面エフェクト

**目的**: 被弾・回復・シーン切り替えなどで、画面全体に色の演出(ライトエフェクト)を付ける。

**ファイル案**: `Engine/ScreenEffect.h/.cpp`(`namespace ScreenEffect`)

**関数**:
```cpp
namespace ScreenEffect {
    void Flash(XMFLOAT4 color, float duration);                    // 画面全体が一瞬光る
    void Vignette(XMFLOAT4 color, float duration, int blinkCount); // 画面の端だけ色付きで点滅(被弾で赤く)
    void Fade(XMFLOAT4 color, float toAlpha, float duration);      // 暗転・明転(シーン切り替えにも使う)
    bool IsFading();                                               // フェード中か
}
```

**設計のポイント**:
- 作り方の案: 「中央が透明で、端だけ色の付いた画像」(ビネット用)と「真っ白な画像」(フラッシュ・フェード用)を画面全体に重ねて、色と透明度を時間で変えます。**シェーダーの変更は不要です。**
- 描画の順番は「ゲーム画面 → エフェクト → HUDの文字」。今の `GameEngine::Run` では文字が必ず一番上に出るので、この順番は今のままで満たせます。ただし**暗転(Fade)で文字まで隠したい時は、`GameEngine::Run` の描画順を変える必要があります**(どちらにするか未決定)。
- 重ねる画像はカメラの影響を受けない描き方が必要です(5.10 の `Image::DrawRect` と同じ仕組み)。
- 時間はポーズの影響を受けない `Time::GetUnscaledDeltaTime()` を使います(ポーズ中のフェードも動くように)。
- 3D空間の点光源・スポットライト(たいまつ・懐中電灯)は**別の機能**です。今の `Light` は平行光1つだけで、増やすにはシェーダーの変更が必要になります。

**置き換えるSampleのコード(使用例)**:
```cpp
// PlayerController::OnCollisionStay2D: ダメージを受けた時
ScreenEffect::Vignette({ 1.f, 0.f, 0.f, 0.6f }, 0.5f, 2);

// StartButtonを押した時: 暗転してからシーンを切り替える
ScreenEffect::Fade({ 0.f, 0.f, 0.f, 1.f }, 1.f, 0.5f);
```
対象: 両サンプルの被弾処理(今は色が黄色になるだけ)、`StartButton` のシーン切り替え。

**依存する機能**: 5.2 時間とタイマー(Unscaled)、5.4 イージング、5.10 の `Image::DrawRect`(先に作るか、ここで一緒に作る)

---

### 5.9 2Dアニメーション

**目的**: スプライトシートのコマ送りを、手書きのタイマー無しで書けるようにする。

**ファイル案**: `Engine/SpriteAnimationComponent.h/.cpp`

**関数**:
```cpp
class SpriteAnimationComponent : public Component {
public:
    void AddClip(const std::string& name, std::vector<int> frames,
        float frameDuration, bool loop);   // アニメーションを登録(名前, コマ番号の並び, 1コマの秒数, 繰り返すか)
    void Play(const std::string& name);    // 再生(同じ名前を毎フレーム呼んでも最初に戻らない)
    void Stop();                           // 止める
    bool IsFinished() const;               // ループしないアニメーションが最後まで行ったか
    void SetSpeed(float speed);            // 再生の速さ(1=通常、0=一時停止)
    void SetRandomStart(bool enabled);     // 始まるコマをランダムにする(敵の足踏みがそろわないように)
};
```

**設計のポイント**:
- **一番大事なのは、`Play` に今と同じ名前を渡した時は何もしないことです。** Controllerは毎フレーム `Play("Walk")` を呼ぶので、そのたびに最初のコマに戻ると、アニメーションが止まって見えます。
- `ObjectPool` で使い回された時は、`OnActiveChanged(true)` で最初のコマに戻します(前回の続きから始まらないように)。
- **役割分担**: このComponentは「コマを送る」だけです。どのアニメーションを再生するかは、Controllerが `if` で決めます。Unityのような「状態遷移図(Animator)」は作りません(初心者には `if` の方が読みやすいため)。
- 5.3 で、コマ番号が `SpriteRenderer` 側に移っていることが前提です(移っていないと、同じ画像の全員が同じコマになる)。
- **未決定**:
  - Engineに作るか、Sample3Dだけに作るか(今アニメーションを使っているのはSample3Dの敵だけ)
  - 名前付きのクリップを複数持てるようにするか、1種類だけにするか
  - ループしないアニメーションが終わったことを知る方法(`IsFinished` / 終わった時のコールバック / 今は不要)

**置き換えるSampleのコード(使用例)**:
```cpp
// 組み立て時(SetupEnemy)
auto* anim = enemy->AddComponent<SpriteAnimationComponent>();
anim->AddClip("Walk", { 4, 5 }, 0.3f, true);
anim->AddClip("Idle", { 4 }, 1.0f, true);

// Controller::Update
if (moving) anim->Play("Walk"); else anim->Play("Idle");
```
対象: Sample3D `EnemyController` の `frameTimer` / `frameIndex`(5.2 で `Timer` に置き換え済みのコマ送り)と `walkFrames` の計算。

**依存する機能**: 5.2 時間とタイマー、5.3 リソースの使い回し(コマ番号をSpriteRendererへ移す)

---

### 5.10 UI

**目的**: HPバー・音量スライダー・ボタンを、どのゲームでも使える部品にする。画面サイズが変わってもずれないようにする。

**ファイル案**: `Engine/Anchor.h`、`Engine/GaugeRenderer.h/.cpp`、`Engine/SliderComponent.h/.cpp`、`Engine/ButtonComponent.h/.cpp`、`Engine/Image.h/.cpp` に `DrawRect` を追加

**関数**:
```cpp
// ── 土台 ──
enum class Anchor {   // 画面のどこを基準にするか
    TopLeft, Top, TopRight,
    Left, Center, Right,
    BottomLeft, Bottom, BottomRight
};
XMFLOAT2 GetAnchoredPosition(Anchor anchor, XMFLOAT2 offset);  // 基準の位置+ずらす量 → 画面座標

namespace Image {
    void DrawRect(XMFLOAT2 position, XMFLOAT2 size, XMFLOAT4 color);  // 画面に四角を塗る(カメラの影響を受けない)
}

// ── HPバーなどのゲージ ──
class GaugeRenderer : public Component {
public:
    void SetValue(float current, float max);                // 今の値と最大値
    void SetColors(XMFLOAT4 fill, XMFLOAT4 background);     // 中身の色と背景の色
    void SetDelayedBar(bool enabled, XMFLOAT4 color);       // 減った分を少し遅れて縮める(ダメージ量が分かる)
    void SetFollowTarget(GameObject* target, XMFLOAT2 offset); // オブジェクトの上に表示(敵の頭上)
    void SetAnchor(Anchor anchor, XMFLOAT2 offset);         // 画面に固定して表示(左上のHPなど)
};

// ── スライダー ──
class SliderComponent : public Component {
public:
    void  SetRange(float min, float max);                   // 値の範囲
    void  SetValue(float value);                            // 値を設定
    float GetValue() const;                                 // 今の値
    void  SetStep(float step);                              // 刻み幅(0=刻まない)
    void  SetOnChanged(std::function<void(float)> callback);// 値が変わった時に呼ぶ(例: 音量を変える)
};

// ── ボタン ──
class ButtonComponent : public Component {
public:
    void SetOnClick(std::function<void()> callback);   // クリックされた時に呼ぶ
    void SetInteractable(bool enabled);                // falseで押せなくする(灰色にする)
    // ほか、今のButtonControllerにある文字の表示など
};
```

**設計のポイント**:
- 画面座標は「画面中央が(0,0)、Y+=上」(`Text::Draw` と同じ)。`GetAnchoredPosition` は `Graphics::width` / `Graphics::height` から計算するので、画面サイズが変わってもずれません。
- UIモードの `SpriteRenderer` は2Dカメラと一緒にスクロールしてしまうため、`Image::DrawRect` は**カメラの影響を受けない**描き方にします(中身は1x1の白い画像を、色と大きさを変えて描く)。
- `GaugeRenderer` の `SetFollowTarget` は、5.7 の `WorldToScreen` で頭上の画面座標を求めます。
- `ButtonComponent` は、今 Sample2D と Sample3D に**まったく同じ内容でコピーされている** `ButtonController` をEngineへ移したものです。移したら両サンプルの `ButtonController` を消して、`Objects/Button.cpp` から `ButtonComponent` を使います。
- 値を表示するだけで、値の持ち主にはなりません(READMEの「値の持ち主は1か所、HUDは読むだけ」と同じ)。

**置き換えるSampleのコード(使用例)**:
```cpp
// HUDController::Draw(-560, 320 の直書きを置き換え)
XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, { 40.f, -40.f });
Text::Draw(buf, pos.x, pos.y, scoreStyle);   // TextStyleは5.11

// Player.cpp: 頭上のHPバー
auto* gauge = player->AddComponent<GaugeRenderer>();
gauge->SetFollowTarget(player, { 0.f, 70.f });
```
対象: 両サンプルの `HUDController` の座標、両サンプルの `ButtonController`。

**依存する機能**: 5.1 計算系、5.7 カメラ機能(`WorldToScreen`)、5.11 テキスト、5.12 セーブ/ロード(スライダーの値の保存)

---

### 5.11 テキスト

**目的**: 文字の左寄せ・縁取り・自前のフォントを使えるようにする。オブジェクトに付いて動く文字(ダメージの数字など)を出せるようにする。

**ファイル案**: 既存の `Engine/Text.h/.cpp` に追加、`Engine/TextRenderer.h/.cpp`

**関数**:
```cpp
enum class TextAlign { Left, Center, Right };   // 文字の寄せ方

struct TextStyle {
    float size = 32.f;                          // 文字の大きさ
    XMFLOAT4 color = { 1.f, 1.f, 1.f, 1.f };    // 文字の色
    std::wstring fontName = L"Arial";           // フォントの名前
    TextAlign align = TextAlign::Center;        // 寄せ方(今のText::Drawと同じく中央が既定)
    bool bold = false;                          // 太字
    XMFLOAT4 outlineColor = { 0.f, 0.f, 0.f, 0.f }; // 縁取りの色(a=0なら縁取り無し)
};

namespace Text {
    std::wstring LoadFont(const std::wstring& path);  // Assets/Fonts/○○.ttf を読み込み、使う時の名前を返す
    void Draw(const std::wstring& text, float x, float y, const TextStyle& style); // スタイルを指定して描く
    XMFLOAT2 Measure(const std::wstring& text, const TextStyle& style);           // 描いた時の幅と高さ
}

// オブジェクトに付いて一緒に動く文字(ダメージの数字・キャラクターの名前)
class TextRenderer : public Component {
public:
    void SetText(const std::wstring& text);   // 表示する文字
    void SetStyle(const TextStyle& style);    // 見た目
};
```

**設計のポイント**:
- 今の `Text::Draw` でも、PCにインストール済みのフォント名なら `fontName` で指定できます。ただし、**インストールしていないPCでは別のフォントになってしまいます**。`LoadFont` で `Assets/Fonts/` のフォントファイルを読み込むと、チーム全員のPCで同じ見た目になります。
- DirectWriteは、普通のフォントの追加方法(`AddFontResourceEx`)では見つけてくれないことがあるので、フォントファイルから独自のフォント一覧を作る方法(Windows 10以降の `IDWriteFactory5` のフォントセット)を使う予定です。
- 今の `Text::Draw` は指定した座標に文字の**中心**を合わせます。`TextStyle::align` の既定を `Center` にして、今の見た目が変わらないようにします。
- 縁取りは、同じ文字を少しずらして縁の色で数回描いてから、上に本体を描く方法が一番簡単です。
- 今の `Text::Draw` は、呼ばれる度にフォント設定(`IDWriteTextFormat`)を作り直しています。`TextStyle` ごとに作ったものを取っておいて使い回すようにします(`IDWriteTextLayout` の解放漏れは修正済み)。
- `TextRenderer` の位置は、5.7 の `WorldToScreen` で画面座標に直して描きます。

**置き換えるSampleのコード(使用例)**:
```cpp
// HUDController: スコアを左寄せ・縁取りで表示
TextStyle style;
style.size = 28.f;
style.align = TextAlign::Left;
style.outlineColor = { 0.f, 0.f, 0.f, 1.f };
Text::Draw(buf, pos.x, pos.y, style);
```
対象: 両サンプルの `HUDController`、`ButtonController` の文字。

**依存する機能**: 5.7 カメラ機能(`TextRenderer` の `WorldToScreen`)

---

### 5.12 セーブ/ロード

**目的**: ハイスコア・音量などを保存して、次に起動した時も残るようにする。

**ファイル案**: `Engine/SaveData.h/.cpp`(`namespace SaveData`)

**関数**:
```cpp
namespace SaveData {
    void  SetInt(const std::string& key, int value);                          // 整数を書く
    int   GetInt(const std::string& key, int defaultValue = 0);               // 整数を読む(無ければdefaultValue)
    void  SetFloat(const std::string& key, float value);                      // 小数を書く
    float GetFloat(const std::string& key, float defaultValue = 0.f);         // 小数を読む
    void  SetString(const std::string& key, const std::wstring& value);       // 文字を書く
    std::wstring GetString(const std::string& key, const std::wstring& defaultValue = L""); // 文字を読む
    bool  HasKey(const std::string& key);                                     // そのキーがあるか
    void  DeleteKey(const std::string& key);                                  // そのキーを消す

    bool  Save(int slot = 0);   // ファイルに書き出す
    bool  Load(int slot = 0);   // ファイルから読み込む(起動時にEngineが自動で呼ぶ)
}
```

**設計のポイント**:
- 使い方はUnityの `PlayerPrefs` と同じです(Set/Getで読み書きして、`Save` でファイルに書く)。
- 保存先は `%LOCALAPPDATA%\3D_FreamWork\`(例: `save0.txt`)。exeの隣(`x64\Debug\`)は、ビルドし直したりフォルダを消したりすると消えることがあるためです。
- 形式は `HighScore=1200` のような、人が読める文字にします(UTF-8)。デバッグ中にメモ帳で開いて中身を確認・書き換えできます。
- 保存する時は、いったん一時ファイルに書いてから本物と差し替えます。保存中にゲームが落ちても、前のセーブデータが壊れません。
- キーの名前は、EventBusのイベント名と同じくPascalCaseの英語にします(`"HighScore"`, `"MusicVolume"`)。
- 終了時(`GameEngine::Uninit`)に自動で `Save` するかは未決定。

**置き換えるSampleのコード(使用例)**:
```cpp
// GameManager: ゲームオーバーの時にハイスコアを更新
if (score > SaveData::GetInt("HighScore")) {
    SaveData::SetInt("HighScore", score);
    SaveData::Save();
}
```
対象: 両サンプルの `GameManager`(ハイスコアを足す)と `HUDController`(表示)。

**依存する機能**: なし(学生の練習に向いている)

---

### 5.13 音の発生源と聞く位置(立体音響)

**目的**: 音を「どこで鳴っているか」で聞こえ方が変わるようにする(遠い音は小さく、左の音は左から)。同じ効果音を重ねて鳴らせるようにする。BGMと効果音の音量を別々に変えられるようにする。

**ファイル案**: `Engine/AudioSourceComponent.h/.cpp`、`Engine/AudioListenerComponent.h/.cpp`、既存の `Engine/Audio.h/.cpp` に追加

**関数**:
```cpp
enum class AudioGroup { BGM, SE };   // 音のグループ(略語の書き方はCLAUDE.mdの命名規則を参照)

// 音を鳴らすオブジェクトに付ける
class AudioSourceComponent : public Component {
public:
    explicit AudioSourceComponent(const std::wstring& path);  // 鳴らす音のファイル
    void Play();                                     // 鳴らす
    void Stop();                                     // 止める
    void SetVolume(float volume);                    // 音量(0〜1)
    void SetLoop(bool loop);                         // 繰り返すか
    void SetSpatial(bool enabled);                   // 位置で聞こえ方を変えるか(false=BGM・UIの音)
    void SetDistance(float minDistance, float maxDistance); // minまでは最大音量、maxで聞こえなくなる
};

// 聞く側に1つだけ付ける(Playerかカメラ)
class AudioListenerComponent : public Component { };

namespace Audio {
    void PlayOneShot(const std::wstring& path, float volume = 1.f);         // 鳴らしっぱなし(重ねて鳴らせる)
    void PlayOneShotAt(const std::wstring& path, XMFLOAT3 position);        // その場所で鳴らす
    void SetGroupVolume(AudioGroup group, float volume);                    // グループごとの音量
}
```

**設計のポイント**:
- 距離による音の大きさと左右の聞こえ方は、XAudio2に付いている **X3DAudio** で計算します。2Dゲームでも、左にいる敵の音は左から聞こえます。
- **「音のデータ(共有)」と「再生口(鳴らす度に用意)」を分けます。** 音のデータは `Resources::LoadSound` で1回だけ読み(5.3 では画像だけを作ったので、`LoadSound` / `ReleaseSound` はこの機能で `Resources` に足す)、鳴らす度に再生口(`IXAudio2SourceVoice`)を用意します。これで「同じ効果音を重ねて鳴らせない」問題も直ります。
- 再生口を毎回作って捨てるのは重いので、鳴り終わった再生口を使い回します。同時に鳴らせる数の上限(例: 32個)も決めます。
- グループの音量は、XAudio2のサブミックスボイス(音をまとめる途中の口)で実装します。5.10 のスライダーと 5.12 のセーブ/ロードと組み合わせて、音量設定を保存できます。
- `AudioListenerComponent` が2つ以上ある時・1つも無い時の扱い(警告を出す、カメラの位置を使う)は未決定。
- 今の `Audio::Load` / `Play`(IDで鳴らす方法)は、すぐには消さずに残します。

**置き換えるSampleのコード(使用例)**:
```cpp
// BulletManager::Fire: 撃つ度に発射音(連射しても途切れない)
Audio::PlayOneShotAt(L"Assets/Sounds/shot.wav", position);

// Player.cpp
player->AddComponent<AudioListenerComponent>();
```
対象: 両サンプルの発射音・敵を倒した音(今は音が無いので、足して確かめる)。

**依存する機能**: 5.3 リソースの使い回し、5.10 UI(スライダー)、5.12 セーブ/ロード

---

### 5.14 CSV

**目的**: 敵の強さや出現の順番などを、コードではなく表(Excel)で調整できるようにする。

**ファイル案**: `Engine/Csv.h/.cpp`

**関数**:
```cpp
class Csv {
public:
    static Csv Load(const std::wstring& path);   // 読み込む(1行目は列の名前として扱う)
    bool IsValid() const;                        // 読み込めたか
    int  GetRowCount() const;                    // 行の数(1行目の列の名前は数えない)
    std::wstring GetString(int row, const std::string& column) const;               // 文字として読む
    int   GetInt(int row, const std::string& column, int defaultValue = 0) const;   // 整数として読む
    float GetFloat(int row, const std::string& column, float defaultValue = 0.f) const; // 小数として読む
    int   FindRow(const std::string& column, const std::wstring& value) const;      // その列がvalueの行を探す(無ければ-1)
};
```

**設計のポイント**:
- 使い道: 敵のパラメータ表(名前・HP・速さ)、出現ウェーブ表(何秒後に何体)、ステージの配置図(マス目に数字を書く)。
- **列の名前で取り出す**ので、Excelで列の順番を入れ替えても壊れません。列の名前は半角英数字(`Name`, `Hp`, `Speed`)にします。
- ステージの読み込みは、Sceneに直接書かず、工場関数にします(「Sceneは配置だけ」のルール)。
  ```cpp
  // GameScene::Init
  CreateStage(*this, L"Assets/Data/Stage1.csv");
  ```
- **注意**: 日本語版のExcelは、普通に保存すると **Shift-JIS** という文字コードになり、日本語が文字化けします。「**CSV UTF-8(コンマ区切り)**」で保存する決まりにします。Shift-JISのファイルを読んだ時は、`OutputDebugStringA` で警告を出します。
- `"`(ダブルクォート)で囲まれた値・値の中のカンマにも対応します(Excelが書き出すため)。

**置き換えるSampleのコード(使用例)**:
```cpp
// Assets/Data/Enemies.csv
// Name,Hp,Speed
// Slime,1,60
// Bat,2,120

Csv table = Csv::Load(L"Assets/Data/Enemies.csv");
int row = table.FindRow("Name", L"Slime");
float speed = table.GetFloat(row, "Speed", 60.f);
```
対象: 両サンプルの `EnemyManager` の `spawnInterval` / `spawnDistance` と、敵の `speed`(今はヘッダーに直書き)。

**依存する機能**: なし(いつでも並行して作れる。学生の練習に向いている)

---

### 5.15 パーティクル

**目的**: 爆発・煙・火花などを、たくさんの小さな粒で表現する。

**ファイル案**: `Engine/ParticleRenderer.h/.cpp`、`Engine/Effect.h/.cpp`(使い捨てエフェクトの工場関数)

**関数**:
```cpp
struct ParticleSettings {
    std::wstring texture;              // 粒の画像
    float emitRate = 10.f;             // 1秒に出す数
    float lifetimeMin = 0.5f, lifetimeMax = 1.0f;  // 粒が消えるまでの時間(この間でランダム)
    float speedMin = 50.f, speedMax = 100.f;       // 粒の速さ(この間でランダム)
    XMFLOAT3 direction = { 0.f, 1.f, 0.f };        // 出る向き
    float spreadAngle = 30.f;          // 向きのばらつき(度)
    XMFLOAT3 gravity = { 0.f, 0.f, 0.f };          // 重力(粒が落ちていく)
    float startSize = 16.f, endSize = 0.f;         // 出た時と消える時の大きさ
    XMFLOAT4 startColor = { 1.f, 1.f, 1.f, 1.f };  // 出た時の色
    XMFLOAT4 endColor = { 1.f, 1.f, 1.f, 0.f };    // 消える時の色
    bool additive = false;             // 加算合成(重なるほど明るくなる。炎・光向け)
    bool worldSpace = true;            // true=出た場所に残る(煙)、false=オブジェクトと一緒に動く(オーラ)
};

class ParticleRenderer : public Component {
public:
    explicit ParticleRenderer(const ParticleSettings& settings);
    void Play();               // 出し始める
    void Stop();               // 新しく出すのをやめる(出ている粒は寿命まで残る)
    void Burst(int count);     // 一度にcount個出す(爆発)
    bool IsPlaying() const;    // 出している、または粒が残っているか
};

// 使い捨てのエフェクトを作る(粒がすべて消えたら、オブジェクトも自動で消える)
GameObject* CreateEffect(Scene& scene, const ParticleSettings& settings, XMFLOAT3 position);
```

**設計のポイント**:
- **一番大事なのは、粒1つ1つをGameObjectにしないことです。** 粒が数百個になると、GameObjectとComponentの処理だけでとても重くなります。粒は `ParticleRenderer` の中のデータの配列(位置・速度・残り時間)としてまとめて持ち、まとめて描画します。
- 加算合成(`additive`)には、今の `Sprite` とは別のブレンドの設定が必要です(シェーダーの変更は不要)。
- `CreateEffect` で作ったオブジェクトは、`IsPlaying()` が false になったら `Destroy()` します。
- 画像は 5.3 の `Resources` で共有します(爆発を何十回出しても画像の読み込みは1回)。

**置き換えるSampleのコード(使用例)**:
```cpp
// EnemyController::OnTriggerEnter2D: 弾に当たった時に爆発
ParticleSettings explosion;
explosion.texture = L"Assets/particle.png";
explosion.spreadAngle = 360.f;
explosion.additive = true;
GameObject* effect = CreateEffect(*GetOwner()->GetScene(), explosion, t.position);
effect->GetComponent<ParticleRenderer>()->Burst(30);
```
対象: 両サンプルの「敵を倒した時」(今は消えるだけ)。

**依存する機能**: 5.1 計算系、5.2 時間とタイマー、5.3 リソースの使い回し

---

### 5.16 FBX読み込みと3Dアニメーション

**目的**: Mayaで作ったモデルとアニメーションを、ゲームで使えるようにする。

**ファイル案**: `ThirdParty/ufbx/`(外部ライブラリ)、`Engine/Model.h/.cpp`、`Engine/ModelRenderer.h/.cpp`、`Engine/ModelAnimationComponent.h/.cpp`

**外部ライブラリ**: **ufbx**(github.com/ufbx/ufbx、MITライセンス)を使う予定です。`ufbx.h` と `ufbx.c` の2ファイルだけなので、`ThirdParty/ufbx/` に置いてEngineと一緒にビルドできます。**ダウンロードはまだ了承をもらっていません。** 追加する前に、ライブラリの追加とライセンスファイルの扱いについて了承をもらいます。

**関数**:
```cpp
// 読み込んだモデル(みんなで共有するデータ)
class Model {
public:
    static std::shared_ptr<Model> Load(const std::wstring& path);  // .fbx/.objを読む(Resources::LoadModelからも使う)
    int GetNodeCount() const;                                       // パーツ(ノード)の数
    int FindNode(const std::string& name) const;                    // 名前でパーツを探す(無ければ-1)
};

// モデルを描画する
class ModelRenderer : public Component {
public:
    explicit ModelRenderer(const std::wstring& path);         // モデルのファイル
    void SetTexture(const std::wstring& path);                // 画像
    void SetColor(float r, float g, float b, float a);        // 色
    Transform* GetNodeTransform(const std::string& name);     // パーツを手で動かす(砲台だけ回す)
};

// アニメーションを再生する
class ModelAnimationComponent : public Component {
public:
    void Play(const std::string& clipName, bool loop = true); // 再生(Mayaで付けたクリップ名)
    void CrossFade(const std::string& clipName, float seconds); // 今の動きから滑らかに切り替える(再生中と同じ名前なら何もしない)
    bool IsFinished() const;                                   // ループしない動きが終わったか
    void SetSpeed(float speed);                                // 再生の速さ
    std::vector<std::string> GetClipNames() const;             // 入っているクリップ名の一覧(確認用)
};
```

**設計のポイント**:
- **パーツ(ノード)の親子関係を保ったまま持ちます。** 全部を1つのメッシュにまとめてしまうと、後から「砲台だけ回す」「腕だけ動かす」ができなくなります。
- モデルのデータは `Resources::LoadModel`(5.3 では作らなかったので、この機能で `Resources` に足す)で共有し、パーツの動き(ノードの位置・回転)は `ModelRenderer` が1体ごとに持ちます(5.3 と同じ「データと状態を分ける」考え方)。
- **単位と座標系**は読み込む時に変換します。
  - 単位: このエンジンは「100単位=1m」(1単位=1cm)なので、Mayaの既定(cm)ならそのままの数字で合います。mで作ったデータは100倍します。
  - 座標系: Mayaは右手系、DirectXは左手系なので、Zの向きと三角形の頂点の順番を反転します(ufbxの読み込みオプションで指定できます)。
- 今の `Mesh::LoadOBJ` は残します(簡単なモデルはOBJで十分)。
- **3段階に分けて作ります**:
  1. **FBXの読み込み(静止)**: 形と画像だけ表示する
  2. **パーツの動き(ノードアニメーション)**: パーツごとの位置・回転のキーフレームを再生する(戦車・ロボットなど)
  3. **骨の動き(スキンアニメーション)**: 骨に合わせて体が曲がる(人・動物)。頂点に「どの骨に・どれだけ影響されるか」(骨番号と影響度)を持たせ、骨の行列を送る定数バッファと、骨用のシェーダーが必要です。今の `MeshVertex` / `Shader3D.hlsl` は変えずに、**別の頂点形式・別のシェーダーとして足します**(新しいシェーダーファイルも全サンプル共通ですが、ロードマップの機能の範囲なので了承は不要です。着手する時の説明に、追加するシェーダーファイルと両サンプルへの影響を含めます)。
- **Maya側の作業**:
  - 骨を作る(Skeleton)→ 体に骨を付ける(Bind Skin)→ キーフレームを打つ
  - FBXで書き出す時に「Animation」にチェックを入れる
  - 複数の動き(歩く・走る・攻撃)は、Game Exporter でクリップに分けて書き出す
  - 元の `.mb` ファイルも `Assets/Source/` などに残す(後で直せるように)

**置き換えるSampleのコード(使用例)**:
```cpp
// Sample3D Player.cpp(今は立方体のMeshRenderer)
player->AddComponent<ModelRenderer>(L"Assets/Models/Robot.fbx");
auto* anim = player->AddComponent<ModelAnimationComponent>();

// PlayerController::Update
if (moving) anim->CrossFade("Run", 0.2f); else anim->CrossFade("Idle", 0.2f);
```
対象: Sample3D の Player(立方体)、敵(今は2Dの画像)。

**依存する機能**: 5.3 リソースの使い回し、5.2 時間とタイマー、5.1 計算系
