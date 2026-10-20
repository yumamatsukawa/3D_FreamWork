#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Physics/Physics.h"
#include <DirectXMath.h>
#include <vector>

class GameObject;
class MeshRenderer;

// Coordデモの操作役。画面の座標とワールドの座標を行き来する(Image::WorldToScreen2D / ScreenToWorld2D / WorldToScreen3D / ScreenPointToRay3D)。
// 2D版: マウスの位置をワールドの位置に直して印を置き、WASDで2Dカメラを動かして見え方が変わるのを見る。2Dは奥(z)の違う物も、同じ大きさ・同じ位置の計算で見える(zは使わない)。
// 3D版: マウスの線を Physics::Raycast に渡して、マウスの下の物を探す。物の頭上に、WorldToScreen3D で文字を出す。
// 2Dカメラを動かすので、出る時(Uninit)に原点へ戻す
class CoordDemoController : public Component {
private:
    static constexpr size_t maxMarks = 30;     // 2D: 置ける印の数

    // 3D: 調べられる物1つ分
    struct Target {
        GameObject* object;                    // 持ち主
        MeshRenderer* mesh;                    // 見た目(マウスが乗ったら明るくする)
        DirectX::XMFLOAT4 baseColor;           // 元の色
    };

    DemoMode mode;                             // 2D版か3D版か

    // 2D版の状態
    std::vector<GameObject*> depthSprites;     // 奥行き(z)だけが違う3つのスプライト
    std::vector<DirectX::XMFLOAT2> marks;      // 置いた印のワールドの位置(Z=0の平面の上)

    // 3D版の状態
    std::vector<Target> targets;               // 調べられる物
    bool hasHit = false;                       // マウスの線が何かに当たっているか
    RaycastHit hit;                            // 当たった物の情報
    GameObject* selected = nullptr;            // クリックで選んだ物

    void Update2D(float dt);
    void Update3D();
    void Draw2D();
    void Draw3D();

public:
    explicit CoordDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 2D版: 奥行きの違うスプライトを足す(CreateCoordDemo()から)
    void AddDepthSprite(GameObject* object) { depthSprites.push_back(object); }
    // 3D版: 調べられる物を足す
    void AddTarget(GameObject* object, MeshRenderer* mesh);

    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
