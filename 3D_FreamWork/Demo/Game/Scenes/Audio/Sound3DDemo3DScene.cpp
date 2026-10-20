#include "Sound3DDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Audio/Sound3DDemo.h"

void Sound3DDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"位置で聞こえ方が変わる音のデモ(Sound3D)",
        L"橙(音源)が回って鳴ります。青(聞く側)から遠いほど小さく、カメラから見て左なら左から聞こえます(ヘッドホン推奨)",
        L"操作: WASD 動く / 1 鳴/止 / 2 位置の効き / ↑↓ 聞こえる距離 / ←→ 回る速さ / SPACE 1回鳴らす / 右ドラッグ 視点 / Tab: 2D版へ",
        L"使っているAPI: AudioSourceComponent(SetSpatial, SetDistance), AudioListenerComponent, PlayOneShotAt" });
    GameObject* camera = CreateDemoStage3D(*this, { 0.0f, 0.0f, 20.0f }, 0.0f, -58.0f, 1250.0f);
    CreateSound3DDemo(*this, DemoMode::Mode3D, camera);
}

void Sound3DDemo3DScene::Uninit()
{
    Scene::Uninit();
}
