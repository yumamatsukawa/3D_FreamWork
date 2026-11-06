#include "NetworkObjects.h"
#include "NetworkConfig.h"
#include "NetworkIdentityComponent.h"
#include "NetworkTransformComponent.h"
#include <algorithm>
#include <map>
#include <windows.h>

namespace {
    std::unordered_map<int, NetworkIdentityComponent*> registry;       // NetworkId → Identity
    std::vector<NetworkTransformComponent*> transformSyncs;            // 位置を同期するComponent
    std::unordered_map<int, NetworkObjects::PrefabFactory> prefabs;    // プレハブ番号 → 工場関数
    std::unordered_map<int, NetworkObjects::DespawnHandler> despawnHandlers;
    std::vector<NetworkObjects::SpawnRecord> spawnRecords;
    std::map<int, double> recentlyFreed;                               // 消したNetworkId → 消した時刻(しばらく使い回さない)
    int nextSpawnId = Network::firstSpawnId;
}

namespace NetworkObjects {

void Register(int networkId, NetworkIdentityComponent* identity) {
    if (networkId <= 0 || !identity) return;
    auto it = registry.find(networkId);
    if (it != registry.end() && it->second != identity) {
        OutputDebugStringA(("[Network] NetworkId " + std::to_string(networkId) + " is used by two objects (the last one wins)\n").c_str());
    }
    registry[networkId] = identity;
}

void Unregister(NetworkIdentityComponent* identity) {
    for (auto it = registry.begin(); it != registry.end();) {
        if (it->second == identity) it = registry.erase(it);
        else ++it;
    }
}

NetworkIdentityComponent* Find(int networkId) {
    auto it = registry.find(networkId);
    return it == registry.end() ? nullptr : it->second;
}

const std::unordered_map<int, NetworkIdentityComponent*>& GetAll() {
    return registry;
}

void RegisterTransformSync(NetworkTransformComponent* component) {
    if (std::find(transformSyncs.begin(), transformSyncs.end(), component) == transformSyncs.end()) transformSyncs.push_back(component);
}

void UnregisterTransformSync(NetworkTransformComponent* component) {
    transformSyncs.erase(std::remove(transformSyncs.begin(), transformSyncs.end(), component), transformSyncs.end());
}

const std::vector<NetworkTransformComponent*>& GetTransformSyncs() {
    return transformSyncs;
}

void RegisterPrefab(int prefabId, PrefabFactory factory) {
    prefabs[prefabId] = std::move(factory);
}

const PrefabFactory* FindPrefab(int prefabId) {
    auto it = prefabs.find(prefabId);
    return it == prefabs.end() ? nullptr : &it->second;
}

void SetDespawnHandler(int prefabId, DespawnHandler handler) {
    despawnHandlers[prefabId] = std::move(handler);
}

const DespawnHandler* FindDespawnHandler(int prefabId) {
    auto it = despawnHandlers.find(prefabId);
    return it == despawnHandlers.end() ? nullptr : &it->second;
}

void ClearPrefabs() {
    prefabs.clear();
    despawnHandlers.clear();
}

int AllocateSpawnId(double now) {
    // 古い「消した番号」の記録を片付ける
    for (auto it = recentlyFreed.begin(); it != recentlyFreed.end();) {
        if (now - it->second > Network::spawnIdReuseDelay) it = recentlyFreed.erase(it);
        else ++it;
    }
    // 順に探す(65535の次は firstSpawnId に戻る。使用中・消したばかりの番号は飛ばす)
    for (int tries = 0; tries < 65536; tries++) {
        const int id = nextSpawnId;
        nextSpawnId = (nextSpawnId >= 65535) ? Network::firstSpawnId : nextSpawnId + 1;
        if (registry.count(id) || recentlyFreed.count(id)) continue;
        bool recorded = false;
        for (const SpawnRecord& record : spawnRecords) if (record.networkId == id) { recorded = true; break; }
        if (!recorded) return id;
    }
    return 0;
}

void AddSpawnRecord(const SpawnRecord& record) {
    spawnRecords.push_back(record);
}

void RemoveSpawnRecord(int networkId, double now) {
    spawnRecords.erase(std::remove_if(spawnRecords.begin(), spawnRecords.end(),
        [networkId](const SpawnRecord& record) { return record.networkId == networkId; }), spawnRecords.end());
    recentlyFreed[networkId] = now;
}

std::vector<SpawnRecord> GetSpawnRecords() {
    // 登録簿から消えた物(シーンの切り替えなど)の記録は捨てる
    spawnRecords.erase(std::remove_if(spawnRecords.begin(), spawnRecords.end(),
        [](const SpawnRecord& record) { return registry.find(record.networkId) == registry.end(); }), spawnRecords.end());
    return spawnRecords;
}

void ClearSpawnRecords() {
    spawnRecords.clear();
    recentlyFreed.clear();
    nextSpawnId = Network::firstSpawnId;
}

void ClearAll() {
    ClearPrefabs();
    ClearSpawnRecords();
}

}
