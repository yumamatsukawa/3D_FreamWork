#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>
#include <vector>

class GameObject;
class SpriteRenderer;
class MeshRenderer;

// Visibilityデモの操作役。たくさん置いた物が「画面(カメラ)に映っているか」を Visibility で調べて数え、
// 映っていない物は描かない設定(SetFrustumCulling)のオン/オフと、省いた数(GetCulledCount)を見せる。
// 右上の地図に、物(映っている=緑、映っていない=灰色)とカメラの範囲を描く。
// 2D版: WASDで2Dカメラを動かす。円(IsCircleInView2D)と四角(IsRectInView2D)で調べる。
// 3D版: WASDで見る点を動かす(カメラは向いている方へついていく)。球(IsSphereInView3D)と箱(IsBoxInView3D)で調べる。
// 2Dカメラを動かすので、出る時(Uninit)に原点へ戻す
class VisibilityDemoController : public Component {
public:
    // 調べ方(物ごとに2種類を使い分ける)
    enum class Shape { Round, Square };

private:
    // 調べる物1つ分
    struct Item {
        GameObject* object;        // 持ち主
        SpriteRenderer* sprite;    // 2D版の見た目(3D版はnullptr)
        MeshRenderer* mesh;        // 3D版の見た目(2D版はnullptr)
        Shape shape;               // 円/球か、四角/箱か
        float size;                // 円/球の半径、四角/箱の半分の大きさ
    };

    DemoMode mode;                 // 2D版か3D版か
    std::vector<Item> items;       // 調べる物の一覧
    GameObject* cameraTarget = nullptr;   // 3D版: カメラが見ている点のオブジェクト(WASDで動かす)
    bool cullingOn = true;         // 画面の外の物を描かないか(1キー。SetFrustumCulling)

    void ApplyCulling();           // 全部の物に、描かない設定を入れる
    void MoveCamera(float dt);     // WASD・矢印でカメラを動かす
    bool IsInView(const Item& item) const;   // 画面に映っているか(Visibility)
    void DrawMap();                // 右上の地図

public:
    explicit VisibilityDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 調べる物を足す(CreateVisibilityDemo()から)
    void AddItem(GameObject* object, SpriteRenderer* sprite, MeshRenderer* mesh, Shape shape, float size) {
        items.push_back({ object, sprite, mesh, shape, size });
    }
    // 3D版: 見る点のオブジェクトを渡す
    void SetCameraTarget(GameObject* target) { cameraTarget = target; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
