#include "Visibility.h"
#include "Image.h"
#include "Graphics.h"
#include <DirectXCollision.h>
#include <algorithm>
#include <cmath>

namespace Visibility {
    namespace {
        BoundingFrustum frustum;       // ワールド座標の視錐台(BeginFrameで作る)
        bool frustumReady = false;     // BeginFrameで作った後か
        int culledCount = 0;           // このフレームに省いた数(数えている途中)
        int lastCulledCount = 0;       // 前のフレームに省いた数(GetCulledCountが返す)

    }

    void BeginFrame() {
        // 前のフレームの数を確定させて、数え直す
        lastCulledCount = culledCount;
        culledCount = 0;

        // 3Dカメラの視錐台: 投影行列から作ると「カメラの原点・向き」の空間なので、ビュー行列の逆行列でワールド座標に直す
        const Camera& camera = Image::GetCamera3D();
        BoundingFrustum viewSpace(camera.GetProjectionMatrix(Image::GetAspectRatio()));   // 左手系(既定)でエンジンと合う
        XMMATRIX inverseView = XMMatrixInverse(nullptr, camera.GetViewMatrix());
        viewSpace.Transform(frustum, inverseView);
        frustumReady = true;
    }

    void CountCulled() {
        culledCount++;
    }

    int GetCulledCount() {
        return lastCulledCount;
    }

    bool IsSphereInView3D(XMFLOAT3 center, float radius) {
        if (!frustumReady) return true;   // まだ判定できない間は、映っているものとして扱う(消さない)
        BoundingSphere sphere(center, radius > 0.f ? radius : 0.f);
        return frustum.Intersects(sphere);
    }

    bool IsTransformedSphereInView3D(const XMMATRIX& world, float localRadius) {
        XMFLOAT4X4 m;
        XMStoreFloat4x4(&m, world);

        // 行(x軸・y軸・z軸)の長さが、それぞれの軸の拡大率
        auto rowLength = [&m](int row) {
            return sqrtf(m.m[row][0] * m.m[row][0] + m.m[row][1] * m.m[row][1] + m.m[row][2] * m.m[row][2]);
        };
        float scale = (std::max)({ rowLength(0), rowLength(1), rowLength(2) });
        return IsSphereInView3D({ m._41, m._42, m._43 }, localRadius * scale);
    }

    bool IsBoxInView3D(XMFLOAT3 center, XMFLOAT3 halfSize) {
        if (!frustumReady) return true;
        BoundingBox box(center, halfSize);
        return frustum.Intersects(box);
    }

    bool IsCircleInView2D(XMFLOAT3 position, float radius) {
        const Camera& camera = Image::GetCamera();

        // 画面座標(画面中央が(0,0))と、画面での半径(zは大きさに影響しない)
        float screenX = position.x - camera.position.x;
        float screenY = position.y - camera.position.y;
        float screenRadius = (radius > 0.f ? radius : 0.f);

        // 画面の四角(±半分の大きさ)から、円の中心までの距離で比べる(角の外側も正しく判定する)
        float outsideX = fabsf(screenX) - Graphics::width * 0.5f;
        float outsideY = fabsf(screenY) - Graphics::height * 0.5f;
        if (outsideX < 0.f) outsideX = 0.f;
        if (outsideY < 0.f) outsideY = 0.f;
        return outsideX * outsideX + outsideY * outsideY <= screenRadius * screenRadius;
    }

    bool IsRectInView2D(XMFLOAT3 position, XMFLOAT2 halfSize) {
        const Camera& camera = Image::GetCamera();

        float screenX = position.x - camera.position.x;
        float screenY = position.y - camera.position.y;
        float screenHalfW = fabsf(halfSize.x);
        float screenHalfH = fabsf(halfSize.y);

        // 四角同士の重なり(中心の距離が、半分の大きさの合計以下なら重なっている)
        return fabsf(screenX) <= Graphics::width * 0.5f + screenHalfW
            && fabsf(screenY) <= Graphics::height * 0.5f + screenHalfH;
    }
}
