#include "DemoWater.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/WaterRenderer.h"

GameObject* CreateDemoWater(Scene& scene, DirectX::XMFLOAT3 position) {
    GameObject* water = scene.CreateObject("Water");
    water->transform.position = position;

    // 半透明の物は、不透明の物(地形・Player)より後に描く(drawPriorityを大きくする)
    water->SetDrawPriority(100);

    // 見た目: 色・波の大きさ・速さを決める(当たり判定は付けないので、通り抜けられる)
    WaterSettings settings;
    settings.size = { 5000.0f, 5000.0f };             // 丘(4000 x 4000)の全体と、その外側をおおう広さ(丘が島に見える)
    settings.cells = 320;                             // 一番短い波(波の長さ x 0.23 = 約69)に、格子(5000 ÷ 320 = 約16)が4個以上入る細かさ
    settings.color = { 0.1f, 0.4f, 0.6f, 0.55f };     // 青緑で半透明(wが透明度)。水の下の地面が透けて見える
    settings.waveHeight = 8.0f;                       // 波の高さ(水位から上下に最大8)
    settings.waveLength = 300.0f;                     // 一番大きい波の長さ
    settings.waveSpeed = 70.0f;                       // 波の進む速さ
    settings.receiveShadow = true;                    // Playerなどの影が水面にも落ちる(§32)
    water->AddComponent<WaterRenderer>(settings);

    return water;
}
