#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../Components/Common/DemoPlane.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <DirectXMath.h>
#include <string>

class Scene;
class GameObject;
class NavGridComponent;

// 動きとAI(Behavior)のデモで使う、動く物・目印・壁・格子を作る道具(2D版と3D版で共有。違いは引数 mode)。
// 位置は「平面の点」(2Dは x,y、3Dは x,z)で渡す。3Dは地面の上に置く

// 動く物1つ分の設計図(円・球。ChaseComponent などを付ける前の、見た目と剛体だけ)
struct DemoAgentDesc {
    DirectX::XMFLOAT2 position{};          // 置く場所(平面の点)
    float size = 60.f;                     // 直径
    BodyType type = BodyType::Dynamic;     // Dynamic=物理で動く(壁にぶつかる) / Kinematic=Transformを直接動かす(壁をすり抜ける)
    float baseY = demoGroundTop;           // 3D: 立つ地面の高さ(地形の上に置く時は、その場所の地形の高さを入れる)
    int cell = 6;                          // 色(box.png の何番目の絵か。2Dの絵に合わせて、3Dも同じ色にする)
    std::string name = "DemoAgent";        // オブジェクトの名前
    std::string tag;                       // オブジェクトのタグ
};

// box.png の絵(0〜8)に近い色。3Dの球の色に使う
DirectX::XMFLOAT4 GetDemoCellColor(int cell);

// 動く物を作る(2Dは円のスプライト+円の剛体、3Dは球のメッシュ+球の剛体)。
// Dynamicの2Dは重力オフ・Z固定・回転固定、Dynamicの3Dは回転固定(転がらない)。重力は3Dだけ効く
GameObject* CreateDemoAgent(Scene& scene, DemoMode mode, const DemoAgentDesc& desc);

// 動く物の色(2Dは絵の番号、3Dは色)を変える
void SetDemoAgentCell(GameObject* agent, DemoMode mode, int cell);

// 目印(目的地・目標)を作る。物理は無い小さな円・球。位置は PlaceDemoGoal で動かす
GameObject* CreateDemoGoal(Scene& scene, DemoMode mode, DirectX::XMFLOAT2 position, int cell, const std::string& name);
void PlaceDemoGoal(GameObject* goal, DemoMode mode, DirectX::XMFLOAT2 position);

// 壁(動かない板・箱。タグ "Wall")を1つ作る。VisionComponent の視線と NavGrid の障害物は、このタグの物。
// center・size: 平面の点(3Dの size は 横と奥行き)、height: 3Dの壁の高さ
GameObject* CreateDemoObstacle(Scene& scene, DemoMode mode, DirectX::XMFLOAT2 center, DirectX::XMFLOAT2 size, float height = 200.f);

// 競技場(field の外側を壁4つで囲む。壁はタグ "Wall")を作る
void CreateDemoField(Scene& scene, DemoMode mode, const DemoField& field);

// 経路探索の格子(NavGridComponent)を、競技場の内側に作る。壁を置いた後に呼ぶ(壁は、最初のフレームの後に自動で読まれる)
NavGridComponent* CreateDemoNavigation(Scene& scene, DemoMode mode, const DemoField& field, float cellSize, float agentRadius);
