#pragma once
#include <DirectXMath.h>
#include "../../../Engine/ObjectPool.h"

class Scene;
class GameObject;

// 敵の生成・使い回し(プーリング)を担当するクラス。BulletManagerと同じ考え方。
//
// 【寿命についての注意】BulletManagerと同じく、必ず「敵を出すシーン自身(例: GameScene)」が
// メンバとして持つこと。シーンが切り替わって破棄される時に一緒に破棄されるので、
// ポインタが宙に浮く(ダングリングポインタになる)心配が無くなる。
class EnemyManager {
public:
    // position(ワールド座標)に1体出現させ、targetを追いかけさせる
    GameObject* Spawn(Scene& scene, XMFLOAT3 position, GameObject* target);

private:
    ObjectPool pool;
};
