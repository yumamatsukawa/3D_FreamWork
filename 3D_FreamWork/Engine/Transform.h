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

    // 親をたどって、position/scale/rotateを「ワールド空間の値」に平らにしたTransformを返す。
    // SpriteRendererなど、2D描画側(Sprite::BuildWorldMatrix)はこれをそのまま使えばよい。
    // ※ rotateはZ軸回転のみを合成する(このエンジンの2D描画がZ軸回転しか使わないため)
    Transform GetWorldTransform() const {
        if (!parent) return *this;

        Transform pw = parent->GetWorldTransform();  // 親のワールド変換(再帰)

        // Spriteの見た目の回転方向(画面上でCCW)に合わせて符号を反転する
        // (Collider側のGetCorners/GetCenterと同じ理由・同じ符号)
        float rad = -pw.rotate.z * (XM_PI / 180.f);
        float cosA = cosf(rad), sinA = sinf(rad);

        // 自分のローカル位置を、親のスケール→親の回転の順で変換してから、親のワールド位置に加算
        float lx = position.x * pw.scale.x;
        float ly = position.y * pw.scale.y;

        Transform world;
        world.position = {
            pw.position.x + (lx * cosA - ly * sinA),
            pw.position.y + (lx * sinA + ly * cosA),
            pw.position.z + position.z * pw.scale.z
        };
        world.scale = { scale.x * pw.scale.x, scale.y * pw.scale.y, scale.z * pw.scale.z };
        world.rotate = { rotate.x, rotate.y, rotate.z + pw.rotate.z };
        return world;
    }

    // ワールド空間での移動量(worldDelta)を、親の回転・拡縮を考慮して
    // このTransformのposition(ローカル)にそのまま加算できる量に変換する。
    // 例: 当たり判定の押し戻し量(ワールド空間で計算される)をpositionに反映する時に使う
    XMFLOAT2 WorldDeltaToLocal(XMFLOAT2 worldDelta) const {
        if (!parent) return worldDelta;  // 親が無ければローカル=ワールドなのでそのまま

        Transform pw = parent->GetWorldTransform();

        // GetWorldTransformで掛けた「-pw.rotate.z」の回転を打ち消す(逆回転)
        float rad = pw.rotate.z * (XM_PI / 180.f);
        float cosA = cosf(rad), sinA = sinf(rad);
        float rx = worldDelta.x * cosA - worldDelta.y * sinA;
        float ry = worldDelta.x * sinA + worldDelta.y * cosA;

        // 親のワールドスケールで割って、ローカルの大きさに戻す(0除算だけ回避)
        float sx = (pw.scale.x != 0.f) ? pw.scale.x : 1.f;
        float sy = (pw.scale.y != 0.f) ? pw.scale.y : 1.f;
        return { rx / sx, ry / sy };
    }

private:
    Transform* parent = nullptr;
    std::vector<Transform*> children;
};
