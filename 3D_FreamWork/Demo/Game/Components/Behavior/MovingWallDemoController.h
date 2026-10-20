#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../Common/DemoPlane.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>
#include <vector>

class GameObject;
class DoorController;
class NavGridComponent;
class PathFollowComponent;

// MovingWallデモの操作役。動く壁(ドア)で、経路が変わるところを見せる。
// 1 でドアを開け閉めする(Kinematic の壁)。格子は SetAutoRebakeInterval で、一定の間隔で自動で焼き直す(3 で入り切り。切ると、4 の RequestRebake で焼くまで格子は古いまま)。
// 2 でクリックの意味を切り替える: 目的地を置く / 柵を置く。柵は物理の無い障害物で、NavGrid::SetBlockedBox で格子だけに登録する(C で全部消す)。
// 緑がドアの通り道にいる間は、ドアを閉められない(挟まって動けなくなるのを防ぐ)。F1 で格子と経路の線、R で緑を最初の位置へ戻す。動く物は Dynamic なので、R は ResetDemoBody を使う
class MovingWallDemoController : public Component {
private:
    static constexpr size_t maxFences = 6;          // 置ける柵の数

    DemoMode mode;                                  // 2D版か3D版か
    DemoField field;                                // 競技場(目印・柵はこの中に収める)
    GameObject* agent = nullptr;                    // 緑: 経路を探して歩く
    GameObject* goal = nullptr;                     // 目的地の目印
    DoorController* door = nullptr;                 // ドア
    NavGridComponent* navigation = nullptr;         // 格子
    PathFollowComponent* follow = nullptr;          // 緑の動き
    DirectX::XMFLOAT3 agentStart{};                 // 緑の出発点(Rで戻す場所)
    std::vector<DirectX::XMFLOAT2> fences;          // 置いた柵の中心(平面の点。古い順)
    DirectX::XMFLOAT3 fenceSize{};                  // 柵の大きさ(SetBlockedBox に渡す)
    bool autoRebake = true;                         // 自動で焼き直すか(3)
    bool placeFence = false;                        // クリックで柵を置くか(2。false なら目的地)
    bool gridVisible = true;                        // 格子と経路の線を出すか(F1)
    float refuseTimer = 0.f;                        // ドアを閉められなかった時の知らせを出す残り時間(秒)

    void ApplyFence(const DirectX::XMFLOAT2& center, bool blocked);   // 柵1つを格子に登録する/外す
    void AddFence(const DirectX::XMFLOAT2& center);                   // 柵を置く(上限を超えたら、一番古いものを外す)
    void ClearFences();                                               // 柵を全部外す

public:
    explicit MovingWallDemoController(DemoMode demoMode) : mode(demoMode), field(GetDemoField(demoMode)) {}

    // 動く物・目印・ドア・格子を、CreateMovingWallDemo()から渡してもらう(Init の後)
    void Setup(GameObject* agentObject, GameObject* goalObject, DoorController* doorController, NavGridComponent* nav);

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
