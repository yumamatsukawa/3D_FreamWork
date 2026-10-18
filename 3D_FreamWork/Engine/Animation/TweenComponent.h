#pragma once
#include "../Core/Component.h"
#include "Ease.h"
#include <DirectXMath.h>
#include <functional>
#include <memory>
#include <vector>

// GameObjectの位置・大きさ・回転・色(と好きな値)を、イージングを付けて目標値まで動かすComponent。
//
// 使い方:
//   auto* tween = obj->AddComponent<TweenComponent>();
//   tween->ScaleTo({ 320.f, 96.f, 1.f }, 0.4f, Ease::OutBack);          // 0.4秒でぴょこっと大きくなる
//   tween->MoveTo({ 0.f, 100.f, 0.f }, 1.0f).SetDelay(0.5f).SetOnComplete([] { ... });   // つなげて書ける
//
// 決まり:
// - 開始値は「動き出した時(SetDelayの待ち時間が終わった時)の今の値」。TweenValueだけは引数のfromから始まる
// - SetDelay / SetLoop / SetUnscaled / SetOnComplete は「直前に始めた動き」に効く
// - 位置・大きさ・回転・色・TweenValueを同時に動かせる。同じ種類(位置なら位置)を2つ始めると、前の動きは消えて
//   後から始めた方だけが動く(上書き)。TweenValueは毎回別の値を動かすものなので上書きせず、いくつでも同時に動かせる
// - durationが0以下の動きは、次のUpdateで(SetDelayがあれば待ってから)すぐ1周終わった状態にする(0で割らない)。
//   普通の動きは目標値で終わる。SetLoop(false, true)(1回だけ行って戻る)は開始値で終わる。SetLoop(true)は終わらない
// - SetLoop(true, ...)の動きは終わらないので、SetOnCompleteのコールバックは呼ばれない(止める時はStop)。
//   SetLoop(false, true)(1回だけ行って戻る)は戻り終わった時に終わるので、呼ばれる
// - SetOnComplete / TweenValueのコールバックの中で、同じTweenComponentに新しい動きを始めても、Stopしても、
//   オブジェクトをDestroyしても大丈夫(新しい動きは次のフレームから動く。Destroyされたら残りの動きは進めない)
// - 位置・大きさ・回転は transform の値(親がいれば親からの相対)をそのまま書き換える。
//   PhysXのDynamicな剛体(RigidbodyComponent)が付いたオブジェクトには MoveTo / RotateTo を使わない(PhysXの位置で上書きされる)
// - GameObjectが非アクティブ(SetActive(false))の間は、動きも止まる
class TweenComponent : public Component {
private:
    // 何を動かすか
    enum class TweenKind { Position, Scale, Rotation, Color, Value };

    // 1つの動き
    struct Tween {
        TweenKind kind = TweenKind::Value;      // 何を動かすか
        DirectX::XMFLOAT4 from{};               // 開始値(位置・大きさ・回転はxyz、色はxyzw、Valueはxだけ使う)
        DirectX::XMFLOAT4 to{};                 // 目標値
        float duration = 0.f;                   // かける時間(秒)
        EaseFunction ease = Ease::OutQuad;      // イージング関数
        float delay = 0.f;                      // 動き出すまでの残り時間(秒)
        float elapsed = 0.f;                    // 動き出してからの時間(秒。ループ中は今の周の時間)
        bool started = false;                   // 動き出したか(開始値を取ったか)
        bool loop = false;                      // 繰り返すか
        bool pingPong = false;                  // 行って戻るか(ループなら「行って戻る」を1周として繰り返す)
        bool unscaled = false;                  // trueならポーズ・スローの影響を受けない時間で動く
        bool removed = false;                   // 終わった・上書きされた・止められた(Updateの最後に消す)
        std::function<void(float)> onUpdate;    // Value用: 毎フレーム今の値を渡す
        std::function<void()> onComplete;       // 終わった時に呼ぶ
    };

    std::vector<std::unique_ptr<Tween>> tweens;    // 動いている動き
    std::vector<std::unique_ptr<Tween>> pending;   // Update中(コールバックの中)に始めた動き。Updateの最後にtweensへ移す
    Tween* lastTween = nullptr;                    // 直前に始めた動き(SetDelayなどの対象。消えたらnullptr)
    bool updating = false;                         // Update中か(trueの間はtweensを書き換えない)

    TweenComponent& StartTween(TweenKind kind, DirectX::XMFLOAT4 to, float duration, EaseFunction ease);
    void UpdateTween(Tween& tween, float dt);
    bool CaptureStartValue(Tween& tween);
    void ApplyValue(Tween& tween, float easedProgress);
    void RemoveFinishedTweens();

public:
    // ─── 動きを始める(すべて自分自身を返すので、SetDelayなどをつなげて書ける) ─────────────
    TweenComponent& MoveTo(DirectX::XMFLOAT3 target, float duration, EaseFunction ease = Ease::OutQuad);    // 位置を動かす
    TweenComponent& ScaleTo(DirectX::XMFLOAT3 target, float duration, EaseFunction ease = Ease::OutQuad);   // 大きさを変える
    TweenComponent& RotateTo(DirectX::XMFLOAT3 target, float duration, EaseFunction ease = Ease::OutQuad);  // 回転させる(度。近い方向ではなく、数値をそのまま補間する)
    TweenComponent& ColorTo(DirectX::XMFLOAT4 target, float duration, EaseFunction ease = Ease::OutQuad);   // 色を変える(SpriteRenderer、無ければMeshRenderer)
    TweenComponent& TweenValue(float from, float to, float duration,
        std::function<void(float)> onUpdate, EaseFunction ease = Ease::OutQuad);                           // 好きな値を動かす(毎フレームonUpdateに今の値を渡す)

    // ─── 直前に始めた動きの設定 ─────────────
    TweenComponent& SetDelay(float seconds);                       // 動き出すまで待つ時間(秒)
    TweenComponent& SetLoop(bool loop, bool pingPong = false);     // 繰り返す(pingPong=行って戻る。loop=falseでpingPong=trueなら1回だけ行って戻る)
    TweenComponent& SetUnscaled(bool unscaled);                    // trueにするとポーズ中も動く(Time::GetUnscaledDeltaTime()で進む)
    TweenComponent& SetOnComplete(std::function<void()> callback); // 終わった時に呼ぶ(ループの動きでは呼ばれない)

    void Stop();   // すべての動きを、今の値のまま止める(OnCompleteは呼ばない)

    void Update(float dt) override;
    void Uninit() override;
};
