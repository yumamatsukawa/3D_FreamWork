#include "BulletSpawner.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../Components/BulletManager.h"

BulletManager* CreateBulletSpawner(Scene& scene) {
    GameObject* obj = scene.CreateObject("BulletSpawner");
    // 弾の発射(見た目は無く、管理役のComponentだけを持つ)
    return obj->AddComponent<BulletManager>();
}
