#pragma once
#include <DirectXMath.h>
#include <string>
#include <memory>
#include "Transform.h"
#include "Collider.h"

class GameObject {
protected:
    std::unique_ptr<Collider2D> collider;
    Transform   transform;
    std::string tag;
    bool        isActive = true;

public:
    GameObject() = default;
    GameObject(std::string Tag) : tag{ Tag } {}
    virtual ~GameObject() {
        if (collider) UnregisterCollider(collider.get());
    }

    virtual void Init() {}
    virtual void Update(float dt) {}
    virtual void Draw() {}
    virtual void Uninit() {
        if (collider) UnregisterCollider(collider.get());
    }

    // すり抜ける当たり判定
    virtual void OnTriggerEnter2D(Collider2D* other) {}
    virtual void OnTriggerStay2D(Collider2D* other) {}
    virtual void OnTriggerExit2D(Collider2D* other) {}

    // すり抜けない当たり判定
    virtual void OnCollisionEnter2D(CollisionInfo info) {}
    virtual void OnCollisionStay2D(CollisionInfo info) {}
    virtual void OnCollisionExit2D(CollisionInfo info) {}

    const std::string& GetTag() const { return tag; }
    Transform& GetTransform()  { return transform; }
    const bool& GetIsActive() const { return isActive; }


    /* =======================================================
    * コライダーの追加:
    * <>に何のコライダーにするのかを入れる
    * ()に入れるもの：
    * ->BoxCollider2D...ならisTriggerと大きさと座標
    * ->CircleCollider2DならisTriggerと半径と座標
    ======================================================== */
    template<typename T = Collider2D, typename... Args>
    T* AddCollider(Args&&... args) {
        auto col = std::make_unique<T>(std::forward<Args>(args)...);
        col->owner = this;
        col->enabled = true;
        T* ptr = col.get();
        RegisterCollider(ptr);
        collider = std::move(col);
        return ptr;
    }
};