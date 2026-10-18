#pragma once
#include "../Core/Component.h"
#include "Anchor.h"
#include <DirectXMath.h>
#include <functional>

class GameObject;

// HPバーなどの「帯のゲージ」を描くComponent。値を表示するだけで、値の持ち主にはならない
// (HPは PlayerController など持ち主が持ち、ゲージは SetValueSource で読んで表示するだけ)。
//
// 使い方:
//   auto* gauge = hud->AddComponent<GaugeRenderer>();
//   gauge->SetAnchor(Anchor::TopLeft, { 80.f, -135.f });     // 画面の左上から右に80、下に135(左上の角がそこに来る)
//   gauge->SetSize(240.f, 16.f);                             // 幅と高さ
//   gauge->SetDelayedBar(true, { 1.f, 0.9f, 0.4f, 1.f });    // 減った分を少し遅れて縮める
//   gauge->SetValueSource([p] { return (float)p->GetHp(); }, [p] { return (float)p->GetMaxHp(); });   // 毎フレーム読んで表示
//
//   // 敵や自分の頭の上に出す時(2D)
//   gauge->SetFollowTarget(player, { 0.f, 70.f });           // playerの位置から、ワールドで上に70
//   // 3Dのオブジェクトの時は、3D用カメラで画面座標に直す
//   gauge->SetUse3DCamera(true);
//
// 決まり:
// - 帯は Direct2D(Image::DrawRect)で描くので、カメラの影響を受けず、画像や3Dの後に重なる。画面座標は「画面中央が(0,0)、Y+=上」
// - SetAnchor(画面に固定): ゲージの anchor 側の角(辺)が、GetAnchoredPosition(anchor, offset) に合う(TopLeft なら左上の角。はみ出さない)
// - SetFollowTarget(オブジェクトの上に表示): ゲージの「中心」が、オブジェクトの位置+offset(ワールドでずらす量)の画面座標に来る。
//   2Dは Image::WorldToScreen2D(2D用カメラ)、SetUse3DCamera(true) なら Image::WorldToScreen3D(3D用カメラ。カメラの後ろにある時は描かない)
//   target が非アクティブ、またはDestroyされている(まだ消えていない間)なら描かない。
//   ★ target は、このゲージより長く生きるオブジェクトにする(ゲージを付けたオブジェクト自身が target なら安全)。
//     別のオブジェクトを target にして、そのオブジェクトが消えてしまう場合は、先に SetFollowTarget(nullptr, ...) か、ゲージ側もDestroyする
//     (ポインタだけでは、消えたかどうかを調べられないため)。nullptr の間は描かない
// - 減った分を遅れて縮める帯(SetDelayedBar)は、減った時から delayedHoldTime 秒は止まり、その後 delayedShrinkSpeed の速さで縮む。
//   増えた時はすぐに追いつく。時間は Update の dt で進む(ポーズ中は止まり、スローの間は遅くなる)
// - 最大値が0以下なら、中身は空として描く。現在の値は 0〜最大値 に収めて描く
class GaugeRenderer : public Component {
private:
    // 画面のどこに描くか
    enum class PositionMode { Anchored, FollowTarget };

    static constexpr float delayedHoldTime = 0.4f;      // 減った後、遅れて縮む帯が縮み始めるまでの時間(秒)
    static constexpr float delayedShrinkSpeed = 0.5f;   // 遅れて縮む帯が縮む速さ(ゲージ全体に対する割合 / 秒。0.5なら全体を2秒で縮む)

    // 値
    float ratio = 1.f;            // 今の割合(0〜1)
    float delayedRatio = 1.f;     // 遅れて縮む帯の割合(0〜1。ratio以上)
    float delayedHold = 0.f;      // 縮み始めるまでの残り時間(秒)
    bool hasValue = false;        // 一度でも値を設定したか(最初の値では、縮む動きをしない)
    std::function<float()> getCurrent;   // 今の値を読む関数(SetValueSource)
    std::function<float()> getMax;       // 最大値を読む関数(SetValueSource)

    // 見た目
    DirectX::XMFLOAT4 fillColor{ 0.3f, 0.85f, 0.3f, 1.f };         // 中身の色
    DirectX::XMFLOAT4 backgroundColor{ 0.f, 0.f, 0.f, 0.6f };      // 背景の色
    bool delayedEnabled = false;                                    // 減った分を遅れて縮めるか
    DirectX::XMFLOAT4 delayedColor{ 1.f, 0.9f, 0.4f, 1.f };         // 遅れて縮む帯の色
    DirectX::XMFLOAT2 size{ 200.f, 20.f };                          // 幅と高さ(ピクセル)

    // 位置
    PositionMode positionMode = PositionMode::Anchored;
    Anchor anchor = Anchor::Center;                 // SetAnchor: 画面のどこを基準にするか
    DirectX::XMFLOAT2 anchorOffset{ 0.f, 0.f };     // SetAnchor: 基準からずらす量(画面座標)
    GameObject* target = nullptr;                   // SetFollowTarget: 追いかけるオブジェクト
    DirectX::XMFLOAT2 targetOffset{ 0.f, 0.f };     // SetFollowTarget: オブジェクトの位置からずらす量(ワールド)
    bool use3DCamera = false;                       // trueなら3D用カメラで画面座標に直す

    // 描く中心の画面座標を求める。描かない時(targetが無い・カメラの後ろなど)はfalse
    bool GetScreenCenter(DirectX::XMFLOAT2& center) const;

public:
    // ─── 値 ─────────────
    void SetValue(float current, float max);   // 今の値と最大値(中身の割合 = current / max)
    // 毎フレーム自動で読む関数を決める(値の持ち主のゲッターを渡す)。決めると、Updateのたびに SetValue(getCurrent(), getMax()) される
    void SetValueSource(std::function<float()> currentGetter, std::function<float()> maxGetter);
    float GetRatio() const { return ratio; }                  // 中身の割合(0〜1)
    float GetDelayedRatio() const { return delayedRatio; }    // 遅れて縮む帯の割合(0〜1)

    // ─── 見た目 ─────────────
    void SetColors(DirectX::XMFLOAT4 fill, DirectX::XMFLOAT4 background);   // 中身の色と背景の色
    void SetDelayedBar(bool enabled, DirectX::XMFLOAT4 color);              // 減った分を少し遅れて縮める(ダメージ量が分かる)
    void SetSize(float width, float height);                                 // 幅と高さ(ピクセル。0以下は無視)

    // ─── 位置 ─────────────
    void SetAnchor(Anchor anchorPoint, DirectX::XMFLOAT2 offset);           // 画面に固定して表示(左上のHPなど)
    void SetFollowTarget(GameObject* followTarget, DirectX::XMFLOAT2 offset);   // オブジェクトの上に表示(敵の頭上)。offsetはワールドでずらす量
    void SetUse3DCamera(bool enabled) { use3DCamera = enabled; }            // 3Dのオブジェクトを追う時はtrue

    void Update(float dt) override;
    void Draw() override;
};
