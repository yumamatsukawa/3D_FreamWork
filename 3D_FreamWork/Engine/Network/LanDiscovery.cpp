#include "enet/enet.h"   // ENetは winsock2.h を読み込むので、windows.h より先(このファイルの最初)に読む
#include "LanDiscovery.h"
#include "NetworkConfig.h"
#include "NetworkTransport.h"
#include <iphlpapi.h>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace {
    constexpr uint8_t magic[4] = { '3', 'D', 'F', 'W' };   // 呼びかけ・返事の目印
    constexpr uint8_t kindQuery = 1;
    constexpr uint8_t kindReply = 2;
    constexpr float hostLifetime = 4.f;                    // 返事が来なくなったホストを一覧から消すまでの秒数

    // このPCのIPv4アドレスと、そのネットワークのブロードキャスト先
    struct Adapter {
        uint32_t ip;          // ネットワークバイトオーダー
        uint32_t broadcast;
    };

    std::vector<Adapter> CollectAdapters() {
        std::vector<Adapter> result;
        ULONG size = 16 * 1024;
        std::vector<char> buffer;
        ULONG error = ERROR_BUFFER_OVERFLOW;
        for (int tries = 0; tries < 3 && error == ERROR_BUFFER_OVERFLOW; tries++) {
            buffer.assign(size, 0);
            error = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
        }
        if (error != NO_ERROR) return result;

        for (IP_ADAPTER_ADDRESSES* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()); adapter; adapter = adapter->Next) {
            if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapter->OperStatus != IfOperStatusUp) continue;
            for (IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next) {
                if (!unicast->Address.lpSockaddr || unicast->Address.lpSockaddr->sa_family != AF_INET) continue;
                const sockaddr_in* sin = reinterpret_cast<const sockaddr_in*>(unicast->Address.lpSockaddr);
                const uint32_t ip = sin->sin_addr.s_addr;
                if ((ntohl(ip) >> 24) == 169) continue;   // 169.254.x.x はつながっていない時の仮のアドレス
                const int prefix = (std::min)((int)unicast->OnLinkPrefixLength, 32);
                const uint32_t mask = prefix == 0 ? 0u : htonl(0xFFFFFFFFu << (32 - prefix));
                result.push_back({ ip, ip | ~mask });
            }
        }
        return result;
    }

    // 受信したパケットが、目印・種類・プロトコル番号・ゲームIDまで合っているか読む(合わなければ false)
    bool ReadHeader(NetworkReader& reader, uint8_t expectedKind, const std::string& gameId) {
        for (uint8_t m : magic) {
            if (reader.ReadU8() != m) return false;
        }
        if (reader.ReadU8() != expectedKind) return false;
        if (reader.ReadU16() != (uint16_t)Network::protocolVersion) return false;
        const std::string theirGameId = reader.ReadString(64);
        return reader.IsOk() && theirGameId == gameId;
    }

    NetworkWriter MakeHeader(uint8_t kind, const std::string& gameId) {
        NetworkWriter writer;
        for (uint8_t m : magic) writer.WriteU8(m);
        writer.WriteU8(kind);
        writer.WriteU16((uint16_t)Network::protocolVersion);
        writer.WriteString(gameId);
        return writer;
    }

    // 1つのソケットでデータを送る
    void SendTo(ENetSocket socket, uint32_t host, int port, const NetworkWriter& writer) {
        ENetAddress address;
        address.host = host;
        address.port = (enet_uint16)port;
        ENetBuffer buffer;
        buffer.data = const_cast<uint8_t*>(writer.GetData().data());
        buffer.dataLength = writer.GetSize();
        enet_socket_send(socket, &address, &buffer, 1);
    }

    ENetSocket CreateSocket(int bindPort, bool broadcast) {
        ENetSocket socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
        if (socket == ENET_SOCKET_NULL) return ENET_SOCKET_NULL;
        enet_socket_set_option(socket, ENET_SOCKOPT_NONBLOCK, 1);
        if (broadcast) enet_socket_set_option(socket, ENET_SOCKOPT_BROADCAST, 1);

        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = (enet_uint16)bindPort;
        if (enet_socket_bind(socket, &address) < 0) {
            enet_socket_destroy(socket);
            return ENET_SOCKET_NULL;
        }
        return socket;
    }
}

