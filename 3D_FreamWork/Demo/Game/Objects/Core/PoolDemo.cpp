#include "PoolDemo.h"
#include "../../Components/Core/PoolDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"

GameObject* CreatePoolDemo(Scene& scene) {
    // 弾を出す台(見た目だけ。位置は PoolDemoController の muzzle と合わせる)
    GameObject* base = scene.CreateObject("PoolMuzzle");
    base->transform.position = { 0.f, -200.f, 0.f };
    base->transform.scale = { 56.f, 56.f, 1.f };
    base->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(4);

    // 弾のプールを持ち、キー・クリックで弾を出す役
    GameObject* demo = scene.CreateObject("PoolDemo");
    demo->AddComponent<PoolDemoController>();
    return demo;
}
