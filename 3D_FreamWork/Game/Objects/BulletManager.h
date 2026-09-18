#pragma once
#include <DirectXMath.h>
#include "../../Engine/ObjectPool.h"

class Scene;
class GameObject;

// 弾の生成・使い回し(プーリング)を担当するクラス。
// 「誰が撃ったか(shooter)」を受け取れるようにしてあるので、
// 今後「自分が撃った弾には自分は当たらない」「スコアを誰に入れるか」等の拡張がしやすい。
//
// 【寿命についての注意】
// 内部のObjectPoolは、Sceneが管理しているGameObjectへの生ポインタを持つ。
// そのため、BulletManagerは必ず「弾を撃つシーン自身(例: GameScene)」がメンバとして
// 持つこと。シーンが切り替わって破棄される時にBulletManagerも一緒に破棄されるので、
// ポインタが宙に浮く(ダングリングポインタになる)心配が無くなる。
// ※ グローバル変数やstatic変数として、シーンをまたいで生かし続けてはいけない。
class BulletManager {
public:
    // shooter(撃った本人)から、指定位置・方向に弾を1発発射する
    GameObject* Fire(Scene& scene, XMFLOAT3 position, XMFLOAT2 direction);

private:
    ObjectPool pool;
};
