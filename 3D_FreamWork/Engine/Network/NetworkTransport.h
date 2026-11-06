#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "NetworkMessage.h"

struct _ENetHost;
struct _ENetPeer;

// ENetの初期化と終了(何回呼ばれても1回だけ初期化する。NetworkTransport と LanDiscovery が使う)
bool NetworkAcquireEnet();
void NetworkReleaseEnet();

// ENet(UDPの上に「信頼できる通信」と「速いけれど落ちることもある通信」を作るライブラリ)を包んだクラス。
// ENetを知っているのは、このクラスと LanDiscovery だけ(公開ヘッダーに enet/enet.h を出さない)。
// 接続相手は peerId(整数)で呼ぶ。Network の外でも new できるので、ウィンドウ無しのテストでも使える。
class NetworkTransport {
public:
    enum class EventType { Connect, Disconnect, Receive };

    struct Event {
        EventType type = EventType::Receive;
        int peerId = -1;                // 相手の番号
        int channel = 0;                // Receive の時: 届いたチャンネル
        std::vector<uint8_t> data;      // Receive の時: パケットの中身
    };

private:
    _ENetHost* host = nullptr;
    bool isServer = false;
    bool holdsEnet = false;             // ENetの初期化を1回分持っているか
    int timeoutMinMs;
    int timeoutMaxMs;

    // 1秒ごとの送受信量の計算用
    NetworkStats stats;
    unsigned int lastSentData = 0, lastReceivedData = 0, lastSentPackets = 0, lastReceivedPackets = 0;
    float statsTimer = 0.f;

    _ENetPeer* GetPeer(int peerId) const;

public:
    NetworkTransport();
    ~NetworkTransport();
    NetworkTransport(const NetworkTransport&) = delete;
    NetworkTransport& operator=(const NetworkTransport&) = delete;

    bool StartServer(int port, int maxPeers);       // 待ち受けを始める。失敗(ポート使用中など)は false
    bool StartClient();                             // 接続する側の準備
    int  Connect(const std::string& address, int port);   // 接続を始める。相手の番号を返す(始められなければ -1)。つながった時に Connect イベントが来る
    void Stop();                                    // 全員に切断を知らせて(少しだけ待って)閉じる
    bool IsActive() const { return host != nullptr; }
    bool IsServer() const { return isServer; }

    bool Poll(Event& out);                          // 溜まっている出来事を1つ取り出す(無ければ false)。毎フレーム、false になるまで呼ぶ
    bool Send(int peerId, int channel, const void* data, size_t size, bool reliable);
    bool Send(int peerId, int channel, const NetworkWriter& writer, bool reliable) { return Send(peerId, channel, writer.GetData().data(), writer.GetSize(), reliable); }
    void Flush();                                   // 溜めた送信をすぐ流す
    void DisconnectPeer(int peerId);                // 溜めた送信を流してから切る
    void SetTimeouts(int minMs, int maxMs);         // 応答が無い時に切るまでの時間(ミリ秒)。テスト用に変えられる

    float GetPeerRoundTrip(int peerId) const;       // ENetが測った往復の時間(秒)
    std::string GetPeerAddress(int peerId) const;   // 相手のIPアドレス
    bool IsPeerConnected(int peerId) const;

    void UpdateStats(float dt);                     // 1秒ごとの送受信量を数え直す
    const NetworkStats& GetStats() const { return stats; }
};
