#include "EventBusDemo.h"
#include "../Common/DemoButton.h"
#include "../../Components/Core/EventLogManager.h"
#include "../../Components/Core/EventScoreManager.h"
#include "../../Components/Core/EventSenderManager.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"

GameObject* CreateEventBusDemo(Scene& scene) {
    // 記録係と受け取る側(お互いも、送る側も知らない。先に作った方が先に通知を受ける)
    GameObject* log = scene.CreateObject("EventLog");
    log->AddComponent<EventLogManager>();
    GameObject* score = scene.CreateObject("EventScore");
    score->AddComponent<EventScoreManager>();

    // コイン(コイン取得イベントで「誰が」として渡される物)
    GameObject* coin = scene.CreateObject("Coin");
    coin->transform.position = { -330.f, -40.f, 0.f };
    coin->transform.scale = { 56.f, 56.f, 1.f };
    coin->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(7);

    // 送る側と、イベントを送るボタン(押されたら、送る側に頼む)
    GameObject* sender = scene.CreateObject("EventSender");
    EventSenderManager* senderManager = sender->AddComponent<EventSenderManager>(coin);
    CreateDemoButton(scene, L"Defeat", { -480.f, 60.f }, { 240.f, 56.f }, 28.f, [senderManager] { senderManager->SendDefeat(); });
    CreateDemoButton(scene, L"Coin", { -480.f, -40.f }, { 240.f, 56.f }, 28.f, [senderManager] { senderManager->SendCoin(); });
    return sender;
}
