#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class DemoCameraController;

// 地形デモで歩く箱の操作を担当する(カメラの向きを基準に、WASDで水平に動く。PhysXの剛体に速度を渡すので、坂道は重力と当たり判定で登る)。
// 地形の外へ落ちた時や、Rキーで、スタート地点へ戻る
class DemoWalkerController : public Component {
private:
    DemoCameraController* camera = nullptr;      // 向き・移動の基準にするカメラ
    DirectX::XMFLOAT3 spawnPosition{ 0.f, 0.f, 0.f };   // スタート地点

    static constexpr float speed = 450.f;        // 移動速度
    static constexpr float fallLimit = 1500.f;   // スタート地点よりこれだけ下へ落ちたら、スタートへ戻す

    void Respawn();   // スタート地点へ戻す(速度も0にする)

public:
    // 向き・移動の基準にするカメラと、スタート地点を、CreateDemoWalker()から渡してもらう
    void Setup(DemoCameraController* cameraController, DirectX::XMFLOAT3 spawn) {
        camera = cameraController;
        spawnPosition = spawn;
    }

    void Update(float dt) override;
};
