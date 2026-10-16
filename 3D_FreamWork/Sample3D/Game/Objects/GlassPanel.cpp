#include "GlassPanel.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"

GameObject* CreateGlassPanel(Scene& scene, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 size) {
    GameObject* panel = scene.CreateObject("GlassPanel");
    panel->transform.position = position;
    panel->transform.scale    = size;

    // 半透明の物は、不透明の物より後に描く(drawPriorityを大きくする)
    panel->SetDrawPriority(10);

    // 見た目(3Dメッシュの立方体)をマテリアルで設定する
    auto* meshRenderer = panel->AddComponent<MeshRenderer>(Mesh::CreateCube());
    Material& material = meshRenderer->GetMaterial();
    material.color = { 0.4f, 0.7f, 1.0f, 0.4f };   // 水色で半透明(aが透明度)
    material.depthWrite = false;                    // 自分より後ろの物を隠さない
    material.specularStrength = 0.9f;               // 光が当たる所がきらっと光る
    material.shininess = 64.f;
    material.emissiveColor = { 0.3f, 0.6f, 1.0f };  // ほんのり自分でも光る
    material.emissiveStrength = 0.15f;

    return panel;
}
