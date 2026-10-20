#include "CameraDemo3DScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGround.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/CameraDemo.h"

void CameraDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"Cameraデモ(三人称・一人称カメラ / CameraShake)",
        L"3Dカメラ(Image::GetCamera3D)を動かします。Vで、後ろから見る三人称と、自分の目から見る一人称を切り替えます。",
        L"操作: WASD 動く / V 視点 / 右ドラッグ 向き / Q・E ズーム / 1〜3 揺れの強さ / SPACE 揺らす / Tab: 2D版へ",
        L"使っているAPI: Image::GetCamera3D, SetCamera3DAutoSync(false), CameraShake::Shake" });
    CreateDemoSkybox(*this);
    CreateDemoGround(*this);
    CreateCameraDemo(*this, DemoMode::Mode3D);
}

void CameraDemo3DScene::Uninit()
{
    Scene::Uninit();
}
