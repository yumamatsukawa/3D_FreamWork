#include "Network.h"
#include "NetworkSession.h"
#include "LanDiscovery.h"
#include "../Core/Time.h"
#include "../Core/EventBus.h"
#include "../Core/Scene.h"
#include "../Core/GameObject.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>

namespace {
    constexpr float sceneChangeRetryTime = 2.f;   // シーンの切り替えを頼んでから、この秒数たっても終わらなければ、もう一度頼む
    constexpr int sceneChangeMaxTries = 3;

    // 通信の状態(最初に StartHost / Join / StartSearch / Spawn などを呼んだ時に作る。何も呼ばなければ作られない)
    struct State {
        NetworkSession session;
        LanDiscovery discovery;
        std::string gameId = "Game";
        double time = 0.0;                         // Network が動いてからの時間(消した番号の使い回し防止などに使う)

        // 入力
        NetworkInput localInput;                   // SetLocalInput の最新
        bool inputEverSet = false;                 // SetLocalInput が一度でも呼ばれたか(呼ばれないゲームは入力を送らない)
        unsigned short prevButtons = 0;            // 前回のボタンの状態(押した瞬間を求める用)
        unsigned short localPending = 0;           // 送信待ちの「押した瞬間」
        unsigned short pressedHistory[3] = {};     // クライアント: 直近3回の送信の「押した瞬間」(新しい順。1つ落ちても取りこぼさないよう重ねて送る)
        uint16_t sendSeq = 0;                      // クライアント: 入力の連番
        float inputTimer = 0.f;
        NetworkInput inputs[Network::maxPlayers];  // ホスト: プレイヤーごとの入力
        double lastInputTime[Network::maxPlayers] = {};
        uint16_t lastRecvSeq[Network::maxPlayers] = {};
        bool haveRecvSeq[Network::maxPlayers] = {};

        // 出来事・共有の値
        std::unordered_map<std::string, std::vector<Network::EventHandler>> handlers;
        std::map<std::string, int> shared;

        // 位置のスナップショット
        float snapshotTimer = 0.f;
        uint16_t snapshotSeq = 0;                  // ホスト: 連番
        bool haveSnapshot = false;                 // クライアント: 一度でも受け取ったか
        uint16_t lastSnapshotSeq = 0;
        double lastSnapshotTime = 0.0;             // クライアント: 最後に受け取ったホストの時刻
        double estHostTime = 0.0;                  // クライアント: ホストの時刻の推定(毎フレーム進め、届く度に少しだけ合わせる)
        bool haveEstimate = false;
        float snapshotAge = 0.f;                   // クライアント: 最後に位置が届いてからの秒数
        int snapshotRate = Network::defaultSnapshotRate;
        float interpolationDelay = Network::defaultInterpolationDelay;

        // シーン
        Scene* currentScene = nullptr;
        int currentSceneId = 0;
        bool localReady = false;                   // 今のシーンの準備ができているか(NotifySceneReady を呼んだか)
        int sceneGeneration = 0;                   // シーンの世代番号(切り替える度に進む。古いシーンのパケットを捨てるのに使う)
        bool playerReady[Network::maxPlayers] = {};   // ホスト: そのクライアントの今のシーンの準備ができているか
        std::map<int, std::function<void()>> scenes;
        int pendingSceneId = -1;                   // クライアント: ホストに言われて切り替え中のシーン
        float pendingSceneTimer = 0.f;
        int pendingSceneTries = 0;

        std::function<void(int)> joinedHandler;
        std::function<void(int)> leftHandler;
        bool debugLog = false;
    };

    State* g = nullptr;

    void Log(const std::string& text) {
        OutputDebugStringA(("[Network] " + text + "\n").c_str());
    }

    float Clamp01Axis(float v) { return (std::max)(-1.f, (std::min)(1.f, v)); }

    void HandlePacket(int sender, PacketType type, NetworkReader& reader);
    void DispatchEvent(const std::string& name, int sender, const uint8_t* payload, size_t size);
    void DespawnInternal(NetworkIdentityComponent* identity);
    void SendStateTo(int playerIndex);
    void CallSceneFunction(int sceneId);

    // 接続のたびに、通信の途中経過を初期状態に戻す
    void ResetRuntime() {
        for (int i = 0; i < Network::maxPlayers; i++) {
            g->inputs[i] = NetworkInput();
            g->lastInputTime[i] = 0.0;
            g->haveRecvSeq[i] = false;
            g->playerReady[i] = false;
        }
        g->pressedHistory[0] = g->pressedHistory[1] = g->pressedHistory[2] = 0;
        g->localPending = 0;
        g->sendSeq = 0;
        g->inputTimer = 0.f;
        g->snapshotTimer = 0.f;
        g->snapshotSeq = 0;
        g->haveSnapshot = false;
        g->haveEstimate = false;
        g->snapshotAge = 0.f;
        g->shared.clear();
        g->pendingSceneId = -1;
        g->session.sceneGeneration = 0;
        g->session.sceneId = g->currentSceneId;
        g->sceneGeneration = 0;
        NetworkObjects::ClearSpawnRecords();
        for (NetworkTransformComponent* sync : NetworkObjects::GetTransformSyncs()) sync->ClearSamples();
    }

