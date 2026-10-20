#pragma once
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <DirectXMath.h>
#include <string>

class Scene;
class GameObject;

// 物理のデモで使う剛体の形(2Dと3Dで、名前の対応は 円=Circle/球=Sphere、四角=Square/箱=Box、カプセル=Capsule2D/Capsule)
enum class DemoShape { Round, Block, Capsule };

// 剛体付きの物1つ分の設計図(CreateDemoBody2D / CreateDemoBody3D に渡す)
struct DemoBodyDesc {
    DemoShape shape = DemoShape::Block;
    BodyType type = BodyType::Dynamic;          // Static=動かない / Kinematic=コードで動かす / Dynamic=物理で動く
    DirectX::XMFLOAT3 position{};               // 置く場所(2Dはzを使わず、手前(z=0)に置く)
    float size = 60.f;                          // 円・球=直径、四角・箱=一辺、カプセル=幅(直径)。カプセルの高さは幅の2倍
    DirectX::XMFLOAT4 color{ 1.f, 1.f, 1.f, 1.f };   // 色(3D)。2Dは四角の色合いだけに使う
    int cell = 6;                               // 2Dの円・カプセル: box.png の何番目の絵を使うか(0〜8。色が違う)
    bool isTrigger = false;                     // true=すり抜ける判定(半透明に描く)
    bool canRotate = false;                     // 2Dの動く物: true=回転できる(既定は回転を固定する)
    std::string name = "DemoBody";              // オブジェクトの名前
    std::string tag;                            // オブジェクトのタグ(Physics::Raycast などで絞る時に使う)
};

// 2Dの剛体付きの物を作る(見た目は2Dモードのスプライト、zは0で手前)。
// 動く物(Dynamic)は、重力オフ・Z固定(奥に動かさない)を必ず付ける。2Dのデモはこの関数で剛体を作る(付け忘れ防止)
GameObject* CreateDemoBody2D(Scene& scene, const DemoBodyDesc& desc);

// 3Dの剛体付きの物を作る(見た目は3Dメッシュ。動く物は重力あり)
GameObject* CreateDemoBody3D(Scene& scene, const DemoBodyDesc& desc);
