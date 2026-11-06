#pragma once

// ネットワーク(LAN通信)の定数を1か所に集めたもの。最大人数などを変える時は、ここだけ直す。
namespace Network {
    static constexpr int   maxPlayers = 4;                    // 最大人数(ホストを含む)。2〜4で試せる
    static constexpr int   protocolVersion = 1;               // 通信の仕様の版。違う版とはつながらない(Rejectされる)
    static constexpr int   defaultPort = 47200;               // ゲーム用のUDPポート
    static constexpr int   discoveryPort = 47201;             // ホスト探索用のUDPポート(ゲーム用とは別)
    static constexpr int   defaultSnapshotRate = 30;          // 位置の送信回数/秒(ホスト → クライアント)
    static constexpr int   inputRate = 60;                    // 入力の送信回数/秒の上限(クライアント → ホスト)
    static constexpr float defaultInterpolationDelay = 0.1f;  // 補間の遅れ(秒)

    static constexpr int   channelCount = 3;                  // ENetのチャンネルの数
    static constexpr int   channelReliable = 0;               // 信頼する通信(参加・出来事・スポーンなど)
    static constexpr int   channelInput = 1;                  // 信頼しない通信(入力・ping)
    static constexpr int   channelSnapshot = 2;               // 信頼しない通信(位置のスナップショット)

    static constexpr int   timeoutMinMs = 10000;              // 応答が無くても切らない最低の時間(ミリ秒)。ウィンドウのドラッグやデバッガの停止に備えて長め
    static constexpr int   timeoutMaxMs = 20000;              // これだけ応答が無ければ必ず切る(ミリ秒)
    static constexpr float joinTimeout = 5.f;                 // 接続してから Join が来るまで(ホスト側)/ 接続が終わるまで(クライアント側)待つ秒数
    static constexpr float inputTimeout = 1.f;                // 入力がこれだけ届かなかったら、ホストは入力をゼロに戻す(秒)
    static constexpr float pingInterval = 1.f;                // pingを送る間隔(秒)

    static constexpr int   maxNameLength = 16;                // プレイヤー名の長さの上限
    static constexpr int   maxEventNameLength = 32;           // 出来事の名前の長さの上限
    static constexpr int   maxEventPayloadSize = 512;         // 出来事の中身の大きさの上限(バイト)
    static constexpr int   maxSharedValues = 64;              // 共有の値の数の上限
    static constexpr int   maxPacketSize = 1400;              // 受け取るパケットの大きさの上限(これより大きいものは捨てる)
    static constexpr int   snapshotChunkSize = 1100;          // スナップショット1パケットの大きさの目安(これを超えたら次のパケットに分ける)
    static constexpr float positionLimit = 1000000.f;         // 受け取った位置の範囲(±)。これを超える値は範囲に収める
    static constexpr int   firstSpawnId = 1000;               // スポーンした物のNetworkIdの最初の番号(シーンに置く物は 1〜999)
    static constexpr float spawnIdReuseDelay = 5.f;           // 消したNetworkIdを、この秒数は使い回さない
}
