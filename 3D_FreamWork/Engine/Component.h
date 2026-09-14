#pragma once

class GameObject;

// すべてのComponent(振る舞い)の基底クラス。
// UnityでいうMonoBehaviourに近い役割で、GameObjectに後から付け外しして使う。
class Component {
private:
    GameObject* owner = nullptr;
    bool enabled = true;

public:
    virtual ~Component() = default;

    // GameObject::AddComponentで追加された直後に1回だけ呼ばれる
    virtual void Init() {}
    // 毎フレーム呼ばれる(ロジック更新)
    virtual void Update(float dt) {}
    // 毎フレーム呼ばれる(描画)
    virtual void Draw() {}
    // GameObjectが破棄される時に呼ばれる
    virtual void Uninit() {}

    void SetOwner(GameObject* go) { owner = go; }
    GameObject* GetOwner() const { return owner; }

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool v) { enabled = v; }
};
