#include "enet/enet.h"   // ENetは winsock2.h を読み込むので、windows.h より先(このファイルの最初)に読む
#include "NetworkTransport.h"
#include "NetworkConfig.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

namespace {
    int enetUsers = 0;   // ENetを初期化している NetworkTransport / LanDiscovery の数(0になったら終了処理)
}

// ENetの初期化と終了(何回呼ばれても1回だけ初期化する)
bool NetworkAcquireEnet() {
    if (enetUsers == 0 && enet_initialize() != 0) return false;
    enetUsers++;
    return true;
}

void NetworkReleaseEnet() {
    if (enetUsers <= 0) return;
    enetUsers--;
    if (enetUsers == 0) enet_deinitialize();
}

NetworkTransport::NetworkTransport() : timeoutMinMs(Network::timeoutMinMs), timeoutMaxMs(Network::timeoutMaxMs) {}

NetworkTransport::~NetworkTransport() {
    Stop();
}

ENetPeer* NetworkTransport::GetPeer(int peerId) const {
    if (!host || peerId < 0 || (size_t)peerId >= host->peerCount) return nullptr;
    return &host->peers[peerId];
}

bool NetworkTransport::StartServer(int port, int maxPeers) {
    Stop();
    if (port <= 0 || port > 65535 || maxPeers <= 0) return false;
    if (!NetworkAcquireEnet()) return false;
    holdsEnet = true;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = (enet_uint16)port;
    host = enet_host_create(&address, (size_t)maxPeers, Network::channelCount, 0, 0);
    if (!host) {
        Stop();
        return false;
    }
    isServer = true;
    return true;
}

bool NetworkTransport::StartClient() {
    Stop();
    if (!NetworkAcquireEnet()) return false;
    holdsEnet = true;

    host = enet_host_create(nullptr, 1, Network::channelCount, 0, 0);
    if (!host) {
        Stop();
        return false;
    }
    isServer = false;
    return true;
}

int NetworkTransport::Connect(const std::string& addressText, int port) {
    if (!host || port <= 0 || port > 65535 || addressText.empty()) return -1;

    ENetAddress address;
    if (enet_address_set_host_ip(&address, addressText.c_str()) != 0 &&
        enet_address_set_host(&address, addressText.c_str()) != 0) {
        return -1;   // 数字のIPでも名前でもない
    }
    address.port = (enet_uint16)port;

    ENetPeer* peer = enet_host_connect(host, &address, Network::channelCount, 0);
    if (!peer) return -1;
    enet_peer_timeout(peer, 0, (enet_uint32)timeoutMinMs, (enet_uint32)timeoutMaxMs);
    return (int)(peer - host->peers);
}

void NetworkTransport::Stop() {
    if (host) {
        // 繋がっている相手全員に切断を知らせる。返事を最大0.2秒だけ待つ
        bool any = false;
        for (size_t i = 0; i < host->peerCount; i++) {
            ENetPeer* peer = &host->peers[i];
            if (peer->state == ENET_PEER_STATE_CONNECTED) {
                enet_peer_disconnect(peer, 0);
                any = true;
            }
            else if (peer->state != ENET_PEER_STATE_DISCONNECTED) {
                enet_peer_reset(peer);   // 接続の途中の相手は、待たずに捨てる
            }
        }
        if (any) {
            ENetEvent event;
            for (int waited = 0; waited < 200; waited += 10) {
                bool allDone = true;
                while (enet_host_service(host, &event, waited == 0 ? 0 : 10) > 0) {
                    if (event.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(event.packet);
                }
                for (size_t i = 0; i < host->peerCount; i++) {
                    if (host->peers[i].state != ENET_PEER_STATE_DISCONNECTED) allDone = false;
                }
                if (allDone) break;
            }
        }
        enet_host_flush(host);
        enet_host_destroy(host);
        host = nullptr;
    }
    if (holdsEnet) {
        NetworkReleaseEnet();
        holdsEnet = false;
    }
    isServer = false;
    stats = NetworkStats();
    lastSentData = lastReceivedData = lastSentPackets = lastReceivedPackets = 0;
    statsTimer = 0.f;
}

bool NetworkTransport::Poll(Event& out) {
    if (!host) return false;

    ENetEvent event;
    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            out.type = EventType::Connect;
            out.peerId = (int)(event.peer - host->peers);
            out.channel = 0;
            out.data.clear();
            enet_peer_timeout(event.peer, 0, (enet_uint32)timeoutMinMs, (enet_uint32)timeoutMaxMs);
            return true;
        case ENET_EVENT_TYPE_DISCONNECT:
            out.type = EventType::Disconnect;
            out.peerId = (int)(event.peer - host->peers);
            out.channel = 0;
            out.data.clear();
            return true;
        case ENET_EVENT_TYPE_RECEIVE:
            out.type = EventType::Receive;
            out.peerId = (int)(event.peer - host->peers);
            out.channel = event.channelID;
            out.data.assign(event.packet->data, event.packet->data + event.packet->dataLength);
            enet_packet_destroy(event.packet);
            return true;
        default:
            break;
        }
    }
    return false;
}

