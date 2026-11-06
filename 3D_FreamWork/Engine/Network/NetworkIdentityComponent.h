#pragma once
#include "../Core/Component.h"

// ネットワークで同期するオブジェクトに付ける印(NetworkId・所有者・Authority)。
// ホストの物にも、クライアントのコピーにも、同じComponentを付ける(違いは HasAuthority() が決める)。
//
// 使い方:
//   box->AddComponent<NetworkIdentityComponent>()->SetSceneId(10);   // シーンに最初から置く物の番号(1〜999)
//   if (!identity->HasAuthority()) return;                           // ルールを書くComponentの最初に。クライアントでは動かさない
//
// 決まり:
// - シーンに置く物の番号は、ホストとクライアントが同じシーンのコード(Init)で同じ番号を付ける。1〜999
// - スポーンした物の番号は、ホストが付ける(1000以上。Network::Spawn)
// - 非アクティブ(ObjectPoolに返した)になると登録簿から外れる。シーンに置いた物はアクティブに戻ると登録し直される。
//   スポーンした物は番号を手放すので、次のスポーン(Network::Spawn)で新しい番号が付く
class NetworkIdentityComponent : public Component {
private:
    int networkId = 0;            // 0 = まだ番号が無い
    int prefabId = 0;             // スポーンした物のプレハブ番号(シーンに置いた物は 0)
    int ownerPlayerIndex = -1;    // 所有者(この物を操作するプレイヤー)。-1 = 誰のでもない(ホストの物)

    void RegisterIfNeeded();

public:
    void SetSceneId(int id);                                    // シーンに最初から置く物の番号(1〜999)
    void SetSpawnInfo(int id, int prefab, int owner);           // Network がスポーンの時に呼ぶ(ゲーム側からは呼ばない)
    void SetOwnerPlayerIndex(int playerIndex) { ownerPlayerIndex = playerIndex; }

    int  GetNetworkId() const { return networkId; }
    int  GetPrefabId() const { return prefabId; }
    int  GetOwnerPlayerIndex() const { return ownerPlayerIndex; }
    bool IsOwner() const;                                       // 所有者が自分か
    bool HasAuthority() const;                                  // 本当の状態を決める側か(ホスト、またはオフライン)。クライアントは false

    void Uninit() override;                                     // 登録簿から外す
    void OnActiveChanged(bool active) override;                 // 非アクティブになったら登録簿から外す
};
