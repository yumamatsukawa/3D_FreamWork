#pragma once
#include <functional>
#include <map>
#include <string>
#include "NetworkConfig.h"
#include "NetworkMessage.h"
#include "NetworkTransport.h"

// 「今つながっているみんな」のまとまり(セッション)。参加者の名簿・プレイヤー番号・参加の受付(Join/Accept/Reject)・
// 切断の検知・pingを担当する。入力・出来事・同期など、ゲームの中身のパケットは onPacket で上の層(Network)に渡す。
// Network の外でも new できるので、ウィンドウ無しのテストで、ホスト役とクライアント役を同じプログラムに作れる。
class NetworkSession {
public:
    using PacketHandler = std::function<void(int senderIndex, PacketType type, NetworkReader& reader)>;

    struct Player {
        bool connected = false;    // 今いるか
        std::string name;          // 名前
        int peerId = -1;           // ホストだけ: そのクライアントの接続の番号
    };

private:
    NetworkTransport transport;
    NetworkState state = NetworkState::Offline;
    std::string gameId = "Game";                     // 同じIDのゲームどうしだけつながる
    std::string localName;                           // 自分の名前
    Player players[Network::maxPlayers];             // 名簿(番号 0 がホスト)
    int localIndex = 0;                              // 自分のプレイヤー番号
    int hostPeerId = -1;                             // クライアント: ホストの接続の番号
    std::map<int, float> pending;                    // ホスト: 接続したけれどまだ Join が来ていない相手(接続の番号 → 待った秒数)
    float connectTimer = 0.f;                        // クライアント: 接続を始めてからの秒数
    float pingTimer = 0.f;                           // pingを送る間隔の計測
    float ping = 0.f;                                // ホストとの往復の時間(秒)
    double clock = 0.0;                              // このセッションが動いている時間(秒)
    std::string lastError;                           // 直近の失敗の理由(画面に出す)

    void HandleConnect(int peerId);
    void HandleDisconnect(int peerId);
    void HandleReceive(int peerId, const std::vector<uint8_t>& data);
    void HandleJoin(int peerId, NetworkReader& reader);
    void HandleAccept(NetworkReader& reader);
    void HandleReject(NetworkReader& reader);
    void RemovePlayer(int index);
    void LoseConnection(const std::string& reason);   // クライアント: ホストとの接続が切れた
    int  FindPlayerByPeer(int peerId) const;

public:
    // ─── ゲームの中身のパケットを受け取る(ホスト: senderIndex はクライアントの番号 / クライアント: いつも 0) ───
    PacketHandler onPacket;
    std::function<void(int)> onPlayerJoined;       // 誰かが入った(ホスト・クライアントどちらでも。引数は番号)
    std::function<void(int)> onPlayerLeft;         // 誰かが出た
    std::function<void(int)> onClientAccepted;     // ホストだけ: 参加を許可した直後(途中参加の人に今の状態を送る)
    std::function<void()> onConnected;             // クライアントだけ: 参加が許可された
    std::function<void()> onDisconnected;          // クライアントだけ: つながらなくなった(ホストが閉じた・断られた・応答なし)
    std::function<void(PacketType, int, bool)> onTrace;   // デバッグ用: 送受信したパケットの種類(種類, 相手の番号, 受信か)

    // ホストが決める「今のシーン」の情報(Accept に入れて、途中参加の人に伝える。クライアントは Accept で上書きされる)
    int sceneGeneration = 0;
    int sceneId = 0;

    NetworkSession();
    void SetGameId(const std::string& id) { gameId = id; }
    const std::string& GetGameId() const { return gameId; }

    bool StartHost(const std::string& playerName, int port);                                   // 部屋を作る。失敗(ポート使用中など)は false
    bool Join(const std::string& address, int port, const std::string& playerName);            // 参加を申し込む。結果は GetState() で分かる
    void Disconnect();                                                                         // 抜ける / 部屋を閉じる
    void Update(float dt);                                                                     // 受信と、タイムアウト・pingの処理(毎フレーム)
    void Flush() { transport.Flush(); }

    // ─── 状態 ───
    NetworkState GetState() const { return state; }
    bool IsActive() const { return state != NetworkState::Offline; }
    bool IsHost() const { return state == NetworkState::Hosting; }
    bool IsClient() const { return state == NetworkState::Connected || state == NetworkState::Connecting; }
    int  GetLocalPlayerIndex() const { return localIndex; }
    int  GetPlayerCount() const;
    bool IsPlayerConnected(int index) const;
    std::string GetPlayerName(int index) const;
    int  GetPlayerPeerId(int index) const;
    float GetPing() const { return ping; }
    double GetClock() const { return clock; }
    const std::string& GetLastError() const { return lastError; }
    NetworkStats GetStats() const { return transport.GetStats(); }
    NetworkTransport& GetTransport() { return transport; }

    // ─── 送信(ホスト: playerIndex のクライアントへ / クライアント: ホストへ) ───
    bool SendTo(int playerIndex, const NetworkWriter& packet, int channel, bool reliable);
    void SendToAll(const NetworkWriter& packet, int channel, bool reliable, int exceptPlayer = -1);   // ホストだけ: 接続中のクライアント全員へ
    bool SendToHost(const NetworkWriter& packet, int channel, bool reliable);                          // クライアントだけ
};
