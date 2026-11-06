#pragma once
#include <functional>
#include <string>
#include <vector>
#include <DirectXMath.h>
#include "NetworkConfig.h"
#include "NetworkMessage.h"
#include "NetworkInput.h"
#include "NetworkObjects.h"
#include "NetworkIdentityComponent.h"
#include "NetworkTransformComponent.h"

// LAN通信(ネットワーク)の窓口。ゲーム側が呼ぶのはここと、2つのComponent(NetworkIdentityComponent / NetworkTransformComponent)だけ。
// アプリ全体で1つ、シーンをまたいで接続が残る(Input / Audio / Time と同じ「名前空間+関数」の形。Get()は無い)。
//
// 方式: ホスト式。1台が「ホスト」になり、ルールと物理(PhysX)の計算はホストだけが行う。
//       クライアントは「入力」だけをホストへ送り、ホストから届いた「結果(位置など)」を表示する。
//       ホスト自身もプレイヤーの1人(番号 0)。
//
// 使い方:
//   Network::SetGameId("MyGame");
//   Network::StartHost("Player1");                                   // ホストになる
//   Network::Join("127.0.0.1", Network::defaultPort, "Player2");     // 参加する(別のPCは、ホストの画面に出るIPアドレス)
//   Network::StartSearch();  Network::GetFoundHosts();               // LANのホストを探す
//   Network::SendEvent("DoorOpened");                                // 出来事を全員へ
//   Network::Subscribe("DoorOpened", [this](NetworkReader& payload, int sender) { ... });
//
// 何もしなければ今まで通り: StartHost / Join / StartSearch を呼ばない限り、Update / EndFrame は何もしない(オフライン)。
// オフラインでも、ゲームのコードは同じに書ける(IsHost() は true、GetLocalPlayerIndex() は 0、GetInput(0) は自分の入力)。
//
// 決まり:
// - ルールを書くComponentは、Update / OnTriggerEnter の最初に if (!identity->HasAuthority()) return; と書く(クライアントでは動かさない)
// - this を使うラムダを Subscribe / RegisterPrefab したら、そのSceneの Uninit() で Network::ClearHandlers() を呼ぶ(EventBus::Clear() と同じ決まり)
// - 同じLANの信頼できる相手が前提(暗号化・認証・ずる対策は無い)。公共のWi-Fiで他人がいる所では使わない
namespace Network {
    using EventHandler = std::function<void(NetworkReader& payload, int senderPlayerIndex)>;
    using PrefabFactory = NetworkObjects::PrefabFactory;

    // ─── 準備 ───
    void SetGameId(const std::string& gameId);   // 同じIDのゲームどうしだけつながる("Demo" など。ほかの人のゲームと混ざらない)

    // ─── ホストになる / 探す / 参加する / やめる ───
    bool StartHost(const std::string& playerName, int port = defaultPort);   // 部屋を作る。自分は番号 0。失敗(ポート使用中など)は false(理由は GetLastError)
    void StartSearch();                                        // LANのホスト探索を始める(1秒おきに呼びかける)
    void StopSearch();
    bool IsSearching();
    const std::vector<NetworkHostInfo>& GetFoundHosts();       // 見つかったホスト一覧(消えたホストは数秒で消える)
    bool Join(const std::string& address, int port, const std::string& playerName);   // 参加を申し込む(結果は GetState() / "NetworkConnected" で分かる)
    void Disconnect();                                         // 抜ける / 部屋を閉じる(ホストなら全員が切断される)

    // ─── 状態を調べる ───
    NetworkState GetState();
    bool IsOnline();               // ホスト中または参加中(参加の途中も含む)
    bool IsHost();                 // ホスト中か。オフラインの時も true(1人で遊ぶ時は自分がホスト扱い)
    bool IsClient();               // 参加中(途中も含む)か
    int  GetLocalPlayerIndex();    // 自分のプレイヤー番号(オフライン・ホストは 0)
    int  GetPlayerCount();         // 今の人数
    bool IsPlayerConnected(int playerIndex);
    std::string GetPlayerName(int playerIndex);
    float GetPing();               // ホストとの往復の時間(秒)。ホストは 0
    NetworkStats GetStats();       // 直近1秒の送受信のバイト数・パケット数(確認用)
    const std::string& GetLastError();                  // 直近の失敗の理由(接続できなかった・断られた、など)
    std::vector<std::string> GetLocalAddresses();       // このPCのIPアドレス(ホスト画面に出して、手入力で参加する人に教える)
    float GetSnapshotAge();        // クライアント: 最後に位置が届いてからの秒数(大きい時は「つながりが悪い」)

