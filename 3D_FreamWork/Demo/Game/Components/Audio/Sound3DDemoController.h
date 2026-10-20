#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../Common/DemoPlane.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class GameObject;
class AudioSourceComponent;
class DemoCameraController;

// Sound3Dデモの操作役。音源(AudioSourceComponent。ぐるぐる回る)と、聞く側(AudioListenerComponent。WASDで動かす)を見せる。
// 近いほど大きく、左にあれば左から聞こえる(2D版は画面の上が正面、3D版は3Dカメラの向きが正面)。
// 緑の輪=最大音量の距離、黄色の輪=聞こえなくなる距離(SetDistance)。1で鳴/止、2で位置の効き(SetSpatial)、
// ↑↓で聞こえなくなる距離、←→で回る速さ、SPACEでその場所に1回だけ鳴らす(Audio::PlayOneShotAt)
class Sound3DDemoController : public Component {
private:
    DemoMode mode;                                   // 2D版か3D版か
    DemoField field;                                 // 聞く側が動ける範囲
    GameObject* emitter = nullptr;                   // 音源の物
    GameObject* listener = nullptr;                  // 聞く側の物
    AudioSourceComponent* source = nullptr;          // 音源の音
    DemoCameraController* camera = nullptr;          // 3D版: 正面の向きの基準(2D版は nullptr)
    DirectX::XMFLOAT2 orbitCenter{};                 // 音源が回る中心(平面の点)
    DirectX::XMFLOAT2 orbitRadius{};                 // 音源が回る大きさ(横, 縦)
    float objectSize = 50.f;                         // 物の大きさ
    float listenerSpeed = 350.f;                     // 聞く側の移動の速さ
    float minDistance = 100.f;                       // この距離までは最大音量
    float maxDistance = 600.f;                       // この距離で聞こえなくなる
    float distanceStep = 100.f;                      // ↑↓で1回に変える量
    float angle = 0.f;                               // 音源が回っている角度(ラジアン)
    float orbitSpeed = 0.8f;                         // 回る速さ(ラジアン/秒。0で止まる)
    bool spatial = true;                             // 位置で聞こえ方を変えるか
    DirectX::XMFLOAT3 shotPosition{};                // SPACEで鳴らした場所
    float shotTimer = 0.f;                           // 鳴らした場所の輪を出している残り時間

    static constexpr float shotRingSeconds = 1.2f;   // 鳴らした場所の輪を出す時間
    static constexpr float maxOrbitSpeed = 2.4f;     // 回る速さの上限

    DirectX::XMFLOAT2 GetForward() const;            // 聞く側の正面(平面の向き。長さ1)
    DirectX::XMFLOAT2 GetRight() const;              // 聞く側の右(平面の向き。長さ1)
    void PlaceOnPlane(GameObject* object, DirectX::XMFLOAT2 point);   // 物を平面の点に置く(地面・手前に収まる高さで)

public:
    explicit Sound3DDemoController(DemoMode demoMode) : mode(demoMode), field(GetDemoField(demoMode)) {}

    // 音源・聞く側・音・カメラを、CreateSound3DDemo()から渡してもらう(Init の後)
    void Setup(GameObject* emitterObject, GameObject* listenerObject, AudioSourceComponent* audioSource, DemoCameraController* cameraController);

    void Update(float dt) override;
    void Draw() override;
};
