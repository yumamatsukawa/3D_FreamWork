#include "CameraDemo.h"
#include "../../Components/Render/CameraDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render/CameraFollowComponent.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include <cmath>

GameObject* CreateCameraDemo(Scene& scene, DemoMode mode) {
    const bool is3D = (mode == DemoMode::Mode3D);

    // 動いたのが分かる目印(2D: 箱を5x5に並べる / 3D: 柱を円に並べる)
    if (!is3D) {
        for (int ix = -2; ix <= 2; ix++) {
            for (int iy = -2; iy <= 2; iy++) {
                GameObject* mark = scene.CreateObject("Mark");
                mark->transform.position = { 400.f * (float)ix, 240.f * (float)iy, 0.f };
                mark->transform.scale = { 56.f, 56.f, 1.f };
                mark->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(((ix + 2) + (iy + 2) * 5) % 9);
            }
        }
    }
    else {
        for (int i = 0; i < 24; i++) {
            const float angle = 6.2831853f * (float)i / 24.f;
            const float radius = (i % 2 == 0) ? 700.f : 1100.f;
            GameObject* pillar = scene.CreateObject("Pillar");
            pillar->transform.position = { sinf(angle) * radius, 50.f, cosf(angle) * radius };
            pillar->transform.scale = { 60.f, 200.f, 60.f };
            pillar->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(0.5f + 0.5f * sinf(angle), 0.5f + 0.5f * sinf(angle + 2.094f), 0.5f + 0.5f * sinf(angle + 4.189f), 1.0f);
        }
    }

    // 動かす自分(操作役を付ける。2Dはこの後にカメラの部品を付けて、動いた後の位置を追わせる)
    GameObject* self = scene.CreateObject("CameraTarget");
    CameraDemoController* controller = self->AddComponent<CameraDemoController>(mode);
    if (is3D) {
        self->transform.position = { 0.f, -20.f, 0.f };
        self->transform.scale = { 60.f, 60.f, 60.f };
        MeshRenderer* body = self->AddComponent<MeshRenderer>(Mesh::CreateCube());
        body->SetColor(1.0f, 0.9f, 0.3f, 1.0f);
        body->SetRimLight({ 0.3f, 0.7f, 1.0f }, 1.0f);
        controller->SetBody(body);
    }
    else {
        self->transform.position = { 0.f, 0.f, 0.f };   // 目印の箱より手前(UIモードは作った順に重なる。後に作ったので手前)
        self->transform.scale = { 70.f, 70.f, 1.f };
        self->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2)->SetSpriteIndex(0);
        controller->SetFollow(self->AddComponent<CameraFollowComponent>());
    }
    return self;
}
