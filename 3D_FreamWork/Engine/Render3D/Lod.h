#pragma once
#include <DirectXMath.h>

using namespace DirectX;

// 距離の設定(LOD段階1: 遠い物を「描かない・影を落とさない・枠線を描かない」で軽くする)。
// MeshRenderer / ModelRenderer / TerrainRenderer が1つずつ持つ。0(既定)は「その機能は使わない=今まで通り」。
struct LodSettings {
    float drawDistance = 0.f;      // これより遠い物は描かない(0=いつも描く)
    float shadowDistance = 0.f;    // これより遠い物は影を落とさない(0=いつも落とす)
    float outlineDistance = 0.f;   // これより遠い物は枠線を描かない(0=いつも描く)
};

// 距離で省く判断をまとめたもの。
//
// 決まり:
// - 距離は「カメラ(Image::GetCamera3D().position。CameraShake の揺れ込み)から物の球の表面まで」=「中心までの距離 − 半径」。
//   大きな物は、中心が遠くても手前の端が近ければ省かない。ちょうど設定の距離なら省かない(超えたら省く)
// - 物の中心は、ワールド行列の位置。半径は MeshRenderer なら頂点の一番遠い点(Visibility と同じ)×行列の一番大きい拡大率
// - 描画を省くだけで、物理(当たり判定)は止まらない
// - 影は「カメラからの距離」で判断する。光の向きによっては、遠い物の影が近くの地面に落ちるので、shadowDistance を短くしすぎないこと
// - SetEnabled(false) にすると、すべての物の設定を無視する(LODを使わない状態と比べる確認用。既定はtrue)
// - SetGlobalScale(0.5f) にすると、すべての距離を半分にする(まとめて調整する用。既定は1)
namespace Lod {
    // カメラ(Image::GetCamera3D().position)から、ワールド座標 position までの距離
    float GetCameraDistance(XMFLOAT3 position);

    // 球(center, radius)の表面までの距離(0以上)が、limit を超えているか。limit が 0 以下、または SetEnabled(false) の時は false
    bool IsBeyond(XMFLOAT3 center, float radius, float limit);
    // ワールド行列を持つ物版。localRadius は拡大する前の半径(球の中心は行列の位置、半径は localRadius × 行列の一番大きい拡大率)
    bool IsBeyond(const XMMATRIX& world, float localRadius, float limit);

    void SetEnabled(bool enabled);
    bool IsEnabled();
    void SetGlobalScale(float scale);   // 0以下は1に戻す
    float GetGlobalScale();
}