    void EnsureState() {
        if (g) return;
        g = new State();

        // セッションからの通知を、このファイルの処理につなぐ
        g->session.onPacket = [](int sender, PacketType type, NetworkReader& reader) { HandlePacket(sender, type, reader); };
        g->session.onTrace = [](PacketType type, int player, bool received) {
            if (!g || !g->debugLog) return;
            Log(std::string(received ? "recv " : "send ") + GetPacketTypeName(type) + " player " + std::to_string(player));
        };
        g->session.onPlayerJoined = [](int index) {
            if (g->joinedHandler) g->joinedHandler(index);
            EventBus::Get().Publish("NetworkPlayerJoined");
        };
        g->session.onPlayerLeft = [](int index) {
            if (g->session.IsHost()) {
                // 抜けた人が所有するスポーンした物は、自動で消す
                for (const NetworkObjects::SpawnRecord& record : NetworkObjects::GetSpawnRecords()) {
                    if (record.ownerPlayerIndex != index) continue;
                    NetworkIdentityComponent* identity = NetworkObjects::Find(record.networkId);
                    if (identity) DespawnInternal(identity);
                }
                g->inputs[index] = NetworkInput();
                g->playerReady[index] = false;
                g->haveRecvSeq[index] = false;
            }
            if (g->leftHandler) g->leftHandler(index);
            EventBus::Get().Publish("NetworkPlayerLeft");
        };
        g->session.onClientAccepted = [](int index) {
            // 途中参加の人へ、共有の値をまとめて送る(物の状態は、その人が SceneReady を送ってきてから)
            g->inputs[index] = NetworkInput();
            g->playerReady[index] = false;
            g->haveRecvSeq[index] = false;
            g->lastInputTime[index] = g->session.GetClock();
            for (const auto& item : g->shared) {
                NetworkWriter packet = BeginPacket(PacketType::SharedValue);
                packet.WriteString(item.first);
                packet.WriteI32(item.second);
                g->session.SendTo(index, packet, Network::channelReliable, true);
            }
        };
        g->session.onConnected = []() {
            g->shared.clear();
            g->haveSnapshot = false;
            g->haveEstimate = false;
            g->sceneGeneration = g->session.sceneGeneration;
            g->pressedHistory[0] = g->pressedHistory[1] = g->pressedHistory[2] = 0;
            g->localPending = 0;
            g->inputTimer = 0.f;
            for (NetworkTransformComponent* sync : NetworkObjects::GetTransformSyncs()) sync->ClearSamples();

            // 今いるシーンの準備ができているなら、ホストに伝える(伝えると、今の物の状態が届く)
            if (g->localReady) {
                NetworkWriter packet = BeginPacket(PacketType::SceneReady);
                packet.WriteU8((uint8_t)g->sceneGeneration);
                packet.WriteI32(g->currentSceneId);
                g->session.SendToHost(packet, Network::channelReliable, true);
            }
            // ホストが別のシーンにいて、そのシーンへの切り替え方が登録されていれば切り替える
            if (g->session.sceneId != 0 && g->session.sceneId != g->currentSceneId) CallSceneFunction(g->session.sceneId);

            EventBus::Get().Publish("NetworkConnected");
        };
        g->session.onDisconnected = []() {
            g->shared.clear();
            g->haveSnapshot = false;
            g->haveEstimate = false;
            g->pendingSceneId = -1;
            for (int i = 0; i < Network::maxPlayers; i++) g->inputs[i] = NetworkInput();
            EventBus::Get().Publish("NetworkDisconnected");
        };
    }

    // ─── 出来事 ───

    void DispatchEvent(const std::string& name, int sender, const uint8_t* payload, size_t size) {
        auto it = g->handlers.find(name);
        if (it != g->handlers.end()) {
            std::vector<Network::EventHandler> copy = it->second;   // 呼んでいる間の Subscribe に備えてコピー
            for (auto& handler : copy) {
                NetworkReader reader(payload, size);
                handler(reader, sender);
            }
        }
        // 中身の無い出来事は、EventBus にも届ける
        if (size == 0) EventBus::Get().Publish(name);
    }

    NetworkWriter BuildEventPacket(const std::string& name, const uint8_t* payload, size_t size, NetworkTarget target, int sender) {
        NetworkWriter packet = BeginPacket(PacketType::Event);
        packet.WriteU8((uint8_t)target);
        packet.WriteU8((uint8_t)sender);
        packet.WriteString(name);
        packet.WriteU16((uint16_t)size);
        packet.WriteBytes(payload, size);
        return packet;
    }

