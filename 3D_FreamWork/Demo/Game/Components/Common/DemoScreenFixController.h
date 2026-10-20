#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

// 付けた物(デモのボタンなど)を、2Dカメラが動いても画面の同じ場所に見えるようにする。
// 付けた時の位置を「画面の位置」として覚え、毎フレーム(Updateと、描く直前のDraw)カメラの分だけずらす。
// 描く前に位置を直すので、他のComponent(SpriteRendererなど)より先に付ける。2Dカメラが動かないデモでは、位置は変わらない
class DemoScreenFixController : public Component {
private:
    DirectX::XMFLOAT2 screenPosition{};    // 画面の位置(カメラが原点にある時のワールドの位置と同じ)

    void Place();   // カメラの位置の分だけずらして、画面の同じ場所に見える位置に置く

public:
    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