LanDiscovery::LanDiscovery() : responderSocket((unsigned long long)ENET_SOCKET_NULL), searchSocket((unsigned long long)ENET_SOCKET_NULL) {}

LanDiscovery::~LanDiscovery() {
    CloseSockets();
}

void LanDiscovery::CloseSockets() {
    if (responderSocket != (unsigned long long)ENET_SOCKET_NULL) enet_socket_destroy((ENetSocket)responderSocket);
    if (searchSocket != (unsigned long long)ENET_SOCKET_NULL) enet_socket_destroy((ENetSocket)searchSocket);
    responderSocket = searchSocket = (unsigned long long)ENET_SOCKET_NULL;
    responding = searching = false;
    if (holdsEnet) {
        NetworkReleaseEnet();
        holdsEnet = false;
    }
}

bool LanDiscovery::StartResponder(const std::string& name, int port, int portOfDiscovery) {
    StopResponder();
    if (!holdsEnet) {
        if (!NetworkAcquireEnet()) return false;
        holdsEnet = true;
    }
    hostName = name;
    gamePort = port;
    discoveryPortNumber = portOfDiscovery > 0 ? portOfDiscovery : Network::discoveryPort;

    // 同じ探索用ポートを待ち受けられるのは1つのプロセスだけ(2つ目は探す側として動く)
    ENetSocket socket = CreateSocket(discoveryPortNumber, false);
    if (socket == ENET_SOCKET_NULL) {
        if (!searching && holdsEnet) { NetworkReleaseEnet(); holdsEnet = false; }
        return false;
    }
    responderSocket = (unsigned long long)socket;
    responding = true;
    return true;
}

void LanDiscovery::StopResponder() {
    if (responderSocket != (unsigned long long)ENET_SOCKET_NULL) enet_socket_destroy((ENetSocket)responderSocket);
    responderSocket = (unsigned long long)ENET_SOCKET_NULL;
    responding = false;
    if (!searching && holdsEnet) {
        NetworkReleaseEnet();
        holdsEnet = false;
    }
}

bool LanDiscovery::StartSearch(int portOfDiscovery) {
    StopSearch();
    if (!holdsEnet) {
        if (!NetworkAcquireEnet()) return false;
        holdsEnet = true;
    }
    discoveryPortNumber = portOfDiscovery > 0 ? portOfDiscovery : Network::discoveryPort;

    ENetSocket socket = CreateSocket(0, true);   // 返事を受け取るポートは何番でもよい
    if (socket == ENET_SOCKET_NULL) {
        if (!responding && holdsEnet) { NetworkReleaseEnet(); holdsEnet = false; }
        return false;
    }
    searchSocket = (unsigned long long)socket;
    searching = true;
    queryTimer = 1.f;   // すぐ最初の呼びかけを送る
    found.clear();
    foundInfos.clear();
    return true;
}

void LanDiscovery::StopSearch() {
    if (searchSocket != (unsigned long long)ENET_SOCKET_NULL) enet_socket_destroy((ENetSocket)searchSocket);
    searchSocket = (unsigned long long)ENET_SOCKET_NULL;
    searching = false;
    found.clear();
    foundInfos.clear();
    if (!responding && holdsEnet) {
        NetworkReleaseEnet();
        holdsEnet = false;
    }
}

void LanDiscovery::Update(float dt) {
    if (responding) ReceiveQueries();

    if (searching) {
        queryTimer += dt;
        if (queryTimer >= 1.f) {
            queryTimer = 0.f;
            SendQueries();
        }
        ReceiveReplies();

        // 返事が来なくなったホストを消す
        for (Found& item : found) item.age += dt;
        found.erase(std::remove_if(found.begin(), found.end(), [](const Found& item) { return item.age > hostLifetime; }), found.end());
        foundInfos.clear();
        for (const Found& item : found) foundInfos.push_back(item.info);
    }
}

