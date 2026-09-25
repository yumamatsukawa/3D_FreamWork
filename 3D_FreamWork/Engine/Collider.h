#pragma once
#include <DirectXMath.h>
#include <string>
#include "Transform.h"

class GameObject;

// 当たり判定は今はPhysX(RigidbodyComponent)が計算している。
// このクラスは、Component::OnTriggerEnter2Dなどのコールバックで「誰と当たったか」を
// 伝えるための軽いハンドルとしてだけ存在する(実際の形状・判定計算は持たない)
class Collider2D {
public:
    GameObject* owner = nullptr;
    bool isTrigger = false;
    bool isStatic = false;  // true: 押し戻しで動かない(Enemyなど)

    const std::string& GetTag() const;
};

/* ========================================================
* ----押し戻し情報----
* other : Collider2D* : 相手のコライダー
* normal: XMFLOAT2     : 押し戻し方向
* depth : float        : めり込み量
=========================================================*/
struct CollisionInfo {
    Collider2D* other = nullptr;
    DirectX::XMFLOAT2 normal = { 0.f, 0.f };
    float             depth = 0.f;
};

// 点(スクリーン座標やマウス座標など)がTransformの四角形の中に入っているか判定する。
// 当たり判定システムとは独立した、UIのクリック判定などに使う汎用ユーティリティ
bool IsPointInBox(DirectX::XMFLOAT2 point, const Transform& transform);