    void HandleEventPacket(int sender, NetworkReader& reader) {
        const uint8_t rawTarget = reader.ReadU8();
        const int claimedSender = reader.ReadU8();
        const std::string name = reader.ReadString(Network::maxEventNameLength);
        const size_t size = reader.ReadU16();
        if (!reader.IsOk() || rawTarget > (uint8_t)NetworkTarget::Others || size > (size_t)Network::maxEventPayloadSize || size > reader.GetRemaining() || name.empty()) return;
        std::vector<uint8_t> payload(size);
        if (!reader.ReadBytes(payload.data(), size)) return;
        const NetworkTarget target = (NetworkTarget)rawTarget;

        if (g->session.IsHost()) {
            // クライアントから届いた: 送り主は接続から決めた番号(パケット内の番号は信用しない)
            if (sender <= 0) return;
            DispatchEvent(name, sender, payload.data(), size);
            if (target != NetworkTarget::Host) {
                // 全員(送った本人にも戻る)/ 自分以外(送った本人を除く)へ中継する
                const NetworkWriter relay = BuildEventPacket(name, payload.data(), size, target, sender);
                g->session.SendToAll(relay, Network::channelReliable, true, target == NetworkTarget::Others ? sender : -1);
            }
        }
        else {
            if (claimedSender >= Network::maxPlayers) return;
            DispatchEvent(name, claimedSender, payload.data(), size);
        }
    }

    // ─── 位置のスナップショット ───

    void SendSnapshots() {
        bool anyReady = false;
        for (int i = 1; i < Network::maxPlayers; i++) {
            if (g->session.IsPlayerConnected(i) && g->playerReady[i]) anyReady = true;
        }
        if (!anyReady) return;

        const uint16_t seq = g->snapshotSeq++;
        const double hostTime = g->session.GetClock();

        // 1パケットが大きくなりすぎないよう、約1.1KBごとに分ける(分割のどれか1つが落ちると全部捨てられるため)
        NetworkWriter entries;
        int count = 0;
        auto flush = [&]() {
            if (count == 0) return;
            NetworkWriter packet = BeginPacket(PacketType::Snapshot);
            packet.WriteU8((uint8_t)g->sceneGeneration);
            packet.WriteU16(seq);
            packet.WriteDouble(hostTime);
            packet.WriteU8((uint8_t)count);
            packet.WriteBytes(entries.GetData().data(), entries.GetSize());
            for (int i = 1; i < Network::maxPlayers; i++) {
                if (g->session.IsPlayerConnected(i) && g->playerReady[i]) g->session.SendTo(i, packet, Network::channelSnapshot, false);
            }
            entries.Clear();
            count = 0;
        };

        for (NetworkTransformComponent* sync : NetworkObjects::GetTransformSyncs()) {
            GameObject* owner = sync->GetOwner();
            if (!owner || !owner->GetIsActive()) continue;
            NetworkIdentityComponent* identity = owner->GetComponent<NetworkIdentityComponent>();
            if (!identity || identity->GetNetworkId() <= 0) continue;

            NetworkWriter entry;
            sync->WriteState(entry);
            if (entries.GetSize() + entry.GetSize() + 3 > (size_t)Network::snapshotChunkSize || count >= 250) flush();
            entries.WriteU16((uint16_t)identity->GetNetworkId());
            entries.WriteU8((uint8_t)entry.GetSize());
            entries.WriteBytes(entry.GetData().data(), entry.GetSize());
            count++;
        }
        flush();
    }

    void HandleSnapshot(NetworkReader& reader) {
        if (!g->localReady) return;   // 今のシーンの準備ができるまでは使わない
        const int generation = reader.ReadU8();
        const uint16_t seq = reader.ReadU16();
        const double hostTime = reader.ReadDouble();
        const int count = reader.ReadU8();
        if (!reader.IsOk() || generation != (g->sceneGeneration & 0xFF)) return;

        // 連番が古い物(順番が入れ替わって届いた物)は捨てる。同じ連番は、分けて送られた続き
        if (g->haveSnapshot && (int16_t)(seq - g->lastSnapshotSeq) < 0) return;

        for (int i = 0; i < count; i++) {
            const int id = reader.ReadU16();
            const size_t length = reader.ReadU8();
            if (!reader.IsOk() || length > reader.GetRemaining()) return;
            NetworkReader entry(reader.GetCurrent(), length);
            uint8_t skip[255];
            reader.ReadBytes(skip, length);

            // 知らない NetworkId は無視する(スポーンのパケットより先に届くことがある)
            NetworkIdentityComponent* identity = NetworkObjects::Find(id);
            GameObject* owner = identity ? identity->GetOwner() : nullptr;
            NetworkTransformComponent* sync = owner ? owner->GetComponent<NetworkTransformComponent>() : nullptr;
            if (!sync) continue;
            sync->ReadState(entry, hostTime);
        }

        g->haveSnapshot = true;
        g->lastSnapshotSeq = seq;
        g->snapshotAge = 0.f;

        // ホストの時刻の推定を、届いた時刻に少しだけ合わせる(大きくずれていたら飛ばす)
        if (hostTime > g->lastSnapshotTime || !g->haveEstimate) {
            if (!g->haveEstimate || std::fabs(hostTime - g->estHostTime) > 0.5) g->estHostTime = hostTime;
            else g->estHostTime += (hostTime - g->estHostTime) * 0.15;
            g->haveEstimate = true;
            g->lastSnapshotTime = hostTime;
        }
    }

