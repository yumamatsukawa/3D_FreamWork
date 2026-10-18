#pragma once
#include "../Core/Component.h"
#include "../Render3D/Mesh.h"
#include "MoveComponent.h"
#include <DirectXMath.h>
#include <string>

class GameObject;

// 見張り役の「視野」を持たせるComponent(扇形の中にいて、壁にさえぎられていなければ見える)。
// 見えたかどうかを調べるだけで、見えた後の動き(追いかける・色を変える)は付けた側のComponentが決める。
//
// 決まり:
// - 視野は、目の位置(オーナーの位置 + SetEyeOffset)から、正面に向かう扇形(角度 SetFieldOfView・距離 SetRange)
// - 角度・距離は「見る平面」の上だけで考える(XYなら2D、XZなら3Dの地面。Zの差、Yの差は無視する)
// - 正面は、SetFacingで指定した向きが最優先。指定が無ければ、XZは Math::Forward3D(rotate.y)、XYは Math::Forward2D(rotate.z)
//   ※追いかける敵のようにrotateを使わない物は、毎フレーム SetFacing で進む向きを渡す
// - 壁にさえぎられているかは、目から相手へ Physics::Raycast を飛ばして調べる。SetObstacleTagのタグを付けたオブジェクトだけが壁
//   (壁にタグを付け忘れると素通しになる。タグを""にすると壁は考えない)。地面・ほかの敵・弾は壁にならない
// - 相手の「1点(位置)」だけを見る。相手の中心が壁の陰なら、体が少しはみ出していても見えない
// - Raycastは1つ前のPhysics::Updateの位置を使うので、動いたばかりの壁には1フレーム遅れて反映される
// - 壁の判定(Raycast)は CanSee(GameObject*) では SetCheckInterval の間隔で覚えた結果を使う(角度・距離は毎回調べる)
class VisionComponent : public Component {
private:
    float fieldOfView = 90.f;                    // 視野の角度(全体。度)
    float range = 1000.f;                        // 見える距離
    MovePlane plane = MovePlane::XY;             // 見る平面
    DirectX::XMFLOAT3 eyeOffset{ 0.f, 0.f, 0.f };   // 目の位置(オーナーの位置からの相対)
    std::string obstacleTag = "Wall";            // 視線をさえぎる物のタグ
    DirectX::XMFLOAT3 facing{ 0.f, 0.f, 0.f };   // 手で指定した正面(長さ0なら指定なし)
    float checkInterval = 0.1f;                  // 壁の判定をやり直す間隔(秒)

    // CanSee(GameObject*) が覚えている壁の判定
    mutable GameObject* cachedTarget = nullptr;  // 覚えている相手
    mutable bool cachedVisible = false;          // 壁にさえぎられていなかったか
    mutable float checkTimer = 0.f;              // 次にやり直すまでの残り時間(秒)
    mutable bool seeing = false;                 // 最後の CanSee(GameObject*) の結果(デバッグ表示の色)

    // デバッグ表示(扇形の線)。距離1の扇形を作っておき、Transformで大きさと向きを変える
    Mesh fanMesh;
    float fanMeshFov = -1.f;                     // 今のfanMeshを作った時の角度(違ったら作り直す)
    MovePlane fanMeshPlane = MovePlane::XY;      // 今のfanMeshを作った時の平面(違ったら作り直す)
    bool fanMeshReady = false;

    static bool debugDrawEnabled;

    DirectX::XMFLOAT3 GetEyePosition() const;    // 目のワールド座標
    DirectX::XMFLOAT3 ToPlane(DirectX::XMFLOAT3 v) const;   // 平面の外の成分を0にする
    bool IsBlocked(DirectX::XMFLOAT3 point) const;          // 目から相手の点までの間に壁があるか(Raycast)

public:
    void SetFieldOfView(float degrees);          // 視野の角度(全体。90なら左右45度ずつ。0〜360に収める)
    void SetRange(float distance) { range = (distance > 0.f) ? distance : 0.f; }   // 見える距離
    void SetPlane(MovePlane newPlane) { plane = newPlane; }                        // 見る平面(2D=XY、3Dの地面=XZ。既定はXY)
    void SetEyeOffset(DirectX::XMFLOAT3 offset) { eyeOffset = offset; }            // 目の位置(足元ではなく胸の高さなど)
    void SetObstacleTag(const std::string& tag) { obstacleTag = tag; }             // このタグの物は壁(既定 "Wall")
    void SetFacing(DirectX::XMFLOAT3 direction) { facing = direction; }            // 正面を手で指定する(長さ0に戻すと、transformの向きを使う)
    void SetCheckInterval(float seconds) { checkInterval = (seconds > 0.f) ? seconds : 0.f; }   // 壁の判定の間隔(既定0.1秒。0なら毎回)

    float GetFieldOfView() const { return fieldOfView; }
    float GetRange() const { return range; }

    // 今の正面(長さ1。平面の外の成分は0)。向きが決まらない時は{0,0,0}
    DirectX::XMFLOAT3 GetFacing() const;

    bool IsInCone(DirectX::XMFLOAT3 point) const;     // 扇形(角度と距離)の中か。壁は考えない
    bool CanSee(DirectX::XMFLOAT3 point) const;       // 扇形の中で、壁にさえぎられていないか(呼ぶ度にRaycastする)
    bool CanSee(GameObject* target) const;            // 上と同じ(相手の位置を使う。壁の判定はSetCheckIntervalの間隔で覚える)

    // trueにすると、全てのVisionComponentの扇形を線で描く(見えている時は赤、見えていない時は緑)
    static void SetDebugDrawEnabled(bool enabled) { debugDrawEnabled = enabled; }

    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
