#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>
#include <string>
#include <vector>

// Mathデモの操作役。MathUtil(Lerp・Clamp・角度・内積・外積・乱数など)の結果を、点と線の印で見せる。
// 1〜4キーで見せる関数を選ぶ。2D版はマウス、3D版は矢印キーなどで値を動かす
class MathDemoController : public Component {
private:
    static constexpr int binCount = 10;          // 2D: 乱数の分布を数える箱の数
    static constexpr size_t maxScatter = 220;    // 2D: 円周上の乱数の点を残しておく数

    DemoMode mode;                               // 2D版か3D版か
    int selected = 0;                            // 今見せている関数(0〜3)
    float elapsed = 0.f;                         // 動き始めてからの時間(アニメーション用)

    // 2D版の状態
    float armAngle = 0.f;                        // 角度の腕が今向いている角度(MoveTowardsAngleで目標に近づく)
    int histogram[binCount] = {};                // RandomRange(0, 1) が各箱に入った回数
    std::vector<DirectX::XMFLOAT2> scatter;      // RandomOnCircle の点(古いものから消す)
    int diceResult = 0;                          // RandomRange(1, 6) の最後の結果

    // 3D版の状態
    DirectX::XMFLOAT3 follower{ -200.f, 40.f, 300.f };    // MoveTowards で目標に近づく点
    DirectX::XMFLOAT3 userVector{ 120.f, 80.f, 60.f };    // Normalize を見るための、キーで動かすベクトル

    void Update2D(float dt);
    void Update3D(float dt);
    void Draw2D() const;
    void Draw3D() const;
    void DrawMenu() const;                       // 1〜4の関数の名前の並び(今のものを色で示す)

public:
    explicit MathDemoController(DemoMode demoMode) : mode(demoMode) {}

    void Update(float dt) override;
    void Draw() override;
};