    // ─── 入力 ───

    void HandleInputPacket(int sender, NetworkReader& reader) {
        if (!g->session.IsHost() || sender <= 0) return;
        const uint16_t seq = reader.ReadU16();
        NetworkInput in;
        in.moveX = Clamp01Axis(reader.ReadFloat());
        in.moveY = Clamp01Axis(reader.ReadFloat());
        in.lookX = Clamp01Axis(reader.ReadFloat());
        in.lookY = Clamp01Axis(reader.ReadFloat());
        in.buttons = reader.ReadU16();
        unsigned short history[3];
        for (unsigned short& h : history) h = reader.ReadU16();
        if (!reader.IsOk()) return;

        // 古い連番は捨てる。新しい連番の分だけ「押した瞬間」を取り込む(取りこぼした分は重ねて送られた履歴から拾う)
        int diff = 1;
        if (g->haveRecvSeq[sender]) diff = (int16_t)(seq - g->lastRecvSeq[sender]);
        if (diff <= 0) return;
        unsigned short pressed = g->inputs[sender].pressed;
        for (int k = 0; k < 3 && k < diff; k++) pressed |= history[k];

        in.pressed = pressed;
        g->inputs[sender] = in;
        g->lastRecvSeq[sender] = seq;
        g->haveRecvSeq[sender] = true;
        g->lastInputTime[sender] = g->session.GetClock();
    }

    // ─── スポーン ───

    NetworkWriter BuildSpawnPacket(const NetworkObjects::SpawnRecord& record, const Transform& transform) {
        NetworkWriter packet = BeginPacket(PacketType::Spawn);
        packet.WriteU8((uint8_t)g->sceneGeneration);
        packet.WriteU16((uint16_t)record.networkId);
        packet.WriteI32(record.prefabId);
        packet.WriteU8((uint8_t)(record.ownerPlayerIndex < 0 ? 255 : record.ownerPlayerIndex));
        packet.WriteFloat(transform.position.x);
        packet.WriteFloat(transform.position.y);
        packet.WriteFloat(transform.position.z);
        packet.WriteFloat(transform.rotate.x);
        packet.WriteFloat(transform.rotate.y);
        packet.WriteFloat(transform.rotate.z);
        return packet;
    }

    void HandleSpawn(NetworkReader& reader) {
        if (!g->localReady || !g->currentScene) return;
        const int generation = reader.ReadU8();
        const int id = reader.ReadU16();
        const int prefabId = reader.ReadI32();
        const int rawOwner = reader.ReadU8();
        NetworkSpawnInfo info;
        info.position = { reader.ReadFloat(), reader.ReadFloat(), reader.ReadFloat() };
        info.rotate = { reader.ReadFloat(), reader.ReadFloat(), reader.ReadFloat() };
        if (!reader.IsOk() || generation != (g->sceneGeneration & 0xFF) || id < Network::firstSpawnId) return;
        if (rawOwner != 255 && rawOwner >= Network::maxPlayers) return;
        info.position.x = (std::max)(-Network::positionLimit, (std::min)(Network::positionLimit, info.position.x));
        info.position.y = (std::max)(-Network::positionLimit, (std::min)(Network::positionLimit, info.position.y));
        info.position.z = (std::max)(-Network::positionLimit, (std::min)(Network::positionLimit, info.position.z));

        if (NetworkObjects::Find(id)) return;   // もう作ってある(途中参加の状態送信と重なった時)
        const NetworkObjects::PrefabFactory* factory = NetworkObjects::FindPrefab(prefabId);
        if (!factory) {
            Log("Spawn: prefab " + std::to_string(prefabId) + " is not registered");
            return;
        }

        info.networkId = id;
        info.prefabId = prefabId;
        info.ownerPlayerIndex = rawOwner == 255 ? -1 : rawOwner;
        info.hasAuthority = false;
        GameObject* object = (*factory)(*g->currentScene, info);
        if (!object) return;

        object->transform.position = info.position;
        object->transform.rotate = info.rotate;
        NetworkIdentityComponent* identity = object->GetComponent<NetworkIdentityComponent>();
        if (!identity) identity = object->AddComponent<NetworkIdentityComponent>();
        identity->SetSpawnInfo(id, prefabId, info.ownerPlayerIndex);

        // ObjectPoolで使い回した物の、前回の位置の写真が残らないようにする
        NetworkTransformComponent* sync = object->GetComponent<NetworkTransformComponent>();
        if (sync) sync->ClearSamples();
    }

    void HandleDespawn(NetworkReader& reader) {
        const int generation = reader.ReadU8();
        const int id = reader.ReadU16();
        if (!reader.IsOk() || generation != (g->sceneGeneration & 0xFF)) return;
        NetworkIdentityComponent* identity = NetworkObjects::Find(id);
        if (!identity || id < Network::firstSpawnId) return;

        GameObject* object = identity->GetOwner();
        const NetworkObjects::DespawnHandler* handler = NetworkObjects::FindDespawnHandler(identity->GetPrefabId());
        NetworkObjects::Unregister(identity);
        if (handler) (*handler)(object);
        else object->Destroy();
    }

