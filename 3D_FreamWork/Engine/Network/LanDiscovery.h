#pragma once
#include <string>
#include <vector>
#include "NetworkMessage.h"

// LANでホストを探す仕組み。ゲーム本体とは別のUDPポート(Network::discoveryPort)を使う。
// - ホスト側(StartResponder): 「ホストの人、いますか?」の呼びかけを受けたら、呼びかけた相手だけに返事を返す
// - 探す側(StartSearch): 1秒おきにLAN全体へ呼びかけて、返事をGetFoundHostsに溜める(数秒返事が無いホストは消す)
// ゲームIDが違う相手(同じLANで動いている別のゲーム)は無視する。ENetのソケットのAPIを使う(生のWinsockは触らない)
class LanDiscovery {
private:
    unsigned long long responderSocket;     // ホスト側の待ち受けソケット
    unsigned long long searchSocket;        // 探す側のソケット
    bool responding = false;
    bool searching = false;
    std::string gameId = "Game";
    std::string hostName;                   // 返事に入れる名前
    int gamePort = 0;                       // 返事に入れるゲーム用のポート
    int playerCount = 1;                    // 返事に入れる人数
    int maxCount = 1;
    int discoveryPortNumber = 0;            // 使っている探索用のポート
    float queryTimer = 0.f;                // 呼びかけの間隔の計測
    bool holdsEnet = false;

    struct Found {
        NetworkHostInfo info;
        float age = 0.f;                    // 最後に返事が来てからの秒数
    };
    std::vector<Found> found;
    std::vector<NetworkHostInfo> foundInfos;   // GetFoundHosts で返す一覧

    void SendQueries();
    void ReceiveQueries();
    void ReceiveReplies();
    void CloseSockets();

public:
    LanDiscovery();
    ~LanDiscovery();
    LanDiscovery(const LanDiscovery&) = delete;
    LanDiscovery& operator=(const LanDiscovery&) = delete;

    void SetGameId(const std::string& id) { gameId = id; }

    bool StartResponder(const std::string& name, int port, int portOfDiscovery = 0);   // ホスト側を始める(探索用ポートを使えなければ false)
    void StopResponder();
    void SetPlayerCount(int count, int maxPlayers) { playerCount = count; maxCount = maxPlayers; }
    bool IsResponding() const { return responding; }

    bool StartSearch(int portOfDiscovery = 0);   // 探す側を始める
    void StopSearch();
    bool IsSearching() const { return searching; }

    void Update(float dt);
    const std::vector<NetworkHostInfo>& GetFoundHosts() const { return foundInfos; }

    // このPCのIPv4アドレスの一覧(ホスト画面に出して、手入力で参加する人に教える用)。127.0.0.1 は入れない
    static std::vector<std::string> GetLocalAddresses();
};