void LanDiscovery::SendQueries() {
    const NetworkWriter query = MakeHeader(kindQuery, gameId);
    ENetSocket socket = (ENetSocket)searchSocket;

    // 全体向け、それぞれのネットワークの中向け(複数のアダプタがあっても届くように)、このPC自身(同じPCの別プロセス用)
    SendTo(socket, ENET_HOST_BROADCAST, discoveryPortNumber, query);
    for (const Adapter& adapter : CollectAdapters()) SendTo(socket, adapter.broadcast, discoveryPortNumber, query);
    SendTo(socket, htonl(INADDR_LOOPBACK), discoveryPortNumber, query);
}

void LanDiscovery::ReceiveQueries() {
    ENetSocket socket = (ENetSocket)responderSocket;
    uint8_t data[512];
    for (int i = 0; i < 32; i++) {   // 1フレームに処理する数の上限
        ENetAddress from;
        ENetBuffer buffer;
        buffer.data = data;
        buffer.dataLength = sizeof(data);
        const int length = enet_socket_receive(socket, &from, &buffer, 1);
        if (length == 0) break;      // もう無い
        if (length < 0) continue;    // 読めなかった(ICMPのエラーなど)。次を読む

        NetworkReader reader(data, (size_t)length);
        if (!ReadHeader(reader, kindQuery, gameId)) continue;   // 目印・ゲームIDが違う物は無視する

        // 呼びかけた相手だけに返事を返す
        NetworkWriter reply = MakeHeader(kindReply, gameId);
        reply.WriteU16((uint16_t)gamePort);
        reply.WriteString(hostName);
        reply.WriteU8((uint8_t)playerCount);
        reply.WriteU8((uint8_t)maxCount);
        SendTo(socket, from.host, from.port, reply);
    }
}

void LanDiscovery::ReceiveReplies() {
    ENetSocket socket = (ENetSocket)searchSocket;
    uint8_t data[512];
    for (int i = 0; i < 64; i++) {
        ENetAddress from;
        ENetBuffer buffer;
        buffer.data = data;
        buffer.dataLength = sizeof(data);
        const int length = enet_socket_receive(socket, &from, &buffer, 1);
        if (length == 0) break;
        if (length < 0) continue;

        NetworkReader reader(data, (size_t)length);
        if (!ReadHeader(reader, kindReply, gameId)) continue;
        NetworkHostInfo info;
        info.port = reader.ReadU16();
        info.name = reader.ReadString(Network::maxNameLength);
        info.playerCount = reader.ReadU8();
        info.maxPlayers = reader.ReadU8();
        if (!reader.IsOk() || info.port == 0 || info.maxPlayers == 0 || info.maxPlayers > 16) continue;   // 壊れた返事

        char text[64] = {};
        enet_address_get_host_ip(&from, text, sizeof(text));
        info.address = text;

        // 同じ場所のホストは1つにまとめる(同じPCは、全体向けと自分宛ての両方から返事が来る)
        bool known = false;
        for (Found& item : found) {
            if (item.info.address == info.address && item.info.port == info.port) {
                item.info = info;
                item.age = 0.f;
                known = true;
                break;
            }
        }
        if (!known && found.size() < 32) found.push_back({ info, 0.f });
    }
}

std::vector<std::string> LanDiscovery::GetLocalAddresses() {
    std::vector<std::string> result;
    for (const Adapter& adapter : CollectAdapters()) {
        const unsigned char* b = reinterpret_cast<const unsigned char*>(&adapter.ip);
        result.push_back(std::to_string(b[0]) + "." + std::to_string(b[1]) + "." + std::to_string(b[2]) + "." + std::to_string(b[3]));
    }
    return result;
}
