#include "ShadowDemo.h"
#include "../../Components/Render3D/ShadowDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/TextRenderer.h"

namespace {
    constexpr float groundTop = -50.f;   // 地面の上面の高さ(Objects/Ground.cpp と同じ)

    // 影のデモの物を1つ置く(position: 中心の位置。名前は labelOffsetY だけ上(下はマイナス)に出す)
    MeshRenderer* CreateShadowObject(Scene& scene, bool sphere, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 size,
        DirectX::XMFLOAT4 color, bool castShadow, const std::wstring& label, float labelOffsetY) {
        GameObject* object = scene.CreateObject("ShadowSample");
        object->transform.position = position;
        object->transform.scale = size;

        // 見た目: 影を落とす・受けるは、Materialで決める(どちらも既定はfalse)
        auto* renderer = object->AddComponent<MeshRenderer>(sphere ? Mesh::CreateSphere() : Mesh::CreateCube());
        renderer->GetMaterial().color = color;
        renderer->GetMaterial().castShadow = castShadow;
        renderer->GetMaterial().receiveShadow = true;

        // 名前(3Dのオブジェクトの上に出る文字)
        TextStyle style;
        style.size = 20.f;
        style.fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
        style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        auto* text = object->AddComponent<TextRenderer>();
        text->SetText(label);
        text->SetStyle(style);
        text->SetUse3DCamera(true);
        text->SetOffset({ 0.f, labelOffsetY, 0.f });
        return renderer;
    }
}

GameObject* CreateShadowDemo(Scene& scene, LightComponent* spotLight) {
    ShadowDemoTargets targets;
    targets.spotLight = spotLight;

    // 球: 影を落とす([1]で切り替え)
    targets.castSphere = CreateShadowObject(scene, true, { -350.f, groundTop + 60.f, 0.f }, { 120.f, 120.f, 120.f },
        { 0.9f, 0.5f, 0.3f, 1.0f }, true, L"球 [1]", 100.f);

    // 立方体: 影を落とさない(いつも)
    MeshRenderer* plainCube = CreateShadowObject(scene, false, { -150.f, groundTop + 55.f, 0.f }, { 110.f, 110.f, 110.f },
        { 0.4f, 0.8f, 0.5f, 1.0f }, false, L"影を落とさない", 100.f);

    // 板と箱: 板が影を落として、下の箱が受ける([2]で受ける・受けないを切り替え)
    MeshRenderer* board = CreateShadowObject(scene, false, { 200.f, groundTop + 190.f, 0.f }, { 200.f, 20.f, 200.f },
        { 0.8f, 0.8f, 0.9f, 1.0f }, true, L"板", 50.f);
    targets.receiveBox = CreateShadowObject(scene, false, { 200.f, groundTop + 55.f, 0.f }, { 110.f, 110.f, 110.f },
        { 0.9f, 0.4f, 0.4f, 1.0f }, false, L"箱 [2]", -95.f);

    // 壁: 影を落とす(スポットライトが当たると、後ろに長い影ができる)
    MeshRenderer* wall = CreateShadowObject(scene, false, { 450.f, groundTop + 130.f, 100.f }, { 40.f, 260.f, 200.f },
        { 0.7f, 0.7f, 0.75f, 1.0f }, true, L"壁", 170.f);

    targets.outlineTargets = { targets.castSphere, plainCube, board, targets.receiveBox, wall };

    // 操作役
    GameObject* demo = scene.CreateObject("ShadowDemo");
    demo->AddComponent<ShadowDemoController>()->Setup(targets);
    return demo;
}
