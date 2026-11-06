#include "NetworkLobbyController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Network/Network.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/UI/ButtonComponent.h"
#include "../../../../Engine/UI/Anchor.h"

namespace {
    constexpr float textX = -616.f;          // 文字の左端のX
    constexpr float firstLineY = 98.f;       // 1行目の文字のY(ボタンの2行の下)
    constexpr float lineGap = 25.f;          // 行の間隔
    constexpr float textSize = 19.f;         // 文字の大きさ
    constexpr float panelWidth = 830.f;      // 文字の後ろの板の幅
    constexpr size_t maxIpLength = 15;       // IPアドレスの文字数の上限("255.255.255.255")
    constexpr int maxReceived = 3;           // 覚えておく受け取った文字の数
    constexpr int maxShownAddresses = 3;     // 画面に出すIPアドレスの数(1行に収める)

    // 英数字だけの文字列を画面用の文字列にする(名前・IPアドレス・エラーは英数字で作っている)
    std::wstring ToWide(const std::string& text) {
        return std::wstring(text.begin(), text.end());
    }

    const wchar_t* StateName(NetworkState state) {
        switch (state) {
        case NetworkState::Hosting:    return L"ホスト中";
        case NetworkState::Connecting: return L"接続中...";
        case NetworkState::Connected:  return L"参加中";
        default:                       return L"オフライン(1人用)";
        }
    }
}

void NetworkLobbyController::Init() {
    Network::SetGameId("Demo");   // 同じIDのデモどうしだけつながる

    // 文字を受け取る(this を使うので、シーンの Uninit で Network::ClearHandlers する)
    Network::Subscribe("Chat", [this](NetworkReader& payload, int sender) {
        const std::string text = payload.ReadString();
        if (!payload.IsOk()) return;
        received.insert(received.begin(), L"[" + ToWide(Network::GetPlayerName(sender)) + L"] " + ToWide(text));
        if ((int)received.size() > maxReceived) received.pop_back();
    });
}

// ─── ボタン ───

void NetworkLobbyController::OnHost() {
    Network::StartHost("Host");   // 失敗(ポート使用中など)の理由は、画面のエラーに出る
}

void NetworkLobbyController::OnJoinLocal() {
    Network::Join("127.0.0.1", Network::defaultPort, "Guest");
}

void NetworkLobbyController::OnJoinIp() {
    editingIp = false;
    Network::Join(ipText, Network::defaultPort, "Guest");
}

void NetworkLobbyController::OnToggleEditIp() {
    editingIp = !editingIp;
}

void NetworkLobbyController::OnFind() {
    Network::StartSearch();
}

void NetworkLobbyController::OnJoinFound(int index) {
    const std::vector<NetworkHostInfo>& found = Network::GetFoundHosts();
    if (index < 0 || index >= (int)found.size()) return;
    const NetworkHostInfo host = found[index];   // Join の中で一覧が変わっても困らないようコピー
    Network::StopSearch();
    Network::Join(host.address, host.port, "Guest");
}

void NetworkLobbyController::OnSend() {
    // 文字を全員へ(自分にも届く)。受け取り側は Init の Subscribe で表示する
    sendCount++;
    NetworkWriter payload;
    payload.WriteString("Hello #" + std::to_string(sendCount));
    Network::SendEvent("Chat", payload, NetworkTarget::All);
}

void NetworkLobbyController::OnDisconnect() {
    Network::Disconnect();
}

// ─── 毎フレーム ───

void NetworkLobbyController::EditIp() {
    // 数字・ピリオド・Backspace・Enter(Tab/Esc/Q/Eはデモの共通キーなので使わない)
    for (int digit = 0; digit <= 9; digit++) {
        if ((Input::GetKeyDown(KEY_0 + digit) || Input::GetKeyDown(VK_NUMPAD0 + digit)) && ipText.size() < maxIpLength) {
            ipText += (char)('0' + digit);
        }
    }
    if ((Input::GetKeyDown(VK_OEM_PERIOD) || Input::GetKeyDown(VK_DECIMAL)) && ipText.size() < maxIpLength) ipText += '.';
    if (Input::GetKeyDown(KEY_BACK) && !ipText.empty()) ipText.pop_back();
    if (Input::GetKeyDown(KEY_ENTER)) editingIp = false;
}

