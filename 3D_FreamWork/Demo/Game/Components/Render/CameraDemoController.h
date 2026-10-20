#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"

class CameraFollowComponent;
class MeshRenderer;

// Cameraデモの操作役。動かす物(自分)に付けて、WASD・矢印キーで動かし、SPACEでCameraShakeで揺らす。
// 2D版: 自分を CameraFollowComponent で2Dカメラが追う(1: なめらかさ / 2: デッドゾーン / 3: 範囲 / 4: 対象からのずれ)。
// 3D版: 自分を三人称・一人称のカメラで見る(V: 視点の切り替え / 右ドラッグ: 向き / Q・E: ズーム / 1〜3: 揺れの強さ)。
// 2Dカメラ・3Dカメラの位置と揺れを変えるので、出る時(Uninit)に必ず戻す
class CameraDemoController : public Component {
private:
    DemoMode mode;                                  // 2D版か3D版か

    // 2D版の状態
    CameraFollowComponent* follow = nullptr;        // 自分を追うカメラの部品(作る側から渡される)
    int smoothIndex = 1;                            // なめらかさ(smoothTimes の何番目か)
    bool deadZoneOn = false;                        // デッドゾーンを使っているか
    bool boundsOn = true;                           // 範囲を使っているか
    bool offsetOn = false;                          // 対象からずらして映しているか

    // 3D版の状態
    MeshRenderer* body = nullptr;                   // 自分の見た目(一人称の間は隠す)
    float yaw = 0.f;                                // 水平方向の向き(ラジアン。0で+Z方向)
    float pitch = -0.3f;                            // 上下の向き(ラジアン。マイナスで見下ろす)
    float distance = 450.f;                         // 三人称: 自分からカメラまでの距離
    bool firstPerson = false;                       // 一人称か
    int shakeIndex = 1;                             // 揺れの強さ(shakeStrengths3D の何番目か)

    void Update2D(float dt);
    void Update3D(float dt);
    void Draw2D();
    void Draw3D();

public:
    explicit CameraDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 2D版: 追うカメラの部品を渡す(作る側が、このComponentの後に付けて、ここへ渡す)
    void SetFollow(CameraFollowComponent* followComponent);
    // 3D版: 自分の見た目を渡す
    void SetBody(MeshRenderer* meshRenderer) { body = meshRenderer; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
