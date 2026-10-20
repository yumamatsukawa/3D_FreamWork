#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class GameObject;

// Transformデモの操作役。親を動かす・回す・大きさを変える・親子をつなぐ/切るを受け付けて、親と子の位置・回転・大きさを文字で出す
class TransformDemoController : public Component {
private:
    static constexpr float moveSpeed = 250.f;      // 親の移動の速さ(1秒あたり)
    static constexpr float turnSpeed = 90.f;       // 親が回る速さ(度 / 秒)
    static constexpr float scaleSpeed = 0.6f;      // 親の大きさが変わる速さ(1秒あたりの割合)
    static constexpr float moveLimit = 400.f;      // 親が動ける範囲(中心から片側)

    DemoMode mode;                 // 2D版か3D版か
    GameObject* parent;            // 親(大きい物)
    GameObject* child;             // 子(小さい物。親と一緒に動く)
    DirectX::XMFLOAT3 childLocalPosition;   // つないだ時の子の位置(親から見た位置。親の大きさ1あたり)
    DirectX::XMFLOAT3 childLocalScale;      // つないだ時の子の大きさ(親から見た倍率)
    float parentSize;              // 親の大きさ(一辺。1・2キーで変わる)
    bool attached = true;          // 子が親につながっているか

    DirectX::XMFLOAT3 GetChildWorldPosition() const;   // 子の画面上の(ワールドの)位置
    void SetAttached(bool value);                       // 親子をつなぐ / 切る(切る時は、今いる場所に残す)

public:
    TransformDemoController(DemoMode demoMode, GameObject* parentObject, GameObject* childObject, float size,
        DirectX::XMFLOAT3 localPosition, DirectX::XMFLOAT3 localScale)
        : mode(demoMode), parent(parentObject), child(childObject), childLocalPosition(localPosition),
        childLocalScale(localScale), parentSize(size) {}

    void Update(float dt) override;
    void Draw() override;
};
