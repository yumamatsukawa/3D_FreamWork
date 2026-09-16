#pragma once
#include "Component.h"
#include "GameObject.h"
#include "Collider.h"

// 四角形の当たり判定を持たせるComponent。
// 形状の計算自体はCollider.hのBoxCollider2Dがそのまま担う。
class BoxColliderComponent : public Component {
private:
    BoxCollider2D collider;

public:
    // 使い方: obj->AddComponent<BoxColliderComponent>(isTrigger, size, offset, isStatic);
    // isStatic = true にすると、床や壁のように押し戻されない(動かない)オブジェクトになる
    BoxColliderComponent(bool isTrigger,
        DirectX::XMFLOAT3 size = { 0.f, 0.f, 0.f },
        DirectX::XMFLOAT3 offset = { 0.f, 0.f, 0.f },
        bool isStatic = false)
        : collider(isTrigger, size, offset) {
        collider.isStatic = isStatic;
    }

    void Init() override {
        collider.owner = GetOwner();
        collider.enabled = true;
        RegisterCollider(&collider);
    }
    void Uninit() override {
        UnregisterCollider(&collider);
    }

    BoxCollider2D* GetCollider() { return &collider; }
};

// 円形の当たり判定を持たせるComponent。
class CircleColliderComponent : public Component {
private:
    CircleCollider2D collider;

public:
    // 使い方: obj->AddComponent<CircleColliderComponent>(isTrigger, radius, offset, isStatic);
    CircleColliderComponent(bool isTrigger,
        float radius = 0.f,
        DirectX::XMFLOAT3 offset = { 0.f, 0.f, 0.f },
        bool isStatic = false)
        : collider(isTrigger, radius, offset) {
        collider.isStatic = isStatic;
    }

    void Init() override {
        collider.owner = GetOwner();
        collider.enabled = true;
        RegisterCollider(&collider);
    }
    void Uninit() override {
        UnregisterCollider(&collider);
    }

    CircleCollider2D* GetCollider() { return &collider; }
};
