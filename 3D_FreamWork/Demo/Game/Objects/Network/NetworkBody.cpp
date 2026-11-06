#include "NetworkBody.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Network/Network.h"

GameObject* CreateNetworkBody(Scene& scene, DemoMode mode, int sceneId, DemoBodyDesc desc) {
    // 本当の状態を決めるのはホスト(とオフライン)だけ。クライアントのコピーはDynamicにしない
    if (desc.type == BodyType::Dynamic && !Network::IsHost()) desc.type = BodyType::Kinematic;

    GameObject* body = (mode == DemoMode::Mode2D) ? CreateDemoBody2D(scene, desc) : CreateDemoBody3D(scene, desc);

    // 同期の印と、位置・向きの同期(2Dは position.xy と rotate.z だけ送る)
    body->AddComponent<NetworkIdentityComponent>()->SetSceneId(sceneId);
    NetworkTransformSettings settings;
    settings.is2D = (mode == DemoMode::Mode2D);
    body->AddComponent<NetworkTransformComponent>(settings);
    return body;
}