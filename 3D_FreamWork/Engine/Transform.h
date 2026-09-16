#pragma once
#include <DirectXMath.h>
#include <vector>
#include <algorithm>

using namespace DirectX;

// 位置・大きさ・回転に加えて、親子階層を持つTransform。
// 子は親のワールド行列を掛け合わせることで「親と一緒に動く」ようになる。
class Transform {
public:
    XMFLOAT3 position{};              // 位置(親がいれば親からの相対座標)
    XMFLOAT3 scale{ 1.f, 1.f, 1.f };  // 大きさ(初期値は等倍)
    XMFLOAT3 rotate{};                // 回転(度数、XYZそれぞれの軸)

    // ─── 親子関係 ─────────────────────────────
    // 親を設定する。newParentにnullptrを渡すと親を外す。
    void SetParent(Transform* newParent) {
        if (parent == newParent) return;

        // 今の親の子リストから自分を外す
        if (parent) {
            auto& siblings = parent->children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
        }

        parent = newParent;

        if (parent) {
            parent->children.push_back(this);
        }
    }

    Transform* GetParent() const { return parent; }
    const std::vector<Transform*>& GetChildren() const { return children; }

    // ─── 行列計算 ─────────────────────────────
    // 自分自身だけのローカル行列(親は考慮しない)
    XMMATRIX GetLocalMatrix() const {
        XMMATRIX s = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX r = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(rotate.x),
            XMConvertToRadians(rotate.y),
            XMConvertToRadians(rotate.z));
        XMMATRIX t = XMMatrixTranslation(position.x, position.y, position.z);
        return s * r * t;
    }

    // 親をたどって、最終的なワールド空間での行列を求める
    XMMATRIX GetWorldMatrix() const {
        XMMATRIX local = GetLocalMatrix();
        if (parent) {
            return local * parent->GetWorldMatrix();
        }
        return local;
    }

private:
    Transform* parent = nullptr;
    std::vector<Transform*> children;
};
