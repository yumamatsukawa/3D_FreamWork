#pragma once
#include "Collider.h"

class GameObject;

// すべてのComponent(振る舞い)の基底クラス。
// UnityでいうMonoBehaviourに近い役割で、GameObjectに後から付け外しして使う。
class Component {
private:
    GameObject* owner = nullptr;
    bool enabled = true;

public:
    virtual ~Component() {};

    // GameObject::AddComponentで追加された直後に1回だけ呼ばれる
    virtual void Init() {}
    // 毎フレーム呼ばれる(ロジック更新)
    virtual void Update(float dt) {}
    // 毎フレーム呼ばれる(描画)
    virtual void Draw() {}
    // GameObjectが破棄される時に呼ばれる
    virtual void Uninit() {}

    // ─── 当たり判定のコールバック ───────────────
    // 付いているGameObjectが誰かと当たった時、その全Componentに通知される。
    // 反応したいComponent(例: PlayerController)だけがoverrideすればよい。

    // すり抜ける当たり判定
    virtual void OnTriggerEnter2D(Collider2D* other) {}
    virtual void OnTriggerStay2D(Collider2D* other) {}
    virtual void OnTriggerExit2D(Collider2D* other) {}

    // すり抜けない当たり判定
    virtual void OnCollisionEnter2D(CollisionInfo info) {}
    virtual void OnCollisionStay2D(CollisionInfo info) {}
    virtual void OnCollisionExit2D(CollisionInfo info) {}

    void SetOwner(GameObject* go) { owner = go; }
    GameObject* GetOwner() const { return owner; }

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool v) { enabled = v; }
};
