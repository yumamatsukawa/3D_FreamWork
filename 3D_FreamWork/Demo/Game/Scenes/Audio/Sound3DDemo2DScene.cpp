#include "Sound3DDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Audio/Sound3DDemo.h"

void Sound3DDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"位置で聞こえ方が変わる音のデモ(Sound3D)",
        L"橙(音源)が回って鳴ります。青(聞く側)から遠いほど小さく、左にあれば左から聞こえます(ヘッドホン推奨)",
        L"操作: WASD 動く / 1 鳴/止 / 2 位置の効き / ↑↓ 聞こえる距離 / ←→ 回る速さ / SPACE 1回鳴らす / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: AudioSourceComponent(SetSpatial, SetDistance), AudioListenerComponent, PlayOneShotAt" });
    CreateSound3DDemo(*this, DemoMode::Mode2D);
}

void Sound3DDemo2DScene::Uninit()
{
    Scene::Uninit();
}
