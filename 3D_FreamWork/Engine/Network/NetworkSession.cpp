#include "NetworkSession.h"
#include <cstdio>

NetworkSession::NetworkSession() {}

// ─── 開始と終了 ───

bool NetworkSession::StartHost(const std::string& playerName, int port) {
    Disconnect();
    lastError.clear();
    // 満員のとき断りの返事を送れるように、最大人数より少し多く接続を受け付ける
    if (!transport.StartServer(port, Network::maxPlayers + 4)) {
        lastError = "Cannot open port " + std::to_string(port) + " (already in use?)";
        return false;
    }
    state = NetworkState::Hosting;
    localIndex = 0;
    clock = 0.0;
    localName = playerName.substr(0, Network::maxNameLength);
    players[0].connected = true;
    players[0].name = localName;
    players[0].peerId = -1;
    return true;
}

bool NetworkSession::Join(const std::string& address, int port, const std::string& playerName) {
    Disconnect();
    lastError.clear();
    if (!transport.StartClient()) {
        lastError = "Cannot start the network";
        return false;
    }
    hostPeerId = transport.Connect(address, port);
    if (hostPeerId < 0) {
        transport.Stop();
        lastError = "Invalid address: " + address;
        return false;
    }
    state = NetworkState::Connecting;
    localName = playerName.substr(0, Network::maxNameLength);
    connectTimer = 0.f;
    pingTimer = 0.f;
    ping = 0.f;
    clock = 0.0;
    localIndex = 0;
    return true;
}

void NetworkSession::Disconnect() {
    if (state == NetworkState::Offline && !transport.IsActive()) return;

    // クライアントは、抜けることをホストに知らせる(Stop が切断の通知も出すが、念のため)
    if (state == NetworkState::Connected) {
        transport.Send(hostPeerId, Network::channelReliable, BeginPacket(PacketType::Leave), true);
    }
    transport.Stop();
    state = NetworkState::Offline;
    for (Player& player : players) player = Player();
    pending.clear();
    hostPeerId = -1;
    localIndex = 0;
    ping = 0.f;
}

// ─── 毎フレーム ───

