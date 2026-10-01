#include "GameScene.h"
#include "../Objects/Player.h"
#include "../Objects/Cube.h"
#include "../Objects/Sphere.h"
#include "../Objects/Skybox.h"
#include "../Objects/Ground.h"
#include "../../../Engine/RigidbodyComponent.h"

void GameScene::Init()
{
    // Colliderの実際の大きさ・位置を枠で表示する(デバッグ用)。
    // 不要になったらこの行を削除するかfalseにすればよい
    RigidbodyComponent::SetDebugDrawEnabled(true);

    CreateSkybox(*this);
    CreateGround(*this);
    CreatePlayer(*this);
    CreateCube(*this);
    CreateSphere(*this);
}

void GameScene::Uninit()
{
    Scene::Uninit();
}