    void DespawnInternal(NetworkIdentityComponent* identity) {
        GameObject* object = identity->GetOwner();
        const int id = identity->GetNetworkId();
        const NetworkObjects::DespawnHandler* handler = NetworkObjects::FindDespawnHandler(identity->GetPrefabId());

        NetworkObjects::RemoveSpawnRecord(id, g->time);
        NetworkObjects::Unregister(identity);
        if (handler) (*handler)(object);
        else object->Destroy();

        if (g->session.IsHost()) {
            NetworkWriter packet = BeginPacket(PacketType::Despawn);
            packet.WriteU8((uint8_t)g->sceneGeneration);
            packet.WriteU16((uint16_t)id);
            for (int i = 1; i < Network::maxPlayers; i++) {
                if (g->session.IsPlayerConnected(i) && g->playerReady[i]) g->session.SendTo(i, packet, Network::channelReliable, true);
            }
        }
    }

    // 準備ができたクライアントへ、今のシーンのスポーンした物を全部教える
    void SendStateTo(int playerIndex) {
        for (const NetworkObjects::SpawnRecord& record : NetworkObjects::GetSpawnRecords()) {
            NetworkIdentityComponent* identity = NetworkObjects::Find(record.networkId);
            if (!identity || !identity->GetOwner()) continue;
            g->session.SendTo(playerIndex, BuildSpawnPacket(record, identity->GetOwner()->transform), Network::channelReliable, true);
        }
    }

    // ─── シーン ───

    void CallSceneFunction(int sceneId) {
        auto it = g->scenes.find(sceneId);
        if (it == g->scenes.end()) {
            Log("Scene " + std::to_string(sceneId) + " is not registered (RegisterScene)");
            return;
        }
        g->localReady = false;
        g->pendingSceneId = sceneId;
        g->pendingSceneTimer = 0.f;
        g->pendingSceneTries = 1;
        it->second();
    }

    // ─── パケットの振り分け ───

    void HandlePacket(int sender, PacketType type, NetworkReader& reader) {
        const bool host = g->session.IsHost();
        switch (type) {
        case PacketType::Input:
            HandleInputPacket(sender, reader);
            break;
        case PacketType::Event:
            HandleEventPacket(sender, reader);
            break;
        case PacketType::SharedValue:
            if (!host) {
                const std::string key = reader.ReadString(Network::maxEventNameLength);
                const int value = reader.ReadI32();
                if (!reader.IsOk() || key.empty()) return;
                if (g->shared.size() >= (size_t)Network::maxSharedValues && !g->shared.count(key)) return;
                g->shared[key] = value;
                EventBus::Get().Publish("SharedValueChanged");
            }
            break;
        case PacketType::Snapshot:
            if (!host) HandleSnapshot(reader);
            break;
        case PacketType::Spawn:
            if (!host) HandleSpawn(reader);
            break;
        case PacketType::Despawn:
            if (!host) HandleDespawn(reader);
            break;
        case PacketType::SceneChange:
            if (!host) {
                const int generation = reader.ReadU8();
                const int sceneId = reader.ReadI32();
                if (!reader.IsOk()) return;
                g->sceneGeneration = generation;
                g->localReady = false;
                for (NetworkTransformComponent* sync : NetworkObjects::GetTransformSyncs()) sync->ClearSamples();
                CallSceneFunction(sceneId);
            }
            break;
        case PacketType::SceneReady:
            if (host && sender > 0) {
                const int generation = reader.ReadU8();
                reader.ReadI32();
                if (!reader.IsOk() || generation != (g->sceneGeneration & 0xFF)) return;
                g->playerReady[sender] = true;
                SendStateTo(sender);
            }
            break;
        default:
            break;
        }
    }
}

