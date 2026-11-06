#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <DirectXMath.h>

class Scene;
class GameObject;
class NetworkIdentityComponent;
class NetworkTransformComponent;

// スポーンするとき、工場関数に渡す情報
struct NetworkSpawnInfo {
    int networkId = 0;
    int prefabId = 0;
    int ownerPlayerIndex = -1;
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT3 rotate{};
    bool hasAuthority = false;            // この PC が本当の状態を決める側か(工場関数が Dynamic / Kinematic を選ぶ)
};

// 同期するオブジェクトの登録簿(NetworkId → GameObject)と、プレハブ(番号 → 工場関数)・スポーンした物の記録。
// 各Componentの Init / Uninit / OnActiveChanged が自動で登録・解除する。シーンが変わる(Uninit)と自動で消えるので、
// 前のシーンのポインタが残ることは無い。
namespace NetworkObjects {
    using PrefabFactory = std::function<GameObject*(Scene& scene, const NetworkSpawnInfo& info)>;
    using DespawnHandler = std::function<void(GameObject* object)>;

    // ─── 登録簿 ───
    void Register(int networkId, NetworkIdentityComponent* identity);   // 同じ番号が既にあれば上書きして知らせる
    void Unregister(NetworkIdentityComponent* identity);
    NetworkIdentityComponent* Find(int networkId);                      // 無ければ nullptr
    const std::unordered_map<int, NetworkIdentityComponent*>& GetAll();

    // ─── 位置の同期をするComponent(登録簿を通じて Network が呼ぶ) ───
    void RegisterTransformSync(NetworkTransformComponent* component);
    void UnregisterTransformSync(NetworkTransformComponent* component);
    const std::vector<NetworkTransformComponent*>& GetTransformSyncs();

    // ─── プレハブ(ホストもクライアントも、シーンの Init で同じ登録をする。ClearPrefabs はシーンの Uninit で) ───
    void RegisterPrefab(int prefabId, PrefabFactory factory);
    const PrefabFactory* FindPrefab(int prefabId);
    void SetDespawnHandler(int prefabId, DespawnHandler handler);
    const DespawnHandler* FindDespawnHandler(int prefabId);
    void ClearPrefabs();

    // ─── スポーンした物の記録(ホストだけ。途中参加の人へ今の物を教えるのに使う) ───
    struct SpawnRecord {
        int networkId = 0;
        int prefabId = 0;
        int ownerPlayerIndex = -1;
    };
    int  AllocateSpawnId(double now);                         // 使える番号を返す(使い切ったら 0)
    void AddSpawnRecord(const SpawnRecord& record);
    void RemoveSpawnRecord(int networkId, double now);        // 記録から外して、その番号をしばらく使い回さないようにする
    std::vector<SpawnRecord> GetSpawnRecords();               // 今も生きている物だけ
    void ClearSpawnRecords();

    void ClearAll();                                          // 全部消す(Network の終了・再開始用)
}
