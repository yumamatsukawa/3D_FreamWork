#pragma once
#include "Collider.h"

class GameObject;

// すべてのComponentの基底クラス。
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
    // GameObject::SetActive()が呼ばれた時に通知される(ObjectPoolでの使い回し検知などに使う)
    virtual void OnActiveChanged(bool active) {}

    // ─── 当たり判定のコールバック ───────────────
    // 付いているGameObjectが誰かと当たった時、その全Componentに通知される。
    // 反応したいComponent(例: PlayerController)だけがoverrideすればよい。

    // すり抜ける当たり判定
    virtual void OnTriggerEnter2D(Collider2D* other) {}    // あたった瞬間
    virtual void OnTriggerStay2D(Collider2D* other) {}     // あたっている間
    virtual void OnTriggerExit2D(Collider2D* other) {}     // 離れた瞬間

    // すり抜けない当たり判定
    virtual void OnCollisionEnter2D(CollisionInfo info) {} // あたった瞬間
    virtual void OnCollisionStay2D(CollisionInfo info) {}  // あたっている間
    virtual void OnCollisionExit2D(CollisionInfo info) {}  // 離れた瞬間

    void SetOwner(GameObject* go) { owner = go; }  // GameObjectを設定
    GameObject* GetOwner() const { return owner; } // GameObjectを返す

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool v) { enabled = v; }
};