void NetworkSession::Update(float dt) {
    if (!transport.IsActive()) return;
    clock += dt;
    transport.UpdateStats(dt);

    NetworkTransport::Event event;
    while (transport.IsActive() && transport.Poll(event)) {
        switch (event.type) {
        case NetworkTransport::EventType::Connect:    HandleConnect(event.peerId); break;
        case NetworkTransport::EventType::Disconnect: HandleDisconnect(event.peerId); break;
        case NetworkTransport::EventType::Receive:    HandleReceive(event.peerId, event.data); break;
        }
    }
    if (!transport.IsActive()) return;

    if (state == NetworkState::Hosting) {
        // Join が来ない相手は切る(つないだだけで放置されたものを溜めない)
        for (auto it = pending.begin(); it != pending.end();) {
            it->second += dt;
            if (it->second > Network::joinTimeout) {
                transport.DisconnectPeer(it->first);
                it = pending.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    else if (state == NetworkState::Connecting) {
        connectTimer += dt;
        if (connectTimer > Network::joinTimeout + 3.f) LoseConnection("Connection timed out");
    }
    else if (state == NetworkState::Connected) {
        // 1秒に1回、pingを送る
        pingTimer += dt;
        if (pingTimer >= Network::pingInterval) {
            pingTimer = 0.f;
            NetworkWriter packet = BeginPacket(PacketType::Ping);
            packet.WriteDouble(clock);
            transport.Send(hostPeerId, Network::channelInput, packet, false);
        }
    }
}

// ─── 受信 ───

void NetworkSession::HandleConnect(int peerId) {
    if (state == NetworkState::Hosting) {
        pending[peerId] = 0.f;
    }
    else if (state == NetworkState::Connecting && peerId == hostPeerId) {
        // つながったので、参加を申し込む
        NetworkWriter packet = BeginPacket(PacketType::Join);
        packet.WriteU16((uint16_t)Network::protocolVersion);
        packet.WriteString(gameId);
        packet.WriteString(localName);
        transport.Send(hostPeerId, Network::channelReliable, packet, true);
    }
}

void NetworkSession::HandleDisconnect(int peerId) {
    if (state == NetworkState::Hosting) {
        pending.erase(peerId);
        const int index = FindPlayerByPeer(peerId);
        if (index > 0) RemovePlayer(index);
    }
    else if (peerId == hostPeerId && state != NetworkState::Offline) {
        LoseConnection(state == NetworkState::Connecting ? "Cannot connect to the host" : "Disconnected from the host");
    }
}

void NetworkSession::HandleReceive(int peerId, const std::vector<uint8_t>& data) {
    // 大きすぎる・空のパケットは捨てる
    if (data.empty() || data.size() > (size_t)Network::maxPacketSize) return;

    NetworkReader reader(data);
    const uint8_t rawType = reader.ReadU8();
    if (rawType == 0 || rawType >= (uint8_t)PacketType::Count_) return;   // 知らない種類
    const PacketType type = (PacketType)rawType;

    // 送り主のプレイヤー番号は、パケットの中身ではなく接続から決める(偽装されないように)
    int sender = 0;
    if (state == NetworkState::Hosting) {
        sender = FindPlayerByPeer(peerId);
        if (onTrace) onTrace(type, sender, true);
        if (sender < 0) {
            // まだ参加していない相手が送っていいのは Join だけ
            if (type == PacketType::Join && pending.count(peerId)) HandleJoin(peerId, reader);
            return;
        }
    }
    else if (state == NetworkState::Connecting || state == NetworkState::Connected) {
        if (peerId != hostPeerId) return;
        if (onTrace) onTrace(type, 0, true);
    }
    else {
        return;
    }

    switch (type) {
    case PacketType::Join:
        // 参加済みの相手の2回目の Join は無視する
        return;
    case PacketType::Leave:
        if (state == NetworkState::Hosting && sender > 0) {
            transport.DisconnectPeer(peerId);
            RemovePlayer(sender);
        }
        return;
    case PacketType::Ping:
        if (state == NetworkState::Hosting) {
            const double sentTime = reader.ReadDouble();
            if (!reader.IsOk()) return;
            NetworkWriter reply = BeginPacket(PacketType::Pong);
            reply.WriteDouble(sentTime);
            transport.Send(peerId, Network::channelInput, reply, false);
        }
        return;
    case PacketType::Pong:
        if (state == NetworkState::Connected) {
            const double sentTime = reader.ReadDouble();
            if (!reader.IsOk()) return;
            const float rtt = (float)(clock - sentTime);
            if (rtt >= 0.f && rtt < 60.f) ping = (ping <= 0.f) ? rtt : ping * 0.5f + rtt * 0.5f;
        }
        return;
    case PacketType::Accept:
        if (state == NetworkState::Connecting) HandleAccept(reader);
        return;
    case PacketType::Reject:
        if (state == NetworkState::Connecting) HandleReject(reader);
        return;
    case PacketType::PlayerJoined:
        if (state == NetworkState::Connected) {
            const int index = reader.ReadU8();
            const std::string name = reader.ReadString(Network::maxNameLength);
            if (!reader.IsOk() || index <= 0 || index >= Network::maxPlayers) return;
            players[index].connected = true;
            players[index].name = name;
            if (onPlayerJoined) onPlayerJoined(index);
        }
        return;
    case PacketType::PlayerLeft:
        if (state == NetworkState::Connected) {
            const int index = reader.ReadU8();
            if (!reader.IsOk() || index <= 0 || index >= Network::maxPlayers) return;
            players[index] = Player();
            if (onPlayerLeft) onPlayerLeft(index);
        }
        return;
    default:
        break;
    }

    // ここから先は、参加済みの相手とのゲームの中身のパケット(上の層に渡す)
    if (state == NetworkState::Connecting) return;   // まだ参加が許可されていない
    if (onPacket) onPacket(sender, type, reader);
}

void NetworkSession::HandleJoin(int peerId, NetworkReader& reader) {
    const int protocol = reader.ReadU16();
    const std::string theirGameId = reader.ReadString(64);
    const std::string name = reader.ReadString(Network::maxNameLength);

    auto reject = [&](RejectReason reason) {
        NetworkWriter packet = BeginPacket(PacketType::Reject);
        packet.WriteU8((uint8_t)reason);
        transport.Send(peerId, Network::channelReliable, packet, true);
        transport.DisconnectPeer(peerId);   // 断りの返事を流してから切る
        pending.erase(peerId);
        if (onTrace) onTrace(PacketType::Reject, -1, false);
    };

    if (!reader.IsOk()) return reject(RejectReason::Invalid);
    if (protocol != Network::protocolVersion) return reject(RejectReason::VersionMismatch);
    if (theirGameId != gameId) return reject(RejectReason::GameIdMismatch);

    // 一番若い空き番号を渡す
    int slot = -1;
    for (int i = 1; i < Network::maxPlayers; i++) {
        if (!players[i].connected) { slot = i; break; }
    }
    if (slot < 0) return reject(RejectReason::Full);

    pending.erase(peerId);
    players[slot].connected = true;
    players[slot].name = name.empty() ? ("Player" + std::to_string(slot + 1)) : name;
    players[slot].peerId = peerId;

    // 許可の返事: 自分の番号・最大人数・ホストの時刻・今のシーン・名簿
    NetworkWriter accept = BeginPacket(PacketType::Accept);
    accept.WriteU8((uint8_t)slot);
    accept.WriteU8((uint8_t)Network::maxPlayers);
    accept.WriteDouble(clock);
    accept.WriteU8((uint8_t)sceneGeneration);
    accept.WriteI32(sceneId);
    int count = 0;
    for (const Player& player : players) if (player.connected) count++;
    accept.WriteU8((uint8_t)count);
    for (int i = 0; i < Network::maxPlayers; i++) {
        if (!players[i].connected) continue;
        accept.WriteU8((uint8_t)i);
        accept.WriteString(players[i].name);
    }
    transport.Send(peerId, Network::channelReliable, accept, true);
    if (onTrace) onTrace(PacketType::Accept, slot, false);

    // ほかの参加者に知らせる
    NetworkWriter joined = BeginPacket(PacketType::PlayerJoined);
    joined.WriteU8((uint8_t)slot);
    joined.WriteString(players[slot].name);
    SendToAll(joined, Network::channelReliable, true, slot);

    if (onPlayerJoined) onPlayerJoined(slot);
    if (onClientAccepted) onClientAccepted(slot);
}

void NetworkSession::HandleAccept(NetworkReader& reader) {
    const int index = reader.ReadU8();
    const int maxCount = reader.ReadU8();
    const double hostClock = reader.ReadDouble();
    const int generation = reader.ReadU8();
    const int hostSceneId = reader.ReadI32();
    const int count = reader.ReadU8();
    if (!reader.IsOk() || index <= 0 || index >= Network::maxPlayers || count > Network::maxPlayers) {
        LoseConnection("Invalid reply from the host");
        return;
    }
    (void)maxCount;
    (void)hostClock;

    Player roster[Network::maxPlayers];
    for (int i = 0; i < count; i++) {
        const int number = reader.ReadU8();
        const std::string name = reader.ReadString(Network::maxNameLength);
        if (!reader.IsOk() || number >= Network::maxPlayers) {
            LoseConnection("Invalid reply from the host");
            return;
        }
        roster[number].connected = true;
        roster[number].name = name;
    }
    for (int i = 0; i < Network::maxPlayers; i++) players[i] = roster[i];
    players[0].peerId = hostPeerId;
    localIndex = index;
    sceneGeneration = generation;
    sceneId = hostSceneId;
    state = NetworkState::Connected;
    pingTimer = Network::pingInterval;   // すぐ最初のpingを送る
    if (onConnected) onConnected();
}

void NetworkSession::HandleReject(NetworkReader& reader) {
    const RejectReason reason = (RejectReason)reader.ReadU8();
    lastError = std::string("Rejected: ") + GetRejectReasonText(reason);
    LoseConnection(lastError);
}

void NetworkSession::RemovePlayer(int index) {
    if (index <= 0 || index >= Network::maxPlayers || !players[index].connected) return;
    players[index] = Player();

    NetworkWriter packet = BeginPacket(PacketType::PlayerLeft);
    packet.WriteU8((uint8_t)index);
    SendToAll(packet, Network::channelReliable, true);

    if (onPlayerLeft) onPlayerLeft(index);
}

void NetworkSession::LoseConnection(const std::string& reason) {
    if (lastError.empty() || reason.rfind("Rejected", 0) == 0) lastError = reason;
    transport.Stop();
    state = NetworkState::Offline;
    for (Player& player : players) player = Player();
    hostPeerId = -1;
    localIndex = 0;
    ping = 0.f;
    if (onDisconnected) onDisconnected();
}

int NetworkSession::FindPlayerByPeer(int peerId) const {
    for (int i = 1; i < Network::maxPlayers; i++) {
        if (players[i].connected && players[i].peerId == peerId) return i;
    }
    return -1;
}

// ─── 状態 ───

int NetworkSession::GetPlayerCount() const {
    if (state == NetworkState::Offline) return 1;
    int count = 0;
    for (const Player& player : players) if (player.connected) count++;
    return count;
}

bool NetworkSession::IsPlayerConnected(int index) const {
    if (index < 0 || index >= Network::maxPlayers) return false;
    if (state == NetworkState::Offline) return index == 0;
    return players[index].connected;
}

std::string NetworkSession::GetPlayerName(int index) const {
    if (index < 0 || index >= Network::maxPlayers) return std::string();
    return players[index].name;
}

int NetworkSession::GetPlayerPeerId(int index) const {
    if (index < 0 || index >= Network::maxPlayers) return -1;
    return players[index].peerId;
}

// ─── 送信 ───

bool NetworkSession::SendTo(int playerIndex, const NetworkWriter& packet, int channel, bool reliable) {
    if (state == NetworkState::Hosting) {
        if (playerIndex <= 0 || playerIndex >= Network::maxPlayers || !players[playerIndex].connected) return false;
        if (onTrace && !packet.IsEmpty()) onTrace((PacketType)packet.GetData()[0], playerIndex, false);
        return transport.Send(players[playerIndex].peerId, channel, packet, reliable);
    }
    return SendToHost(packet, channel, reliable);
}

void NetworkSession::SendToAll(const NetworkWriter& packet, int channel, bool reliable, int exceptPlayer) {
    if (state != NetworkState::Hosting) return;
    for (int i = 1; i < Network::maxPlayers; i++) {
        if (i == exceptPlayer || !players[i].connected) continue;
        SendTo(i, packet, channel, reliable);
    }
}

bool NetworkSession::SendToHost(const NetworkWriter& packet, int channel, bool reliable) {
    if (state != NetworkState::Connected) return false;
    if (onTrace && !packet.IsEmpty()) onTrace((PacketType)packet.GetData()[0], 0, false);
    return transport.Send(hostPeerId, channel, packet, reliable);
}