bool NetworkTransport::Send(int peerId, int channel, const void* data, size_t size, bool reliable) {
    ENetPeer* peer = GetPeer(peerId);
    if (!peer || peer->state != ENET_PEER_STATE_CONNECTED || !data || size == 0) return false;
    if (channel < 0 || channel >= Network::channelCount) return false;

    // 信頼する通信は届くまで送り直す。信頼しない通信(flags=0)は、古い物が新しい物の後に届いたら捨てられる
    ENetPacket* packet = enet_packet_create(data, size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    if (!packet) return false;
    if (enet_peer_send(peer, (enet_uint8)channel, packet) != 0) {
        enet_packet_destroy(packet);
        return false;
    }
    return true;
}

void NetworkTransport::Flush() {
    if (host) enet_host_flush(host);
}

void NetworkTransport::DisconnectPeer(int peerId) {
    ENetPeer* peer = GetPeer(peerId);
    if (!peer) return;
    if (peer->state == ENET_PEER_STATE_CONNECTED) enet_peer_disconnect_later(peer, 0);
    else if (peer->state != ENET_PEER_STATE_DISCONNECTED) enet_peer_reset(peer);
}

void NetworkTransport::SetTimeouts(int minMs, int maxMs) {
    timeoutMinMs = minMs;
    timeoutMaxMs = maxMs;
    if (!host) return;
    for (size_t i = 0; i < host->peerCount; i++) {
        enet_peer_timeout(&host->peers[i], 0, (enet_uint32)minMs, (enet_uint32)maxMs);
    }
}

float NetworkTransport::GetPeerRoundTrip(int peerId) const {
    ENetPeer* peer = GetPeer(peerId);
    return peer ? (float)peer->roundTripTime * 0.001f : 0.f;
}

std::string NetworkTransport::GetPeerAddress(int peerId) const {
    ENetPeer* peer = GetPeer(peerId);
    if (!peer) return std::string();
    char text[64] = {};
    if (enet_address_get_host_ip(&peer->address, text, sizeof(text)) != 0) return std::string();
    return text;
}

bool NetworkTransport::IsPeerConnected(int peerId) const {
    ENetPeer* peer = GetPeer(peerId);
    return peer && peer->state == ENET_PEER_STATE_CONNECTED;
}

void NetworkTransport::UpdateStats(float dt) {
    if (!host) return;
    statsTimer += dt;
    if (statsTimer < 1.f) return;
    statsTimer -= 1.f;
    if (statsTimer > 1.f) statsTimer = 0.f;

    // ENetが数えている合計の、1秒前との差(32ビットが一周しても差は正しく出る)
    stats.bytesSent = (int)(host->totalSentData - lastSentData);
    stats.bytesReceived = (int)(host->totalReceivedData - lastReceivedData);
    stats.packetsSent = (int)(host->totalSentPackets - lastSentPackets);
    stats.packetsReceived = (int)(host->totalReceivedPackets - lastReceivedPackets);
    lastSentData = host->totalSentData;
    lastReceivedData = host->totalReceivedData;
    lastSentPackets = host->totalSentPackets;
    lastReceivedPackets = host->totalReceivedPackets;
}
