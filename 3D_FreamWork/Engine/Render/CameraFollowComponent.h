#pragma once
#include "../Core/Component.h"
#include <DirectXMath.h>

using namespace DirectX;

// 付けたオブジェクトを、2D用カメラ(Image::GetCamera())が追いかけるComponent。Playerなどに付ける。
// カメラの position の x,y だけを動かす(z・target はそのまま)。対象の位置はワールド座標(親がいても正しく追う)
//
// 使い方:
//   auto* follow = player->AddComponent<CameraFollowComponent>();
//   follow->SetSmoothTime(0.15f);                          // 少し遅れてなめらかに追う(既定0=ピッタリ)
//   follow->SetBounds({ -2000.f, -2000.f }, { 2000.f, 2000.f });  // ステージの外を映さない
//
// 決まり:
// - Componentは付けた順にUpdateされる。DynamicのRigidbodyより後(PhysXの結果が反映された位置で追う)、
//   Input::GetMouseWorldPosition() を使うComponentより前に AddComponent する(画面に見えているマウスの位置とずれない)
// - 最初のUpdateと、SetActive(true)で戻った時は、なめらかに追わずに一気に合わせる(遠くから流れてこない)
// - 非アクティブの間はUpdateされないので、カメラはその場で止まる。2つ以上のオブジェクトに付けると、後からUpdateされた方が勝つ
// - なめらかに追う時は Update の dt(ポーズ中は0)で進むので、ポーズ中はカメラも止まる
class CameraFollowComponent : public Component {
private:
    XMFLOAT2 offset{ 0.f, 0.f };     // 対象からどれだけずらして映すか
    float smoothTime = 0.f;          // 追いつくまでの時間(秒)。0=ピッタリ
    bool hasBounds = false;          // SetBoundsで範囲を決めたか
    XMFLOAT2 boundsMin{ 0.f, 0.f };  // 画面に映してよい範囲の左下
    XMFLOAT2 boundsMax{ 0.f, 0.f };  // 画面に映してよい範囲の右上
    XMFLOAT2 deadZone{ 0.f, 0.f };   // カメラを動かさない範囲の大きさ(幅, 高さ)
    XMFLOAT2 goal{ 0.f, 0.f };       // カメラが向かう位置(デッドゾーン・範囲を考えた後)
    bool snapNext = true;            // 次のUpdateで一気に合わせるか

    XMFLOAT2 ClampToBounds(XMFLOAT2 center) const;  // 画面に映る範囲がBoundsの中に収まるように、カメラの中心を制限する

public:
    void Update(float dt) override;
    void OnActiveChanged(bool active) override;

    // 対象からどれだけずらして映すか(例: {0, 100} で対象より上を映す=対象は画面の少し下に見える)
    void SetOffset(XMFLOAT2 value) { offset = value; }
    // 追いつくまでの時間(秒)。0=ピッタリ追従。この秒数で、残りの距離が約5%になる(フレームレートが変わっても同じ動き)。負の値は0
    void SetSmoothTime(float seconds) { smoothTime = (seconds > 0.f) ? seconds : 0.f; }
    // 画面に映してよい範囲(Z=0の平面のワールド座標。左下・右上の角)。画面の端がこの外に出ないところでカメラが止まる。
    // 範囲が画面より小さい向きは、範囲の真ん中に固定する
    void SetBounds(XMFLOAT2 minCorner, XMFLOAT2 maxCorner);
    // この大きさ(幅, 高さ)の四角の中で対象が動いても、カメラは動かさない(画面酔い対策)。四角から出た分だけ動く。負の値は0
    // 四角の中心は、対象がいつも映る位置(画面中央。SetOffsetを使った時はそこからずらした位置)
    void SetDeadZone(XMFLOAT2 size);
};
