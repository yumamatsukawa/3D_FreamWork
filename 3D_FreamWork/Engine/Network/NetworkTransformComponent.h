#pragma once
#include <deque>
#include <DirectXMath.h>
#include "../Core/Component.h"
#include "NetworkMessage.h"

// 位置・向き・大きさの同期の設定
struct NetworkTransformSettings {
    bool syncPosition = true;
    bool syncRotation = true;
    bool syncScale = false;               // 大きさは変わる物だけ true(変わった時だけ送る)
    bool is2D = false;                    // true なら position.xy と rotate.z だけ送る(量が減る)
    float teleportDistance = 500.f;       // これ以上離れたら補間せずに飛ぶ
};

// Transform(位置・向き・大きさ)をホストからクライアントへ同期するComponent。NetworkIdentityComponent と一緒に付ける。
// - ホスト(Authority): Network::EndFrame の時に Transform を読んでスナップショットに入れる(30回/秒)
// - クライアント: 届いたスナップショットを少し遅れて(Network::SetInterpolationDelay)滑らかにつないで、Transform に書く
// 書く・読むタイミングは Network が登録簿を通じて決める(Updateでは何もしない。Rigidbodyとの順番の影響を受けないため)。
//
// 決まり:
// - 同期するのは、親を持たない(ルートの)オブジェクトだけ
// - クライアントのコピーは、Dynamicの剛体にしない(Kinematic か、剛体なし)。Transformを書き換えるため
// - 向き(rotate)は度数のXYZ。360度をまたぐ時は近い向きに回る
class NetworkTransformComponent : public Component {
private:
    struct Sample {
        double time = 0.0;                 // ホストの時刻
        DirectX::XMFLOAT3 position{};
        DirectX::XMFLOAT3 rotate{};
        DirectX::XMFLOAT3 scale{ 1.f, 1.f, 1.f };
    };

    NetworkTransformSettings settings;
    std::deque<Sample> samples;            // クライアント: 届いた写真(古い順)
    DirectX::XMFLOAT3 lastSentScale{};     // ホスト: 最後に送った大きさ
    int scaleResendCounter = 0;            // ホスト: 大きさを定期的に送り直すための数え

public:
    explicit NetworkTransformComponent(const NetworkTransformSettings& transformSettings = NetworkTransformSettings{})
        : settings(transformSettings) {}

    void Init() override;
    void Uninit() override;
    const NetworkTransformSettings& GetSettings() const { return settings; }

    // ─── Network内部用(ゲーム側からは呼ばない) ───
    void WriteState(NetworkWriter& writer);                          // ホスト: 今の Transform をスナップショットの1件として書く
    bool ReadState(NetworkReader& reader, double hostTime);          // クライアント: 1件読んで写真を溜める(壊れていたら false)
    void Interpolate(double renderTime);                             // クライアント: 表示の時刻に合わせて Transform に書く
    void ClearSamples() { samples.clear(); }
    size_t GetSampleCount() const { return samples.size(); }

    static float LerpAngle(float from, float to, float t);          // 角度(度)の補間。360度をまたぐ時は近い向きに回る
};
