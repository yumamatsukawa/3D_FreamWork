---
name: add-object
description: Sample2D/Sample3Dに新しい種類のゲームオブジェクトを、このプロジェクトの規約(Objects/の工場関数+Components/のController)どおりに追加する。「○○を追加して」「敵/アイテム/ギミック/UIを作りたい」など、新しいオブジェクトを作る時に使う。
argument-hint: "[Sample2D|Sample3D] [オブジェクト名] [何をするオブジェクトか]"
---

# オブジェクトの追加

## 1. 決めること

次の3つが依頼から読み取れない時は、作り始める前にユーザーに聞く:
- **どのサンプルか**: `Sample2D` / `Sample3D`
- **名前**: 英語のPascalCase(例: `Item`, `HealthBar`)。工場関数は `Create<名前>`、Componentは `<名前>Controller`
- **何をするか**: 見た目(スプライト/メッシュ)、動き、当たり判定、他のオブジェクトとのやり取り

見た目の無い「管理役」(スコア係・出現係など)の場合は、Componentを `<役割>Manager`、工場関数のファイルを別名(例: `EnemyManager` ↔ `Objects/EnemySpawner`)にする。Componentと同じ名前のファイルを `Objects/` に作らない。

## 2. お手本を読む

書き始める前に、同じサンプルの既存ファイルを読んで書き方をそろえる:
- 見た目と動きがあるもの: `Game/Objects/Player.cpp`、`Game/Components/PlayerController.h/.cpp`
- 管理役: `Game/Objects/EnemySpawner.cpp`、`Game/Components/EnemyManager.h/.cpp`
- 使い回す(ObjectPool)もの: `Sample2D/Game/Components/BulletManager.cpp`
- ルール全般: リポジトリ直下の `CLAUDE.md`

## 3. ファイルを作る(4つ)

パスは `3D_FreamWork/<サンプル>/Game/` から。Engineのヘッダーは `../../../Engine/○○.h`。

**`Objects/<名前>.h`**
```cpp
#pragma once

class Scene;
class GameObject;

// <何を作るかを1行で>
GameObject* Create<名前>(Scene& scene);
```
他のオブジェクトの情報が必要なら、引数でポインタを受け取る(`Create<名前>(Scene& scene, GameObject* target)` など)。

**`Objects/<名前>.cpp`**
```cpp
#include "<名前>.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../Components/<名前>Controller.h"

GameObject* Create<名前>(Scene& scene) {
    GameObject* obj = scene.CreateObject("<名前>");
    obj->transform.position = { 0.0f, 0.0f, 0.0f };
    obj->transform.scale    = { 100.0f, 100.0f, 100.0f };

    // 動き
    obj->AddComponent<<名前>Controller>();

    // 見た目
    // 当たり判定+物理演算(PhysX)

    return obj;
}
```

**`Components/<名前>Controller.h`**
```cpp
#pragma once
#include "../../../Engine/Component.h"

// <このComponentの役割を1行で>
class <名前>Controller : public Component {
private:
    float speed = 100.0f;   // 移動速度   ← メンバには行末コメント

public:
    void Update(float dt) override;
};
```

**`Components/<名前>Controller.cpp`** — 処理のまとまりごとに1行コメント(`// 移動処理: ...`)。

### 中身を書く時のルール
- Dynamicな剛体は `transform.position` を直接書き換えず `RigidbodyComponent::SetVelocity()` で動かす
- 2Dで奥行き方向に動かしたくない時は `SetUseGravity(false)` / `SetFreezeRotation(true)` / `SetFreezePositionZ(true)`
- 他の管理役に知らせる時は `EventBus::Get().Publish("イベント名")`。`this` をキャプチャして `Subscribe` したら、そのSceneの `Uninit()` で `EventBus::Get().Clear()` されているか確認する
- 2Dでマウス位置と比べる時は `Input::GetMouseWorldPosition()`
- 当たり判定の相手はタグで見分ける(`obj->SetTag("Item")` / `info.other->GetTag() == "Player"`)

## 4. プロジェクトに登録する

`<サンプル>/<サンプル>.vcxproj` と `<サンプル>.vcxproj.filters` の **両方** に4ファイルを追加する。既存のエントリの並びに合わせる:
```xml
<!-- .vcxproj -->
<ClCompile Include="Game\Objects\<名前>.cpp" />
<ClInclude Include="Game\Objects\<名前>.h" />

<!-- .vcxproj.filters -->
<ClCompile Include="Game\Objects\<名前>.cpp">
  <Filter>Game\Objects</Filter>
</ClCompile>
```
Componentsのファイルは `Game\Components`。

## 5. シーンに置く

使うシーンの `Init()` に `Create<名前>(*this);` を1行足し、`#include "../Objects/<名前>.h"` を追加する。**シーンにはロジックを書かない**(配置だけ)。

## 6. 確認と仕上げ

1. `build` スキルの手順でファイルチェック(`-FixBom`)とDebug/Releaseビルドを行う
2. `README.md` の「サンプル構成」節のObjects/Components一覧に追加する
3. 作ったファイルの一覧と、どのシーンに置いたかを報告する。動き(操作して確かめること)の確認はユーザーにお願いする
