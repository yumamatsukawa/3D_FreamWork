#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../Objects/Common/DemoBody.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>
#include <string>
#include <vector>

class GameObject;

// Colliderデモの操作役。クリックで、選んだ形(円/球・四角/箱・カプセル)の剛体(Dynamic)を出し、
// 固定の見本(Static)・動く台(Kinematic)とぶつかる様子で、BodyType と当たり判定の形を見せる。
// F1 で当たり判定の枠線(RigidbodyComponent::SetDebugDrawEnabled)のオン/オフ。
// 枠線は全体の設定なので、出る時(Uninit)にオフへ戻す。
// 2D版: 重力が無い上から見た世界なので、勢いをつけて出す。3D版: マウスの下の地面の上から落とす
class ColliderDemoController : public Component {
private:
    // 見本の名札(位置は名札を出すワールドの位置)
    struct Label {
        DirectX::XMFLOAT3 position;
        std::wstring text;
    };

    // クリックで出した物。出した直後は剛体が位置を合わせ直して速さを0にするので、数フレーム後に速さを入れる
    struct Spawned {
        GameObject* object;
        int frames;                       // 出してからのフレーム数
        DirectX::XMFLOAT3 velocity;       // 入れる速さ(3Dは0のまま)
    };

    DemoMode mode;                        // 2D版か3D版か
    DemoShape shape = DemoShape::Round;   // 出す形(1・2・3キー)
    bool debugDraw = true;                // 当たり判定の枠線(F1キー)
    std::vector<Label> labels;            // 見本の名札
    std::vector<Spawned> spawned;         // 出した物(古い順)
    GameObject* mover = nullptr;          // 動く台(Kinematic。左右に往復する)
    DirectX::XMFLOAT3 moverCenter{};      // 動く台の往復の中心
    float moverRange = 0.f;               // 動く台の往復の幅(中心から片側)
    DirectX::XMFLOAT2 arenaMin{};         // 2D版: 物を出せる範囲(壁の内側)
    DirectX::XMFLOAT2 arenaMax{};
    float groundY = 0.f;                  // 3D版: マウスの下の地面の高さ

    void Spawn();                         // クリックした場所に、選んだ形の物を出す
    void ClearBodies();                   // 出した物を全部消す
    void LaunchSpawned();                 // 出した直後の物に、速さを入れる
    const wchar_t* GetShapeName() const;  // 選んだ形の名前(今の側のコライダーの名前)

public:
    explicit ColliderDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 見本の名札を足す・動く台を渡す・出せる範囲を決める(CreateColliderDemo()から。Init の後)
    void AddLabel(DirectX::XMFLOAT3 position, const std::wstring& text) { labels.push_back({ position, text }); }
    void SetMover(GameObject* object, DirectX::XMFLOAT3 center, float range) { mover = object; moverCenter = center; moverRange = range; }
    void SetArena2D(DirectX::XMFLOAT2 min, DirectX::XMFLOAT2 max) { arenaMin = min; arenaMax = max; }
    void SetGroundY(float y) { groundY = y; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
