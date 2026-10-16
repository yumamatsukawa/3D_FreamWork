#pragma once
#include "../../../Engine/Component.h"

// 影の設定を担当する(このシーンの間だけ影を有効にして、終わったら元に戻す。影を落とす・受ける物は、各オブジェクトの Material で決める)
class ShadowManager : public Component {
private:
    static constexpr float shadowDistance = 2500.0f;   // カメラの前方、この大きさの範囲に影を作る(広いほど影がぼやける)

public:
    void Init() override;
    void Uninit() override;
};
