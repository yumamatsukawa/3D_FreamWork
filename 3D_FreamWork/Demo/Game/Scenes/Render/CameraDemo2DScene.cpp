#include "CameraDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/CameraDemo.h"

void CameraDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Cameraデモ(CameraFollowComponent / CameraShake)",
        L"自分(おばけ)を2Dカメラが追いかけます。なめらかさ・デッドゾーン・映す範囲・ずらしを切り替えて、画面の揺れも試せます。",
        L"操作: WASD・矢印 動く / 1 なめらかさ / 2 デッドゾーン / 3 範囲 / 4 ずらし / SPACE 揺らす / Tab: 3D版へ",
        L"使っているAPI: CameraFollowComponent(SetSmoothTime, SetDeadZone, SetBounds, SetOffset), CameraShake::Shake" });
    CreateCameraDemo(*this, DemoMode::Mode2D);
}

void CameraDemo2DScene::Uninit()
{
    Scene::Uninit();
}
