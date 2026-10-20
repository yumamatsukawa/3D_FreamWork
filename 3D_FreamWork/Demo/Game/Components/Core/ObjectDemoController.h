#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Core/Timer.h"
#include <DirectXMath.h>
#include <vector>

class GameObject;

// Objectデモの操作役。キーで中央の物に部品(Component)を付けたり止めたりし、クリックで小さな物を出して一定時間後にDestroyする。状態は文字で出す
class ObjectDemoController : public Component {
private:
    // クリックで出した物(時間が来たらDestroyする)
    struct Spawned {
        GameObject* object;
        Timer life;
    };

    static constexpr float spawnedLifeTime = 3.f;   // 出した物が消えるまでの時間(秒)
    static constexpr int maxSpawned = 60;           // 同時に出せる数
    static constexpr float groundY = -50.f;         // 3D: 地面の高さ

    DemoMode mode;                    // 2D版か3D版か
    GameObject* target;               // 部品を付け外しされる中央の物
    std::vector<Spawned> spawned;     // 出している物
    int spawnedTotal = 0;             // 今までに出した数

    bool GetClickPosition(DirectX::XMFLOAT3& position) const;   // クリックした場所(2D: マウスの下 / 3D: マウスの線が当たる地面)
    void Spawn(const DirectX::XMFLOAT3& position);              // 小さな物を1つ出す
    void DestroyAllSpawned();                                    // 出した物をすべてDestroyする

public:
    ObjectDemoController(DemoMode demoMode, GameObject* targetObject) : mode(demoMode), target(targetObject) {}

    void Update(float dt) override;
    void Draw() override;
};
