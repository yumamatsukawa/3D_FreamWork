#pragma once
#include "../../Engine/Component.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/Image.h"

// 毎フレーム、3D用カメラの位置に自分の位置を合わせる。
// 回転はしないので、常に「無限に遠い背景」のように見える(スカイボックス用)
class SkyboxFollowComponent : public Component {
public:
    void Update(float dt) override {
        GetOwner()->transform.position = Image::GetCamera3D().position;
    }
};
