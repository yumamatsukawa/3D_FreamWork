#include "Lod.h"
#include "../Render/Image.h"
#include <algorithm>
#include <cmath>

namespace Lod {
    namespace {
        bool enabled = true;      // falseなら、すべての設定を無視する
        float globalScale = 1.f;  // すべての距離にかける倍率
    }

    float GetCameraDistance(XMFLOAT3 position) {
        const XMFLOAT3& eye = Image::GetCamera3D().position;
        float dx = position.x - eye.x, dy = position.y - eye.y, dz = position.z - eye.z;
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }

    bool IsBeyond(XMFLOAT3 center, float radius, float limit) {
        if (!enabled || limit <= 0.f) return false;   // 使わない時は何も計算しない
        float scaledLimit = limit * globalScale;
        float surface = GetCameraDistance(center) - (radius > 0.f ? radius : 0.f);
        return surface > scaledLimit;
    }

    bool IsBeyond(const XMMATRIX& world, float localRadius, float limit) {
        if (!enabled || limit <= 0.f) return false;   // 使わない時は行列も読まない
        XMFLOAT4X4 m;
        XMStoreFloat4x4(&m, world);

        // 行(x軸・y軸・z軸)の長さが、それぞれの軸の拡大率(Visibility::IsTransformedSphereInView3D と同じ)
        auto rowLength = [&m](int row) {
            return sqrtf(m.m[row][0] * m.m[row][0] + m.m[row][1] * m.m[row][1] + m.m[row][2] * m.m[row][2]);
        };
        float scale = (std::max)({ rowLength(0), rowLength(1), rowLength(2) });
        return IsBeyond({ m._41, m._42, m._43 }, localRadius * scale, limit);
    }

    void SetEnabled(bool value) {
        enabled = value;
    }

    bool IsEnabled() {
        return enabled;
    }

    void SetGlobalScale(float scale) {
        globalScale = (scale > 0.f) ? scale : 1.f;
    }

    float GetGlobalScale() {
        return globalScale;
    }
}