void NetworkLobbyController::Update(float dt) {
    if (editingIp) EditIp();

    // 見つかったホストのボタン(オフラインで探している間だけ出す)
    const std::vector<NetworkHostInfo>& found = Network::GetFoundHosts();
    const bool show = Network::GetState() == NetworkState::Offline;
    for (size_t i = 0; i < hostButtons.size(); i++) {
        const bool visible = show && i < found.size();
        hostButtons[i]->SetActive(visible);
        if (!visible) continue;
        const NetworkHostInfo& host = found[i];
        hostButtons[i]->GetComponent<ButtonComponent>()->SetLabel(
            ToWide(host.address + ":" + std::to_string(host.port) + "  " + host.name + " " +
                std::to_string(host.playerCount) + "/" + std::to_string(host.maxPlayers)));
    }
}

void NetworkLobbyController::Draw() {
    const NetworkState state = Network::GetState();
    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };
    const DirectX::XMFLOAT4 yellow = { 1.f, 0.9f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 red = { 1.f, 0.5f, 0.5f, 1.f };

    struct Line { std::wstring text; DirectX::XMFLOAT4 color; };
    std::vector<Line> lines;

    // 状態: 接続・自分の番号・人数・ping
    std::wstring status = std::wstring(L"状態: ") + StateName(state);
    if (state == NetworkState::Hosting || state == NetworkState::Connected) {
        status += L"   自分: プレイヤー" + std::to_wstring(Network::GetLocalPlayerIndex()) +
            L"   人数: " + std::to_wstring(Network::GetPlayerCount()) + L"/" + std::to_wstring(Network::maxPlayers);
    }
    if (state == NetworkState::Connected) status += L"   ping: " + std::to_wstring((int)(Network::GetPing() * 1000.f + 0.5f)) + L" ms";
    lines.push_back({ status, yellow });

    // 名簿
    std::wstring roster = L"名簿:";
    for (int i = 0; i < Network::maxPlayers; i++) {
        roster += L"  " + std::to_wstring(i) + L":" + (Network::IsPlayerConnected(i) ? ToWide(Network::GetPlayerName(i)) : std::wstring(L"-"));
    }
    lines.push_back({ roster, white });

    // このPCのIPアドレス(別のPCから手入力で参加する人に教える)
    std::wstring addresses = L"このPCのIP:";
    int shown = 0;
    for (const std::string& address : Network::GetLocalAddresses()) {
        if (++shown > maxShownAddresses) break;   // アダプタが多いPCでも、1行に収まる数まで
        addresses += L"  " + ToWide(address);
    }
    addresses += L"   ポート: " + std::to_wstring(Network::defaultPort);
    lines.push_back({ addresses, cyan });

    // 参加先のIP(入力中は | を出す)
    if (options.joinIp) {
        lines.push_back({ L"参加先のIP: " + ToWide(ipText) + (editingIp ? L"|   (数字・ピリオド・Backspace、Enterで確定)" : L"   (Edit IP で入力)"), editingIp ? yellow : white });
    }

    // 通信量(直近1秒)
    const NetworkStats stats = Network::GetStats();
    lines.push_back({ L"通信量(1秒): 送信 " + std::to_wstring(stats.bytesSent) + L" B / " + std::to_wstring(stats.packetsSent) +
        L" 個   受信 " + std::to_wstring(stats.bytesReceived) + L" B / " + std::to_wstring(stats.packetsReceived) + L" 個", white });

    // ホスト探索
    if (options.find) {
        lines.push_back({ Network::IsSearching() ? L"探索中: 見つかったホスト " + std::to_wstring(Network::GetFoundHosts().size()) + L" 台(右のボタンで参加)"
                                                  : std::wstring(L"探索: Find Hosts を押すとLANのホストを探します"), cyan });
    }

    // 受け取った文字
    if (options.send) {
        for (int i = 0; i < maxReceived; i++) {
            lines.push_back({ i < (int)received.size() ? L"受信: " + received[i] : std::wstring(L"受信: -"), white });
        }
    }

    // エラー(接続できなかった・断られた、など)
    if (!Network::GetLastError().empty()) lines.push_back({ L"エラー: " + ToWide(Network::GetLastError()), red });

    // 文字の後ろの半透明の板(文字より先に描く)
    const DirectX::XMFLOAT2 panelSize = { panelWidth, lineGap * (float)lines.size() + 10.f };
    Image::DrawRect({ textX - 10.f + panelWidth * 0.5f, firstLineY + lineGap * 0.5f - panelSize.y * 0.5f + 5.f }, panelSize, { 0.f, 0.f, 0.f, 0.55f });

    float y = firstLineY;
    for (const Line& line : lines) {
        Text::Draw(line.text, textX, y, MakeDemoTextStyle(textSize, line.color));
        y -= lineGap;
    }
}
