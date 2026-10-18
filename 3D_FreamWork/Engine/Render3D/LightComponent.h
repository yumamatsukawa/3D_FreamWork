#pragma once
#include "../Core/Component.h"
#include <DirectXMath.h>

// 光の種類
enum class LightType {
    Point,   // 点光源: ランタンのように、周り全体を照らす
    Spot,    // スポットライト: 懐中電灯のように、円すい形に照らす
};

// 付けたオブジェクトの位置から光る、点光源・スポットライトのComponent(3Dのメッシュだけに効く。2Dのスプライトには効かない)。
// 平行光源(Image::GetLight())は今まで通り。これは「その場所の周りだけを照らす光」を足す。
//
// 決まり:
// - 位置はオーナーのワールド座標。オブジェクトと一緒に動く。SetActive(false)・SetEnabled(false)・Destroyで消える
// - 同時に使える数は Lighting::maxLights(8)。超えたら、3Dカメラに近い8個だけが光る(残りは光らない。超えた時は出力ウィンドウに警告が1回出る)
// - 減衰は「range でちょうど0になる」式: atten = saturate(1 - (距離/range)^2)^2
// - 光が当たる強さは 色 × intensity × atten × saturate(dot(法線, 光への向き))。マテリアルの光沢(specular)も同じ光で光る。
//   Material::lit = false の物は光の影響を受けない
// - スポットの向きは SetDirection で指定。指定しなければオーナーの水平の正面(Math::Forward3D)
// - 2Dのスプライトには効かない(Shader.hlsl は光の計算をしない)
// - 影: スポットライトだけ SetCastShadow(true) にすると、Shadow が有効な時に影を落とす(同時に影を作れるスポットは1つ。
//   影を落とす光が複数ある時は、3Dカメラに近い1つだけ)。円すいの外側の角度が60度を超える部分には影が出ない。ポイントライトの影は作れない
class LightComponent : public Component {
private:
    LightType type = LightType::Point;
    DirectX::XMFLOAT3 color{ 1.f, 1.f, 1.f };     // 光の色
    float intensity = 1.f;                        // 強さ(2なら2倍明るい)
    float range = 800.f;                          // 光が届く距離(ここで0になる。100単位=1m)
    DirectX::XMFLOAT3 direction{ 0.f, 0.f, 0.f }; // スポットの向き(長さ0ならオーナーの正面)
    float innerAngle = 20.f;                      // スポットの内側の角度(度。中心からここまでは全開)
    float outerAngle = 35.f;                      // スポットの外側の角度(度。ここで0になる)
    bool castShadow = false;                      // 影を落とすか(スポットライトだけ。Shadow が有効な時に意味がある)

public:
    explicit LightComponent(LightType lightType = LightType::Point) : type(lightType) {}

    void SetColor(float r, float g, float b) { color = { r, g, b }; }          // 光の色(既定 白)
    void SetIntensity(float value) { intensity = (value > 0.f) ? value : 0.f; } // 強さ(既定 1)
    void SetRange(float distance) { range = (distance > 0.01f) ? distance : 0.01f; }   // 光が届く距離(既定 800)

    // スポットライトだけ
    void SetDirection(DirectX::XMFLOAT3 newDirection) { direction = newDirection; }   // 向き(長さ0に戻すと、オーナーの正面を使う)
    void SetSpotAngle(float innerDegrees, float outerDegrees);                        // 内側(全開)と外側(消える)の角度。内側 <= 外側(既定 20 / 35)

    // 影を落とす光にする(スポットライトだけ。既定はfalse)。Shadow::GetSettings().enabled も true の時に影が出る
    void SetCastShadow(bool value) { castShadow = value; }
    bool IsCastingShadow() const { return castShadow; }

    LightType GetLightType() const { return type; }
    DirectX::XMFLOAT3 GetColor() const { return color; }
    float GetIntensity() const { return intensity; }
    float GetRange() const { return range; }
    float GetInnerAngle() const { return innerAngle; }
    float GetOuterAngle() const { return outerAngle; }

    DirectX::XMFLOAT3 GetWorldPosition() const;    // 光の位置(オーナーのワールド座標)
    DirectX::XMFLOAT3 GetWorldDirection() const;   // スポットの向き(長さ1)。指定が無ければオーナーの正面

    ~LightComponent() override;
    void Init() override;     // Lighting に登録する
    void Uninit() override;   // 登録を外す
};
