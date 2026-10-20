#include "SpriteDemo.h"
#include "../../Components/Render/SpriteDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    // 3D版のスプライト1つ分の設定(名前・位置・ビルボードで固定する軸)
    struct BillboardSpec {
        const wchar_t* label;
        DirectX::XMFLOAT3 position;
        bool lockX, lockY, lockZ;
    };
    const BillboardSpec billboardSpecs[4] = {
        { L"Lock なし",      { -300.f, 0.f, 200.f }, false, false, false },   // 常にカメラの方を完全に向く
        { L"Lock X・Z(立て看板)", { -10.f, 0.f, 200.f },  true,  false, true  },   // 水平方向だけカメラへ向く(立て看板。立方体の手前)
        { L"Lock 全部", {  250.f, 0.f, 200.f }, true,  true,  true  },   // ビルボードなし(向きが変わらない)
        { L"奥(Lock X・Z)", {  60.f, 0.f, 470.f }, true,  false, true  },   // 立方体に隠れる
    };
}

GameObject* CreateSpriteDemo(Scene& scene, DemoMode mode) {
    // 操作役(スプライトを足していく)
    GameObject* demo = scene.CreateObject("SpriteDemo");
    SpriteDemoController* controller = demo->AddComponent<SpriteDemoController>(mode);

    if (mode == DemoMode::Mode2D) {
        // 1〜4キーで1つずつ動く4つ(コマ・色・大きさ・回転)
        const wchar_t* const labels[4] = { L"コマ(Frame)", L"色(Color)", L"大きさ(Size)", L"回転(Rotate)" };
        const int effects[4] = { SpriteDemoController::Frame, SpriteDemoController::Color, SpriteDemoController::Size, SpriteDemoController::Rotate };
        for (int i = 0; i < 4; i++) {
            GameObject* object = scene.CreateObject("Sprite");
            object->transform.position = { -450.f + 300.f * (float)i, 10.f, 0.f };
            object->transform.scale = { 110.f, 110.f, 1.f };
            SpriteRenderer* sprite = object->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3);
            sprite->SetSpriteIndex(i * 2);
            controller->AddSprite(object, sprite, effects[i], labels[i]);
        }

        // UI・2D・3Dの描き方で重なり方と大きさが変わる2つ(赤が手前、青が奥。奥の方を後に作る)
        SpriteRenderer* pair[2] = {};
        for (int i = 0; i < 2; i++) {
            GameObject* object = scene.CreateObject(i == 0 ? "NearSprite" : "FarSprite");
            object->transform.position = { 300.f + 80.f * (float)i, -150.f, i == 0 ? -100.f : 100.f };
            object->transform.scale = { 120.f, 120.f, 1.f };
            pair[i] = object->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3);
            pair[i]->SetSpriteIndex(i == 0 ? 0 : 6);
        }
        controller->SetOverlapPair(pair[0], pair[1]);

        // 常にUIの緑(いちばん奥のz=+500。UIはzに関係なく、2D・3Dの2つより必ず手前に出る)
        GameObject* uiObject = scene.CreateObject("UiSprite");
        uiObject->transform.position = { 340.f, -205.f, 500.f };
        uiObject->transform.scale = { 90.f, 90.f, 1.f };
        uiObject->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(4);
    }
    else {
        // 真ん中の立方体(奥行きで前後が決まるのを見る)
        GameObject* cube = scene.CreateObject("Cube");
        cube->transform.position = { 0.f, 0.f, 360.f };
        cube->transform.scale = { 100.f, 100.f, 100.f };
        cube->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(0.6f, 0.6f, 0.65f, 1.0f);

        // ビルボードのスプライト4つ(ビルボードで固定する軸が違う。1〜4キーの動きは全部が受ける)
        for (const BillboardSpec& spec : billboardSpecs) {
            GameObject* object = scene.CreateObject("Sprite");
            object->transform.position = spec.position;
            object->transform.scale = { 90.f, 90.f, 1.f };
            SpriteRenderer* sprite = object->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3);
            sprite->SetWorldSpace(true);
            sprite->SetBillboardLock(spec.lockX, spec.lockY, spec.lockZ);
            controller->AddSprite(object, sprite, SpriteDemoController::Frame | SpriteDemoController::Color | SpriteDemoController::Size | SpriteDemoController::Rotate, spec.label);
        }
    }
    return demo;
}