    // ─── 入力(クライアントは送る。ホストは受け取る。ホスト自身の入力もここを通る) ───
    void SetLocalInput(const NetworkInput& input);   // 自分の入力を渡す(毎フレーム。ゲーム側のComponentが呼ぶ)。押した瞬間(pressed)は自動で計算される
    const NetworkInput& GetInput(int playerIndex);   // そのプレイヤーの最新の入力(ホストが読む)。範囲外は全部ゼロ。pressed は1フレームの間だけ立つ

    // ─── 出来事(信頼して送る) ───
    void SendEvent(const std::string& name, NetworkTarget target = NetworkTarget::All);                                // 中身の無い出来事。受け取り側では EventBus::Publish(name) も呼ばれる
    void SendEvent(const std::string& name, const NetworkWriter& payload, NetworkTarget target = NetworkTarget::All);  // 中身つき
    void Subscribe(const std::string& name, EventHandler handler);   // 受け取る。Scene::Uninit で ClearHandlers する
    void ClearHandlers();                                            // 出来事の登録・プレハブの登録・シーンの記憶を消す

    // ─── 共有の値(「今の状態」。ホストが変えると全員に届き、途中参加の人にも渡る) ───
    void SetSharedValue(const std::string& key, int value);          // ホストだけが呼べる(クライアントが呼ぶと出力ウィンドウに知らせて何もしない)
    int  GetSharedValue(const std::string& key, int defaultValue = 0);
    // 値が変わると EventBus に "SharedValueChanged" が届く

    // ─── プレイヤーの出入り ───
    void SetPlayerJoinedHandler(std::function<void(int playerIndex)> handler);
    void SetPlayerLeftHandler(std::function<void(int playerIndex)> handler);
    // EventBus にも "NetworkPlayerJoined" / "NetworkPlayerLeft" / "NetworkConnected" / "NetworkDisconnected" を Publish する(引数なし)

    // ─── スポーンとシーン ───
    void RegisterPrefab(int prefabId, PrefabFactory factory);        // 「この番号はこの工場関数で作る」を登録(ホストもクライアントも同じ登録をする)
    GameObject* Spawn(Scene& scene, int prefabId, int ownerPlayerIndex, DirectX::XMFLOAT3 position);   // ホストだけ。作って全員に知らせる(オフラインでも使える)
    void Despawn(GameObject* object);                                // ホストだけ。消して全員に知らせる
    void SetDespawnHandler(int prefabId, std::function<void(GameObject*)> handler);   // 既定は object->Destroy()。ObjectPool の物は Return を入れる
    void RegisterScene(int sceneId, std::function<void()> changeScene);   // 「この番号のシーンへの切り替え方」を登録([]{ SceneManager::Get().ChangeScene<GameScene>(); } など)
    void ChangeSceneForAll(int sceneId);                             // ホストだけ。全員をそのシーンへ
    void NotifySceneReady(Scene& scene, int sceneId);                // 各シーンの Init の最後で呼ぶ(「準備できた」。ホストはこれを待って今の状態を送る)

    // ─── 調整 ───
    void SetSnapshotRate(int hz);                  // 10〜60
    void SetInterpolationDelay(float seconds);     // 補間の遅れ
    void SetDebugLogEnabled(bool enabled);         // 送受信したパケットの種類を出力ウィンドウに出す(既定オフ)

    // ─── Engine内部用(GameEngineだけが呼ぶ。ゲーム側からは呼ばない) ───
    void Update();      // 受信(Input::Update の直後)
    void EndFrame();    // 送信(Audio::Update の後)
    void Shutdown();    // 終了(SceneManager::Uninit の後)
}
