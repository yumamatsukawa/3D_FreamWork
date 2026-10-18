#pragma once
#include "../Core/Component.h"

// 音を「聞く側」を表すComponent。Player(かカメラ)に1つだけ付ける。
// 付けたオブジェクトのワールド位置が「聞く位置」になり、AudioSourceComponent / Audio::PlayOneShotAt の音は
// ここからの距離で小さくなり、左右の位置で聞こえる向きが変わる
//
// 使い方:
//   player->AddComponent<AudioListenerComponent>();                 // 2D: 画面の上が正面、右が右(向きは固定)
//   player->AddComponent<AudioListenerComponent>()->SetUse3DCamera(true);   // 3D: 3D用カメラの向きで聞く
//
// 決まり:
// - 2つ以上ある時: 最後に有効になった(AddComponent / SetActive(true) された)1つだけが使われ、出力ウィンドウで1回知らせる
// - 1つも無い時: 2D用カメラ(Image::GetCamera())のx,yが「聞く位置」になる(2Dゲームは付けなくても動く)。
//   3Dゲームは、付けないと2D用カメラの位置で聞くことになってしまうので、必ずPlayerなどに付ける
// - 非アクティブの間と、オブジェクトが破棄された後は聞く位置にならない
class AudioListenerComponent : public Component {
private:
    bool use3DCamera = false;   // trueなら向きを3D用カメラに合わせる(falseは2D用の固定の向き)

public:
    void Init() override;
    void Update(float dt) override;
    void Uninit() override;
    void OnActiveChanged(bool active) override;

    // 向きを3D用カメラ(Image::GetCamera3D())に合わせるか(3Dゲーム用)。位置はいつもこのオブジェクトの位置
    void SetUse3DCamera(bool enabled) { use3DCamera = enabled; }
};
