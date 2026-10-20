#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class ModelRenderer;

// Modelデモの操作役。同じFBXを2体置いたうち、左の1体を動かす(右は動かさない見本)。
// パーツ(Top)の回転・上下・大きさと、モデル全体の回転・大きさを切り替え、読み込んだモデルの情報(パーツ数・親子・半径)を表示する
class ModelDemoController : public Component {
private:
    ModelRenderer* moving = nullptr;        // 動かす方のモデル
    ModelRenderer* reference = nullptr;     // 動かさない見本のモデル

    // Topパーツの元の姿(動かす前の値。ModelRenderer::GetNodeTransform で読んだもの)
    bool hasTop = false;                    // Topパーツが見つかったか
    DirectX::XMFLOAT3 topPosition{};        // 元の位置(親のBaseからの相対)
    DirectX::XMFLOAT3 topScale{ 1.f, 1.f, 1.f };   // 元の大きさ

    float time = 0.f;                       // 経過時間(上下・大きさの揺れに使う)
    bool topSpin = true;                    // Topを回すか(1キー)
    bool topBob = false;                    // Topを上下に動かすか(2キー)
    bool topPulse = false;                  // Topの大きさを変えるか(3キー)
    bool modelSpin = false;                 // モデル全体を回すか(4キー)
    int scaleIndex = 0;                     // モデル全体の大きさの段階(5キー)

public:
    // 動かす方と見本のモデルを、CreateModelDemo()から渡してもらう
    void Setup(ModelRenderer* movingModel, ModelRenderer* referenceModel) {
        moving = movingModel;
        reference = referenceModel;
    }

    void Update(float dt) override;
    void Draw() override;
};
