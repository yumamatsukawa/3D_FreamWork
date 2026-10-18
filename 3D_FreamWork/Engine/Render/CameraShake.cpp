#include "CameraShake.h"
#include "Image.h"
#include "../Core/MathUtil.h"
#include "../Animation/Ease.h"

namespace CameraShake {
    namespace {
        float shakeStrength = 0.f;  // 揺れ始めの強さ
        float shakeDuration = 0.f;  // 揺れる時間(秒)。0なら揺れていない
        float shakeElapsed = 0.f;   // 揺れ始めてからの時間(秒)

        bool applied = false;                  // BeginDrawでカメラをずらしたか(EndDrawで戻すため)
        XMFLOAT3 saved2DPosition{};            // ずらす前の2Dカメラの位置
        XMFLOAT3 saved3DPosition{};            // ずらす前の3Dカメラの位置
        XMFLOAT3 saved3DTarget{};              // ずらす前の3Dカメラの注視点

        // 今の揺れの強さ(時間とともに弱まり、終わったら0)
        float GetCurrentStrength() {
            if (shakeDuration <= 0.f || shakeElapsed >= shakeDuration) return 0.f;
            return shakeStrength * (1.f - Ease::OutQuad(shakeElapsed / shakeDuration));
        }

        // 3Dカメラから見た右と上の向き(XMMatrixLookAtLHと同じ求め方)。向きが決まらない時はワールドのX・Y
        void GetCameraRightUp(const Camera& camera, XMFLOAT3& right, XMFLOAT3& up) {
            XMFLOAT3 forward = Math::Normalize(camera.target - camera.position);
            right = Math::Normalize(Math::Cross(camera.up, forward));
            if (Math::Length(forward) < 0.5f || Math::Length(right) < 0.5f) {
                right = { 1.f, 0.f, 0.f };
                up = { 0.f, 1.f, 0.f };
                return;
            }
            up = Math::Cross(forward, right);
        }
    }

    void Shake(float strength, float duration) {
        if (strength <= 0.f || duration <= 0.f) return;

        // 揺れている途中なら、今の揺れ(弱まった後)より強いか同じ時だけ置き換える
        if (strength < GetCurrentStrength()) return;
        shakeStrength = strength;
        shakeDuration = duration;
        shakeElapsed = 0.f;
    }

    bool IsShaking() {
        return GetCurrentStrength() > 0.f;
    }

    void Stop() {
        // 揺れの強さ・時間だけを消す。applied と覚えた値は触らない
        // (BeginDraw〜EndDrawの間に呼ばれても、EndDrawでずらす前の値に戻せるように)
        shakeStrength = 0.f;
        shakeDuration = 0.f;
        shakeElapsed = 0.f;
    }

    void Update(float unscaledDt) {
        if (shakeDuration <= 0.f) return;

        // 揺れを進める。終わったら揺れていない状態に戻す
        shakeElapsed += unscaledDt;
        if (shakeElapsed >= shakeDuration) {
            shakeStrength = 0.f;
            shakeDuration = 0.f;
            shakeElapsed = 0.f;
        }
    }

    void BeginDraw() {
        float strength = GetCurrentStrength();
        if (strength <= 0.f) return;

        // ずらす量: 毎フレームばらばらな向きに、最大strengthだけ(画面の右・上の向き)
        float offsetX = Math::RandomRange(-strength, strength);
        float offsetY = Math::RandomRange(-strength, strength);

        // 戻すために、ずらす前の値を覚えておく
        Camera& camera2D = Image::GetCamera();
        Camera& camera3D = Image::GetCamera3D();
        saved2DPosition = camera2D.position;
        saved3DPosition = camera3D.position;
        saved3DTarget = camera3D.target;
        applied = true;

        // 2Dカメラ: x,yをずらす
        camera2D.position.x += offsetX;
        camera2D.position.y += offsetY;

        // 3Dカメラ: カメラから見た右・上へ、positionとtargetを同じだけずらす(向きは変えない)
        XMFLOAT3 right, up;
        GetCameraRightUp(camera3D, right, up);
        XMFLOAT3 offset3D = right * offsetX + up * offsetY;
        camera3D.position = camera3D.position + offset3D;
        camera3D.target = camera3D.target + offset3D;
    }

    void EndDraw() {
        if (!applied) return;

        // ずらす前の値に戻す(足した分を引くのではなく、覚えた値をそのまま戻すので誤差が残らない)
        Image::GetCamera().position = saved2DPosition;
        Image::GetCamera3D().position = saved3DPosition;
        Image::GetCamera3D().target = saved3DTarget;
        applied = false;
    }
}
