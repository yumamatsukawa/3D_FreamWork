#pragma once
#include <DirectXMath.h>

using namespace DirectX;

// 「カメラに映っているか」を調べる関数をまとめたもの(画面の外にある物を描かない・処理を省く時に使う)。
// 敵の視野(扇形)は VisionComponent を使う。
//
// 決まり:
// - 3Dは「視錐台(カメラに映る台形の空間)」と、物の大きさ(バウンディング球・箱)で調べる。少しでも入っていれば true
// - 2Dは「2Dカメラの画面(画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル)」と、円・四角で調べる。
//   SpriteRenderer(UIモード)と同じ計算(zは大きさに影響しない。奥にあっても同じ大きさで見える)
// - 3Dの視錐台は1フレームに1回(GameEngineが BeginFrame で)作る。ゲーム側は呼ばなくてよい。
//   BeginFrame より前(最初のフレームなど)は、判定できないので「映っている(true)」を返す
// - Updateの中で呼ぶと、1つ前のフレームの描画に使ったカメラで判定する(敵のAIを省く程度なら問題ない)。
//   Drawの中で呼ぶと、そのフレームの描画に使うカメラ(CameraShakeの揺れ込み)で判定する
// - 描画を省くだけで、物理(当たり判定)は止まらない。物が突然消えて見えたら、半径が小さすぎる(GetCulledCountで確認)
namespace Visibility {
    // ── 3D: 今の3Dカメラ(Image::GetCamera3D())の視錐台に、球・箱が少しでも入っているか ──
    // 例: if (!Visibility::IsSphereInView3D(enemy->transform.position, 100.f)) return;   // 画面の外なら何もしない
    bool IsSphereInView3D(XMFLOAT3 center, float radius);
    bool IsBoxInView3D(XMFLOAT3 center, XMFLOAT3 halfSize);      // 軸に沿った箱(回転しない)。halfSizeは辺の長さの半分

    // ── 2D: 今の2Dカメラ(Image::GetCamera())の画面に、円・四角が少しでも入っているか ──
    // position: ワールド座標(zは使わない)。radius / halfSize: ワールドでの大きさ(四角は回転しない)
    // 例: if (!Visibility::IsCircleInView2D(bullet->transform.position, 8.f)) { /* 画面の外 */ }
    bool IsCircleInView2D(XMFLOAT3 position, float radius);
    bool IsRectInView2D(XMFLOAT3 position, XMFLOAT2 halfSize);

    // 前のフレームで、描かずに省いた数(確認用。HUDに出して、ちゃんと省けているか見る)
    int GetCulledCount();

    // ─── Engine内部用(GameEngineと、SpriteRenderer / MeshRenderer / ModelRenderer だけが呼ぶ) ───
    // 視錐台をそのフレームのカメラから作り直し、省いた数を数え直す。CameraShake::BeginDraw() の後に1回
    void BeginFrame();
    // 描かずに省いた数を1つ増やす
    void CountCulled();
    // ワールド行列(位置・拡大)を持つ物の、画面判定。localRadiusは「原点から一番遠い頂点までの距離」(拡大する前)。
    // 球の中心は行列の位置、半径は localRadius × 行列の一番大きい拡大率
    bool IsTransformedSphereInView3D(const XMMATRIX& world, float localRadius);
}
