#include "OutlineDemoScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render3D/OutlineDemo.h"

void OutlineDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"枠線と縁の光のデモ(Outline)",
        L"物の外側に一定の太さの枠線を描きます。壁の後ろの物にも枠線を出せます。縁の光は、物の内側の縁を光らせます。",
        L"操作: 1・2・3 枠線・壁越し・縁の光 / 上下 枠線の太さ / 左右 縁の光の強さ / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: MeshRenderer::SetOutline(色, 太さ, seeThrough), SetRimLight, Material(outline*, rim*)" });
    CreateDemoStage3D(*this, { 0.0f, 30.0f, 120.0f }, 0.0f, -18.0f, 1000.0f);
    CreateOutlineDemo(*this);
}

void OutlineDemoScene::Uninit()
{
    Scene::Uninit();
}