namespace Network {

void SetGameId(const std::string& gameId) {
    EnsureState();
    g->gameId = gameId;
    g->session.SetGameId(gameId);
    g->discovery.SetGameId(gameId);
}

bool StartHost(const std::string& playerName, int port) {
    EnsureState();
    g->session.SetGameId(g->gameId);
    g->discovery.SetGameId(g->gameId);
    ResetRuntime();
    g->session.sceneGeneration = g->sceneGeneration;
    g->session.sceneId = g->currentSceneId;
    if (!g->session.StartHost(playerName, port)) return false;

    // 同じLANのほかのPCから見つけてもらえるようにする(使えなくても、IP直接入力では参加できる)
    g->discovery.StartResponder(playerName, port);
    g->discovery.SetPlayerCount(1, maxPlayers);
    return true;
}

void StartSearch() {
    EnsureState();
    g->discovery.SetGameId(g->gameId);
    g->discovery.StartSearch();
}

void StopSearch() {
    if (g) g->discovery.StopSearch();
}

bool IsSearching() {
    return g && g->discovery.IsSearching();
}

const std::vector<NetworkHostInfo>& GetFoundHosts() {
    static const std::vector<NetworkHostInfo> empty;
    return g ? g->discovery.GetFoundHosts() : empty;
}

bool Join(const std::string& address, int port, const std::string& playerName) {
    EnsureState();
    g->session.SetGameId(g->gameId);
    ResetRuntime();
    return g->session.Join(address, port, playerName);
}

void Disconnect() {
    if (!g) return;
    const bool wasClient = g->session.IsClient();
    g->discovery.StopSearch();
    g->discovery.StopResponder();
    g->session.Disconnect();
    ResetRuntime();
    g->localInput = NetworkInput();
    if (wasClient) EventBus::Get().Publish("NetworkDisconnected");
}

NetworkState GetState() { return g ? g->session.GetState() : NetworkState::Offline; }
bool IsOnline() { return g && g->session.IsActive(); }
bool IsHost() { return !g || !g->session.IsClient(); }
bool IsClient() { return g && g->session.IsClient(); }
int  GetLocalPlayerIndex() { return g ? g->session.GetLocalPlayerIndex() : 0; }
int  GetPlayerCount() { return g ? g->session.GetPlayerCount() : 1; }
bool IsPlayerConnected(int playerIndex) { return g ? g->session.IsPlayerConnected(playerIndex) : playerIndex == 0; }
std::string GetPlayerName(int playerIndex) { return g ? g->session.GetPlayerName(playerIndex) : std::string(); }
float GetPing() { return g ? g->session.GetPing() : 0.f; }
NetworkStats GetStats() { return g ? g->session.GetStats() : NetworkStats(); }

const std::string& GetLastError() {
    static const std::string empty;
    return g ? g->session.GetLastError() : empty;
}

std::vector<std::string> GetLocalAddresses() {
    return LanDiscovery::GetLocalAddresses();
}

float GetSnapshotAge() {
    return g ? g->snapshotAge : 0.f;
}

// ─── 入力 ───

void SetLocalInput(const NetworkInput& input) {
    EnsureState();
    // 押した瞬間は、前回からボタンが新しく押された分を自動で足す
    NetworkInput in = input;
    in.moveX = Clamp01Axis(in.moveX);
    in.moveY = Clamp01Axis(in.moveY);
    in.lookX = Clamp01Axis(in.lookX);
    in.lookY = Clamp01Axis(in.lookY);
    const unsigned short edge = (unsigned short)(in.buttons & ~g->prevButtons);
    g->prevButtons = in.buttons;
    in.pressed = (unsigned short)(in.pressed | edge);

    g->localPending |= in.pressed;
    g->localInput = in;
    g->inputEverSet = true;
    if (!g->session.IsClient()) g->inputs[0] = in;   // ホストとオフライン: 自分の入力がそのまま GetInput(0) になる
}

const NetworkInput& GetInput(int playerIndex) {
    static const NetworkInput zero;
    if (!g || playerIndex < 0 || playerIndex >= maxPlayers) return zero;
    if (g->session.IsClient()) return playerIndex == g->session.GetLocalPlayerIndex() ? g->localInput : zero;
    return g->inputs[playerIndex];
}

// ─── 出来事・共有の値 ───

void SendEvent(const std::string& name, NetworkTarget target) {
    SendEvent(name, NetworkWriter(), target);
}

void SendEvent(const std::string& name, const NetworkWriter& payload, NetworkTarget target) {
    EnsureState();
    if (name.empty() || name.size() > (size_t)maxEventNameLength || payload.GetSize() > (size_t)maxEventPayloadSize) {
        Log("SendEvent: the name (32 chars) or the payload (512 bytes) is too long");
        return;
    }
    const uint8_t* data = payload.GetData().data();
    const size_t size = payload.GetSize();

    if (g->session.IsClient()) {
        // クライアントはホストに送る(ホストが受け取って、必要なら全員へ中継する)
        g->session.SendToHost(BuildEventPacket(name, data, size, target, g->session.GetLocalPlayerIndex()), channelReliable, true);
        return;
    }

    // ホストとオフライン
    if (target != NetworkTarget::Others) DispatchEvent(name, 0, data, size);
    if (g->session.IsHost() && target != NetworkTarget::Host) {
        g->session.SendToAll(BuildEventPacket(name, data, size, target, 0), channelReliable, true);
    }
}

void Subscribe(const std::string& name, EventHandler handler) {
    EnsureState();
    g->handlers[name].push_back(std::move(handler));
}

void ClearHandlers() {
    if (!g) return;
    g->handlers.clear();
    g->joinedHandler = nullptr;
    g->leftHandler = nullptr;
    g->currentScene = nullptr;
    g->localReady = false;
    NetworkObjects::ClearPrefabs();
}

void SetSharedValue(const std::string& key, int value) {
    EnsureState();
    if (g->session.IsClient()) {
        Log("SetSharedValue: only the host can change shared values");
        return;
    }
    if (key.empty() || key.size() > (size_t)maxEventNameLength) return;
    if (g->shared.size() >= (size_t)maxSharedValues && !g->shared.count(key)) return;

    auto it = g->shared.find(key);
    const bool changed = (it == g->shared.end()) || it->second != value;
    g->shared[key] = value;
    if (!changed) return;

    if (g->session.IsHost()) {
        NetworkWriter packet = BeginPacket(PacketType::SharedValue);
        packet.WriteString(key);
        packet.WriteI32(value);
        g->session.SendToAll(packet, channelReliable, true);
    }
    EventBus::Get().Publish("SharedValueChanged");
}

int GetSharedValue(const std::string& key, int defaultValue) {
    if (!g) return defaultValue;
    auto it = g->shared.find(key);
    return it == g->shared.end() ? defaultValue : it->second;
}

void SetPlayerJoinedHandler(std::function<void(int)> handler) { EnsureState(); g->joinedHandler = std::move(handler); }
void SetPlayerLeftHandler(std::function<void(int)> handler) { EnsureState(); g->leftHandler = std::move(handler); }

// ─── スポーンとシーン ───

void RegisterPrefab(int prefabId, PrefabFactory factory) {
    NetworkObjects::RegisterPrefab(prefabId, std::move(factory));
}

void SetDespawnHandler(int prefabId, std::function<void(GameObject*)> handler) {
    NetworkObjects::SetDespawnHandler(prefabId, std::move(handler));
}

GameObject* Spawn(Scene& scene, int prefabId, int ownerPlayerIndex, DirectX::XMFLOAT3 position) {
    EnsureState();
    if (g->session.IsClient()) {
        Log("Spawn: only the host can spawn");
        return nullptr;
    }
    const NetworkObjects::PrefabFactory* factory = NetworkObjects::FindPrefab(prefabId);
    if (!factory) {
        Log("Spawn: prefab " + std::to_string(prefabId) + " is not registered (RegisterPrefab)");
        return nullptr;
    }
    const int id = NetworkObjects::AllocateSpawnId(g->time);
    if (id == 0) {
        Log("Spawn: no NetworkId is left");
        return nullptr;
    }

    NetworkSpawnInfo info;
    info.networkId = id;
    info.prefabId = prefabId;
    info.ownerPlayerIndex = ownerPlayerIndex;
    info.position = position;
    info.hasAuthority = true;
    GameObject* object = (*factory)(scene, info);
    if (!object) return nullptr;

    object->transform.position = position;
    NetworkIdentityComponent* identity = object->GetComponent<NetworkIdentityComponent>();
    if (!identity) identity = object->AddComponent<NetworkIdentityComponent>();
    identity->SetSpawnInfo(id, prefabId, ownerPlayerIndex);

    const NetworkObjects::SpawnRecord record{ id, prefabId, ownerPlayerIndex };
    NetworkObjects::AddSpawnRecord(record);
    if (g->session.IsHost()) {
        const NetworkWriter packet = BuildSpawnPacket(record, object->transform);
        for (int i = 1; i < maxPlayers; i++) {
            if (g->session.IsPlayerConnected(i) && g->playerReady[i]) g->session.SendTo(i, packet, channelReliable, true);
        }
    }
    return object;
}

void Despawn(GameObject* object) {
    if (!object) return;
    EnsureState();
    if (g->session.IsClient()) {
        Log("Despawn: only the host can despawn");
        return;
    }
    NetworkIdentityComponent* identity = object->GetComponent<NetworkIdentityComponent>();
    if (!identity || identity->GetNetworkId() < firstSpawnId) {
        Log("Despawn: this object was not spawned by Network::Spawn");
        return;
    }
    DespawnInternal(identity);
}

void RegisterScene(int sceneId, std::function<void()> changeScene) {
    EnsureState();
    g->scenes[sceneId] = std::move(changeScene);
}

void ChangeSceneForAll(int sceneId) {
    EnsureState();
    if (g->session.IsClient()) {
        Log("ChangeSceneForAll: only the host can change the scene");
        return;
    }
    g->sceneGeneration = (g->sceneGeneration + 1) & 0xFF;
    g->session.sceneGeneration = g->sceneGeneration;
    g->session.sceneId = sceneId;
    for (bool& ready : g->playerReady) ready = false;
    NetworkObjects::ClearSpawnRecords();

    if (g->session.IsHost()) {
        NetworkWriter packet = BeginPacket(PacketType::SceneChange);
        packet.WriteU8((uint8_t)g->sceneGeneration);
        packet.WriteI32(sceneId);
        g->session.SendToAll(packet, channelReliable, true);
    }
    CallSceneFunction(sceneId);
}

void NotifySceneReady(Scene& scene, int sceneId) {
    EnsureState();
    g->currentScene = &scene;
    g->currentSceneId = sceneId;
    g->localReady = true;
    if (g->pendingSceneId == sceneId) g->pendingSceneId = -1;

    if (g->session.IsHost()) {
        g->session.sceneId = sceneId;
    }
    else if (g->session.GetState() == NetworkState::Connected) {
        NetworkWriter packet = BeginPacket(PacketType::SceneReady);
        packet.WriteU8((uint8_t)g->sceneGeneration);
        packet.WriteI32(sceneId);
        g->session.SendToHost(packet, channelReliable, true);
    }
}

// ─── 調整 ───

void SetSnapshotRate(int hz) { EnsureState(); g->snapshotRate = (std::max)(10, (std::min)(60, hz)); }
void SetInterpolationDelay(float seconds) { EnsureState(); g->interpolationDelay = (std::max)(0.f, seconds); }
void SetDebugLogEnabled(bool enabled) { EnsureState(); g->debugLog = enabled; }

// ─── Engine内部用 ───

void Update() {
    if (!g) return;   // StartHost / Join / StartSearch などを一度も呼んでいなければ何もしない
    const float dt = Time::GetUnscaledDeltaTime();
    g->time += dt;

    const bool active = g->session.IsActive();
    if (!active && !g->discovery.IsSearching() && !g->discovery.IsResponding()) return;

    g->session.Update(dt);
    if (g->discovery.IsResponding()) g->discovery.SetPlayerCount(g->session.GetPlayerCount(), maxPlayers);
    g->discovery.Update(dt);

    if (g->session.IsHost()) {
        // 入力が途絶えたプレイヤーは、入力をゼロに戻す(ボタンを押しっぱなしのまま止まらないように)
        for (int i = 1; i < maxPlayers; i++) {
            if (g->session.IsPlayerConnected(i) && g->session.GetClock() - g->lastInputTime[i] > inputTimeout) g->inputs[i] = NetworkInput();
        }
    }
    else if (g->session.GetState() == NetworkState::Connected) {
        g->snapshotAge += dt;

        // ホストに言われたシーンへの切り替えが終わらなければ、もう一度頼む(SceneManager は切り替え中の頼みを無視するため)
        if (g->pendingSceneId >= 0) {
            g->pendingSceneTimer += dt;
            if (g->pendingSceneTimer > sceneChangeRetryTime) {
                if (g->pendingSceneTries >= sceneChangeMaxTries) {
                    g->pendingSceneId = -1;
                }
                else {
                    auto it = g->scenes.find(g->pendingSceneId);
                    g->pendingSceneTimer = 0.f;
                    g->pendingSceneTries++;
                    if (it != g->scenes.end()) it->second();
                }
            }
        }

        // 受け取った位置を、少し遅れた時刻で補間して Transform に書く(Physics::Update の前に書く)
        if (g->haveEstimate) {
            g->estHostTime += dt;
            const double renderTime = g->estHostTime - g->interpolationDelay;
            for (NetworkTransformComponent* sync : NetworkObjects::GetTransformSyncs()) {
                if (sync->GetOwner() && sync->GetOwner()->GetIsActive()) sync->Interpolate(renderTime);
            }
        }
    }
}

void EndFrame() {
    if (!g || !g->session.IsActive()) return;
    const float dt = Time::GetUnscaledDeltaTime();

    if (g->session.IsHost()) {
        // 位置のスナップショットを一定の間隔で送る
        g->snapshotTimer += dt;
        const float interval = 1.f / (float)g->snapshotRate;
        if (g->snapshotTimer >= interval) {
            g->snapshotTimer -= interval;
            if (g->snapshotTimer > interval) g->snapshotTimer = 0.f;
            SendSnapshots();
        }
        // 「押した瞬間」は1フレームだけ立つ(このフレームの Update で読み終わったので消す)
        for (int i = 1; i < maxPlayers; i++) g->inputs[i].pressed = 0;
    }
    else if (g->session.GetState() == NetworkState::Connected && g->inputEverSet) {
        // 入力を一定の間隔で送る(信頼しない通信。「押した瞬間」は直近3回分を重ねて入れる)
        g->inputTimer += dt;
        const float interval = 1.f / (float)inputRate;
        if (g->inputTimer >= interval) {
            g->inputTimer -= interval;
            if (g->inputTimer > interval) g->inputTimer = 0.f;

            g->pressedHistory[2] = g->pressedHistory[1];
            g->pressedHistory[1] = g->pressedHistory[0];
            g->pressedHistory[0] = g->localPending;
            g->localPending = 0;

            NetworkWriter packet = BeginPacket(PacketType::Input);
            packet.WriteU16(++g->sendSeq);
            packet.WriteFloat(g->localInput.moveX);
            packet.WriteFloat(g->localInput.moveY);
            packet.WriteFloat(g->localInput.lookX);
            packet.WriteFloat(g->localInput.lookY);
            packet.WriteU16(g->localInput.buttons);
            for (unsigned short history : g->pressedHistory) packet.WriteU16(history);
            g->session.SendToHost(packet, channelInput, false);
        }
    }

    g->session.Flush();   // 溜めた送信をすぐ流す
}

void Shutdown() {
    if (!g) return;
    g->discovery.StopSearch();
    g->discovery.StopResponder();
    g->session.Disconnect();
    NetworkObjects::ClearAll();
    delete g;
    g = nullptr;
}

}
