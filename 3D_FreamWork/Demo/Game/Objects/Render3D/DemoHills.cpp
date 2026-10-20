#include "DemoHills.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/Terrain.h"
#include "../../../../Engine/Render3D/TerrainRenderer.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <climits>

GameObject* CreateDemoHills(Scene& scene, DirectX::XMFLOAT3 corner) {
    GameObject* hills = scene.CreateObject("Hills");
    hills->transform.position = corner;

    // スカイボックス(INT_MIN)のすぐ後、他のオブジェクトより先に描く(地面と同じ。半透明の物より後に描くと透けた所に地面が出ないことがあるため)
    hills->SetDrawPriority(INT_MIN + 1);

    // 高さの画像(白=高い、黒=低い。129x129)から地形を作る。画像が読めなければ nullptr
    TerrainSettings settings;
    settings.width = 4000.0f;
    settings.depth = 4000.0f;
    settings.maxHeight = 600.0f;
    settings.cellsX = 128;
    settings.cellsZ = 128;
    settings.uvRepeat = 4.0f;   // 1000四方ごとに画像を1回貼る(Groundのタイルと同じ大きさ)
    settings.texture = L"Assets/bg_tile.png";
    auto terrain = Terrain::LoadFromImage(L"Assets/Terrain/heightmap.png", settings);
    if (!terrain) return hills;

    // 見た目: 影を受ける。画像がとても暗いので、Groundと同じ色を掛けて明るい青灰色にする
    auto* renderer = hills->AddComponent<TerrainRenderer>(terrain);
    renderer->GetMaterial().color = { 5.0f, 4.0f, 2.0f, 1.0f };
    renderer->GetMaterial().receiveShadow = true;

    // 当たり判定(PhysX): 見た目と同じ地形のデータから作るので、丘の起伏の上を歩ける。動かない
    hills->AddComponent<TerrainRigidbodyComponent>(terrain);

    return hills;
}
