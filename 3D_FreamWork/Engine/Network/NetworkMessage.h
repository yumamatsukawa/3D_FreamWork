#pragma once
#include <cstdint>
#include <string>
#include <vector>

// ─── ネットワークで共通に使う型 ───────────────────────────────

// 今の状態
enum class NetworkState { Offline, Hosting, Connecting, Connected };   // Connected = クライアントとして参加中

// 出来事の送り先
enum class NetworkTarget { Host, All, Others };   // All = 自分を含む全員、Others = 自分以外

// 確認用の数(HUDに出す)
struct NetworkStats {
    int bytesSent = 0, bytesReceived = 0;       // 直近1秒の送受信バイト数
    int packetsSent = 0, packetsReceived = 0;   // 直近1秒の送受信パケット数
};

// LANで見つかったホスト1台分
struct NetworkHostInfo {
    std::string name;             // ホスト名(表示用)
    std::string address;          // "192.168.0.12" のようなIPアドレス
    int port = 0;                 // ゲーム用のポート
    int playerCount = 0;          // 今の人数
    int maxPlayers = 0;           // 最大人数
};

// ─── パケットの種類 ───────────────────────────────────────────
// パケットの最初の1バイトが種類
enum class PacketType : uint8_t {
    Join = 1,        // クライアント → ホスト: 参加の申し込み
    Accept,          // ホスト → クライアント: 参加を許可
    Reject,          // ホスト → クライアント: 参加を断る
    PlayerJoined,    // ホスト → 全員: 名簿の更新(入った)
    PlayerLeft,      // ホスト → 全員: 名簿の更新(出た)
    Leave,           // クライアント → ホスト: 自分から抜ける
    SceneChange,     // ホスト → 全員: シーンの切り替え
    SceneReady,      // クライアント → ホスト: 切り替えが終わった
    Spawn,           // ホスト → 全員: オブジェクトが現れた
    Despawn,         // ホスト → 全員: オブジェクトが消えた
    Event,           // 誰か → 相手: 出来事
    SharedValue,     // ホスト → 全員: 共有の値
    Ping,            // クライアント → ホスト: pingの測定
    Pong,            // ホスト → クライアント: pingの返事
    Input,           // クライアント → ホスト: 入力(信頼しない)
    Snapshot,        // ホスト → クライアント: 位置のスナップショット(信頼しない)
    Count_           // 種類の数(検査用。送らない)
};

// 参加を断る理由
enum class RejectReason : uint8_t { Full = 1, VersionMismatch, GameIdMismatch, Invalid };

const char* GetPacketTypeName(PacketType type);       // デバッグ表示用の名前
const char* GetRejectReasonText(RejectReason reason); // 画面に出す理由

// ─── 書き込み ─────────────────────────────────────────────────
// 数値や文字を並べて書く。サイズが決まっているので、相手のPCでも同じように読める(リトルエンディアン前提)
class NetworkWriter {
private:
    std::vector<uint8_t> data;

public:
    void WriteU8(uint8_t value);
    void WriteU16(uint16_t value);
    void WriteU32(uint32_t value);
    void WriteI32(int32_t value);
    void WriteFloat(float value);
    void WriteDouble(double value);
    void WriteString(const std::string& value);   // 長さ(1バイト)+文字。255文字を超える分は切り捨てる
    void WriteBytes(const void* bytes, size_t size);

    const std::vector<uint8_t>& GetData() const { return data; }
    size_t GetSize() const { return data.size(); }
    bool IsEmpty() const { return data.empty(); }
    void Clear() { data.clear(); }
};

// 最初の1バイトが種類のパケットを作る
NetworkWriter BeginPacket(PacketType type);

// ─── 読み取り ─────────────────────────────────────────────────
// 範囲外を読もうとしたら、0を返して「失敗」の印を付ける(落ちない)。読み終わったら IsOk() で確かめる。
// 浮動小数点は、NaN・無限大なら失敗にする(壊れたパケット対策)
class NetworkReader {
private:
    const uint8_t* data;
    size_t size;
    size_t position = 0;
    bool failed = false;

public:
    NetworkReader(const uint8_t* bytes, size_t byteSize) : data(bytes), size(bytes ? byteSize : 0) {}
    explicit NetworkReader(const std::vector<uint8_t>& bytes) : data(bytes.data()), size(bytes.size()) {}

    uint8_t ReadU8();
    uint16_t ReadU16();
    uint32_t ReadU32();
    int32_t ReadI32();
    float ReadFloat();
    double ReadDouble();
    std::string ReadString(size_t maxLength = 255);   // maxLength より長い文字は失敗にする
    bool ReadBytes(void* destination, size_t byteCount);

    bool IsOk() const { return !failed; }                  // ここまでの読み取りが全部成功したか
    size_t GetRemaining() const { return size - position; }
    const uint8_t* GetCurrent() const { return data + position; }   // 残りの先頭(中身をそのまま渡したい時用)
};
