#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>
#include <vector>

class GameObject;

// どのボタンを出すか
struct NetworkLobbyOptions {
    bool joinLocal = true;   // Join 127.0.0.1(同じPCの2つ目から参加)
    bool joinIp = true;      // IPアドレスを入力して参加
    bool find = true;        // LANのホストを探す(見つかったホストはボタンで並ぶ)
    bool send = false;       // 文字を送る
};

// ネットワークのデモ共通の「つなぐ画面」。ボタンの処理と、状態(接続・人数・ping・通信量・IP・エラー)の表示を担当する
class NetworkLobbyController : public Component {
private:
    NetworkLobbyOptions options;
    std::string ipText = "192.168.0.";          // 参加先のIPアドレス(入力中の文字)
    bool editingIp = false;                     // IPアドレスを入力している間か
    int sendCount = 0;                          // 送った文字の数(文字に番号を付ける用)
    std::vector<std::wstring> received;         // 受け取った文字(新しい順に最大3つ)
    std::vector<GameObject*> hostButtons;       // 見つかったホストのボタン(使わない時は非アクティブ)

    void EditIp();                              // 入力中のキーを読む

public:
    explicit NetworkLobbyController(const NetworkLobbyOptions& lobbyOptions) : options(lobbyOptions) {}

    void AddHostButton(GameObject* button) { hostButtons.push_back(button); }

    // ─── ボタンの処理 ───
    void OnHost();
    void OnJoinLocal();
    void OnJoinIp();
    void OnToggleEditIp();
    void OnFind();
    void OnJoinFound(int index);
    void OnSend();
    void OnDisconnect();

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
