#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class GameObject;
class TerrainRenderer;
class NavGridComponent;
class PathFollowComponent;

// TerrainPathデモの操作役(3D版だけ)。丘の高さ・坂を取り込んだ格子の上を、PathFollowComponent で歩く緑の動く物を見せる。
// クリックで、目的地の目印を丘の表面に置く(マウスの線を、タグ "Terrain" の丘へ Raycast して場所を調べる)。
// 1・2・3 で、登れる坂の角度(NavGridSettings::maxSlopeDegrees)を 15・20・30 度にする。
// 格子の設定は作る時に決まるので、角度を変える時は格子(NavGridComponent)を作り直して、古い格子を Destroy し、歩く側に SetNavigation で渡し直す。
// 通れないマス(急な坂)の数を表示する。F1 で格子と経路の線(NavGridComponent / PathFollowComponent の SetDebugDrawEnabled)。
// 動く物は Dynamic なので、R で戻す時は ResetDemoBody を使う
class TerrainPathDemoController : public Component {
private:
    TerrainRenderer* terrain = nullptr;             // 丘(高さを調べる)
    GameObject* agent = nullptr;                    // 緑: 経路を探して歩く
    GameObject* goal = nullptr;                     // 目的地の目印
    PathFollowComponent* follow = nullptr;          // 緑の動き
    NavGridComponent* navigation = nullptr;         // 今の格子
    DirectX::XMFLOAT3 agentStart{};                 // 緑の出発点(Rで戻す場所)
    float slopeDegrees = 40.f;                      // 登れる坂の角度(1・2・3)
    float speed = 300.f;                            // 速さ(↑↓)
    bool gridVisible = true;                        // 格子と経路の線を出すか(F1)

    // 通れないマスの数(格子が変わった時だけ数え直す)
    int blockedCount = 0;
    int countedVersion = -1;
    NavGridComponent* countedGrid = nullptr;

    void RebuildNavigation();                       // 登れる坂の角度を変えた格子を作り直す
    bool FindTerrainPoint(DirectX::XMFLOAT3& point) const;   // マウスの下の、丘の表面の位置(Raycast)

public:
    // 丘・動く物・目印・格子を、CreateTerrainPathDemo()から渡してもらう(Init の後)
    void Setup(TerrainRenderer* terrainRenderer, GameObject* agentObject, GameObject* goalObject, NavGridComponent* nav, float startSlope);

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
