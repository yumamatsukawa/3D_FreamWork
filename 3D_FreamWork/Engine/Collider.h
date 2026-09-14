#pragma once
#include <DirectXMath.h>
#include <string>
#include "Transform.h"

class GameObject;

/* =======================================================
* ----何のクラスか----
*	基底コライダークラス
* BoxCollider, CircleColliderの親クラスにあたる
======================================================== */
class Collider2D {
public:
    GameObject* owner = nullptr;
    bool enabled      = true;
    bool isTrigger    = false;  // ★ true=Trigger, false=Collision
    DirectX::XMFLOAT2 offset = { 0.f, 0.f };  // transformからのオフセット

    const std::string& GetTag() const;

    enum class Shape { Box, Circle };
    virtual Shape GetShape() const = 0;
};

/* =======================================================
* ----何のクラスか----
*	ボックスコライダークラス
* OBBの当たり判定を持っている
* AddColliderのときに ”すり抜けるか”と”大きさ”と”座標”を引数に入れる
======================================================== */
class BoxCollider2D : public Collider2D {
public:
    BoxCollider2D(bool IsTrigger, DirectX::XMFLOAT3 size = { 0.f, 0.f, 0.f }, DirectX::XMFLOAT3 pos = { 0.f, 0.f, 0.f })
    {
        offset = { pos.x, pos.y };
        this->size = { size.x, size.y };
        isTrigger = IsTrigger;
    }

    DirectX::XMFLOAT2 size = { 0.f, 0.f };
    Shape GetShape() const override { return Shape::Box; }
    void  GetCorners(DirectX::XMFLOAT2 outCorners[4]) const;
};

/* =======================================================
* ----何のクラスか----
*	サークルコライダークラス
* AddColliderのときに ”すり抜けるか”と”半径”と”座標”を引数に入れる
======================================================== */
class CircleCollider2D : public Collider2D {
public:
    CircleCollider2D(bool IsTrigger, float radius = 0.f, DirectX::XMFLOAT3 pos = { 0.f, 0.f, 0.f })
    {
        offset = { pos.x, pos.y };
        this->radius = radius;
        isTrigger = IsTrigger;
    }

    float  radius = 0.f;
    Shape  GetShape() const override { return Shape::Circle; }
    DirectX::XMFLOAT2 GetCenter() const;
    float             GetRadius() const;
};

/* ========================================================
* ----押し戻し情報----
* other : Collision2D* : 規定コライダー
* nomal : XMFLOAT2     : 押し戻し方向
* depth : float        : めり込み量
=========================================================*/
struct CollisionInfo {
    Collider2D* other = nullptr;
    DirectX::XMFLOAT2 normal = { 0.f, 0.f };  // 押し戻し方向
    float             depth = 0.f;            // めり込み量
};

void RegisterCollider(Collider2D* col);
void UnregisterCollider(Collider2D* col);
void UpdateCollider();
void DrawColliders();
void InitCollider();
void UninitCollider();
bool IsPointInBox(DirectX::XMFLOAT2 point, const Transform& transform);