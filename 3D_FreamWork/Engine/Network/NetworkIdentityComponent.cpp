#include "NetworkIdentityComponent.h"
#include "NetworkObjects.h"
#include "Network.h"
#include "../Core/GameObject.h"
#include <windows.h>

void NetworkIdentityComponent::RegisterIfNeeded() {
    // 非アクティブな間は登録しない(アクティブに戻った時に OnActiveChanged が登録する)
    GameObject* owner = GetOwner();
    if (networkId > 0 && (!owner || owner->GetIsActive())) NetworkObjects::Register(networkId, this);
}

void NetworkIdentityComponent::SetSceneId(int id) {
    if (id < 1 || id >= Network::firstSpawnId) {
        OutputDebugStringA("[Network] SetSceneId: the number must be 1 to 999\n");
        return;
    }
    NetworkObjects::Unregister(this);
    networkId = id;
    RegisterIfNeeded();
}

void NetworkIdentityComponent::SetSpawnInfo(int id, int prefab, int owner) {
    NetworkObjects::Unregister(this);
    networkId = id;
    prefabId = prefab;
    ownerPlayerIndex = owner;
    RegisterIfNeeded();
}

bool NetworkIdentityComponent::IsOwner() const {
    return ownerPlayerIndex >= 0 && ownerPlayerIndex == Network::GetLocalPlayerIndex();
}

bool NetworkIdentityComponent::HasAuthority() const {
    return !Network::IsClient();   // ホストとオフラインは true、クライアントは false
}

void NetworkIdentityComponent::Uninit() {
    NetworkObjects::Unregister(this);
}

void NetworkIdentityComponent::OnActiveChanged(bool active) {
    if (active) {
        RegisterIfNeeded();
    }
    else {
        NetworkObjects::Unregister(this);
        // スポーンした物の番号は、使い終わったら手放す(ObjectPoolで使い回す時、古い番号で登録し直されて、
        // 同じ番号を新しく使っている別の物とぶつからないように。次の Spawn で SetSpawnInfo が付け直す)
        if (networkId >= Network::firstSpawnId) networkId = 0;
    }
}
