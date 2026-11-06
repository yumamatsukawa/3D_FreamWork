#pragma once
#include "../../../../Engine/Core/Component.h"
#include <functional>

// ネットワークで同期する物(剛体付き)を、役割(ホスト/クライアント)に合わせて作り直す役。
// ホストの物はDynamic、クライアントのコピーはKinematicで作る決まりなので(Network::IsHost() を見て決める)、
// このデモのように「シーンの中で Host や Join を押す」場合は、役割が変わった時に作り直す。
// (ロビーのシーンで先につないでからゲームのシーンへ移る作りなら、この作り直しは要らない)
class NetworkWorldController : public Component {
private:
    std::function<void()> build;     // 同期する物を全部作る関数
    bool builtAsClient = false;      // 今ある物は、クライアント用に作った物か
    bool needBuild = false;          // 片付けた後、次のフレームで作り直す印(古い物のUninitが終わってから作る)

    void Destroy();                  // 今ある同期する物(シーンに置いた物)を片付ける

public:
    explicit NetworkWorldController(std::function<void()> buildFunction) : build(std::move(buildFunction)) {}

    void Init() override;
    void Update(float dt) override;
};