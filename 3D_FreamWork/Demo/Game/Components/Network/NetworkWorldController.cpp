#include "NetworkWorldController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Network/Network.h"
#include <vector>

void NetworkWorldController::Init() {
    build();
    builtAsClient = Network::IsClient();
}

void NetworkWorldController::Destroy() {
    // シーンに置いた同期する物(NetworkId 1〜999)を、登録簿から探して片付ける
    std::vector<GameObject*> targets;
    for (const auto& item : NetworkObjects::GetAll()) {
        if (item.first < Network::firstSpawnId && item.second->GetOwner()) targets.push_back(item.second->GetOwner());
    }
    for (GameObject* target : targets) target->Destroy();
}

void NetworkWorldController::Update(float dt) {
    // 作り直す(前のフレームで片付けた物のUninitが終わっているので、同じNetworkIdで作れる)
    if (needBuild) {
        needBuild = false;
        build();
        builtAsClient = Network::IsClient();
        return;
    }

    // 役割が変わった(クライアントになった/やめた)ら、片付けて次のフレームで作り直す
    if (Network::IsClient() != builtAsClient) {
        Destroy();
        needBuild = true;
    }
}