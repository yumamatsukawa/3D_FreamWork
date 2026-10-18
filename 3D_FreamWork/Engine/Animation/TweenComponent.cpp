#include "TweenComponent.h"
#include "../Core/GameObject.h"
#include "../Render/SpriteRenderer.h"
#include "../Render3D/MeshRenderer.h"
#include "../Core/MathUtil.h"
#include "../Core/Time.h"
#include <algorithm>
#include <cmath>
#include <windows.h>

using namespace DirectX;

// ─── 動きを始める ─────────────────────────────
TweenComponent& TweenComponent::MoveTo(XMFLOAT3 target, float duration, EaseFunction ease) {
    return StartTween(TweenKind::Position, { target.x, target.y, target.z, 0.f }, duration, ease);
}

TweenComponent& TweenComponent::ScaleTo(XMFLOAT3 target, float duration, EaseFunction ease) {
    return StartTween(TweenKind::Scale, { target.x, target.y, target.z, 0.f }, duration, ease);
}

TweenComponent& TweenComponent::RotateTo(XMFLOAT3 target, float duration, EaseFunction ease) {
    return StartTween(TweenKind::Rotation, { target.x, target.y, target.z, 0.f }, duration, ease);
}

TweenComponent& TweenComponent::ColorTo(XMFLOAT4 target, float duration, EaseFunction ease) {
    return StartTween(TweenKind::Color, target, duration, ease);
}

TweenComponent& TweenComponent::TweenValue(float from, float to, float duration,
    std::function<void(float)> onUpdate, EaseFunction ease) {
    StartTween(TweenKind::Value, { to, 0.f, 0.f, 0.f }, duration, ease);
    lastTween->from = { from, 0.f, 0.f, 0.f };
    lastTween->onUpdate = std::move(onUpdate);
    return *this;
}

TweenComponent& TweenComponent::StartTween(TweenKind kind, XMFLOAT4 to, float duration, EaseFunction ease) {
    // 同じ種類の動きは上書きする(TweenValueは別々の値なので上書きしない)
    if (kind != TweenKind::Value) {
        for (auto& tween : tweens)  if (tween->kind == kind) tween->removed = true;
        for (auto& tween : pending) if (tween->kind == kind) tween->removed = true;
    }

    auto tween = std::make_unique<Tween>();
    tween->kind = kind;
    tween->to = to;
    tween->duration = duration;
    tween->ease = ease;
    lastTween = tween.get();

    // Update中(コールバックの中)は、回している途中のtweensを書き換えないように一旦pendingへ
    if (updating) pending.push_back(std::move(tween));
    else          tweens.push_back(std::move(tween));
    return *this;
}

// ─── 直前に始めた動きの設定 ─────────────────────────────
TweenComponent& TweenComponent::SetDelay(float seconds) {
    if (lastTween) lastTween->delay = (seconds > 0.f) ? seconds : 0.f;
    return *this;
}

TweenComponent& TweenComponent::SetLoop(bool loop, bool pingPong) {
    if (lastTween) {
        lastTween->loop = loop;
        lastTween->pingPong = pingPong;
    }
    return *this;
}

TweenComponent& TweenComponent::SetUnscaled(bool unscaled) {
    if (lastTween) lastTween->unscaled = unscaled;
    return *this;
}

TweenComponent& TweenComponent::SetOnComplete(std::function<void()> callback) {
    if (lastTween) lastTween->onComplete = std::move(callback);
    return *this;
}

void TweenComponent::Stop() {
    // Update中は印を付けるだけにして、Updateの最後に消す
    for (auto& tween : tweens) tween->removed = true;
    pending.clear();
    lastTween = nullptr;
    if (!updating) tweens.clear();
}

// ─── 毎フレームの更新 ─────────────────────────────
void TweenComponent::Update(float dt) {
    // 今ある動きだけを進める(コールバックの中で始めた動きはpendingに入り、次のフレームから動く)
    updating = true;
    for (size_t i = 0; i < tweens.size(); i++) {
        if (GetOwner()->GetIsDestroyed()) break;   // コールバックでDestroyされたら、残りの動きは進めない
        Tween& tween = *tweens[i];
        if (tween.removed) continue;
        UpdateTween(tween, tween.unscaled ? Time::GetUnscaledDeltaTime() : dt);
    }
    updating = false;

    // 終わった動きを消して、Update中に始めた動きを加える
    RemoveFinishedTweens();
    for (auto& tween : pending) {
        if (tween->removed) continue;   // Update中に始めて、すぐ上書きされた動き
        tweens.push_back(std::move(tween));
    }
    pending.clear();
}

void TweenComponent::UpdateTween(Tween& tween, float dt) {
    // 動き出すまで待つ(待ち時間を超えた分は、そのまま動きに使う)
    if (tween.delay > 0.f) {
        tween.delay -= dt;
        if (tween.delay > 0.f) return;
        dt = -tween.delay;
        tween.delay = 0.f;
    }

    // 動き出した時の今の値を開始値にする
    if (!tween.started) {
        if (!CaptureStartValue(tween)) {
            tween.removed = true;   // 色を変える相手がいないなど、動かせない
            return;
        }
        tween.started = true;
    }

    // 進み具合(0〜1)を求める。pingPongは「行って戻る」で1周(durationの2倍)
    tween.elapsed += dt;
    float progress = 1.f;
    bool finished = false;
    if (tween.duration <= 0.f) {
        // 0秒の動きは、すぐ1周終わった状態にする(0で割らない)
        progress = tween.pingPong ? 0.f : 1.f;
        tween.elapsed = 0.f;
        finished = !tween.loop;
    } else {
        float cycle = tween.pingPong ? tween.duration * 2.f : tween.duration;
        if (tween.elapsed >= cycle) {
            if (tween.loop) {
                tween.elapsed = fmodf(tween.elapsed, cycle);   // 余った時間を次の周へ持ち越す
            } else {
                tween.elapsed = cycle;
                finished = true;
            }
        }
        progress = tween.elapsed / tween.duration;
        if (progress > 1.f) progress = 2.f - progress;   // pingPongの戻り
    }

    float eased = tween.ease ? tween.ease(progress) : progress;
    ApplyValue(tween, eased);
    if (!finished || tween.removed) return;   // TweenValueのonUpdateの中でStopされた時は、終わった扱いにしない

    // 終わった: 先に消す印を付けてから呼ぶ(コールバックの中で同じ種類の動きを始めても上書きされない)
    tween.removed = true;
    if (tween.onComplete) {
        std::function<void()> callback = std::move(tween.onComplete);
        callback();
    }
}

bool TweenComponent::CaptureStartValue(Tween& tween) {
    Transform& t = GetOwner()->transform;
    switch (tween.kind) {
    case TweenKind::Position: tween.from = { t.position.x, t.position.y, t.position.z, 0.f }; return true;
    case TweenKind::Scale:    tween.from = { t.scale.x, t.scale.y, t.scale.z, 0.f };          return true;
    case TweenKind::Rotation: tween.from = { t.rotate.x, t.rotate.y, t.rotate.z, 0.f };       return true;
    case TweenKind::Value:    return true;   // fromは引数で決まっている
    case TweenKind::Color:
        // SpriteRendererを優先し、無ければMeshRendererの色を使う
        if (SpriteRenderer* sprite = GetOwner()->GetComponent<SpriteRenderer>()) {
            tween.from = sprite->GetColor();
            return true;
        }
        if (MeshRenderer* mesh = GetOwner()->GetComponent<MeshRenderer>()) {
            tween.from = mesh->GetColor();
            return true;
        }
        OutputDebugStringA("★ TweenComponent: ColorTo には SpriteRenderer か MeshRenderer が必要です(何もしません)\n");
        return false;
    }
    return false;
}

void TweenComponent::ApplyValue(Tween& tween, float easedProgress) {
    XMFLOAT4 value = {
        Math::Lerp(tween.from.x, tween.to.x, easedProgress),
        Math::Lerp(tween.from.y, tween.to.y, easedProgress),
        Math::Lerp(tween.from.z, tween.to.z, easedProgress),
        Math::Lerp(tween.from.w, tween.to.w, easedProgress),
    };

    Transform& t = GetOwner()->transform;
    switch (tween.kind) {
    case TweenKind::Position: t.position = { value.x, value.y, value.z }; break;
    case TweenKind::Scale:    t.scale = { value.x, value.y, value.z };    break;
    case TweenKind::Rotation: t.rotate = { value.x, value.y, value.z };   break;
    case TweenKind::Color:
        if (SpriteRenderer* sprite = GetOwner()->GetComponent<SpriteRenderer>()) {
            sprite->SetColor(value.x, value.y, value.z, value.w);
        } else if (MeshRenderer* mesh = GetOwner()->GetComponent<MeshRenderer>()) {
            mesh->SetColor(value.x, value.y, value.z, value.w);
        }
        break;
    case TweenKind::Value:
        if (tween.onUpdate) tween.onUpdate(value.x);
        break;
    }
}

void TweenComponent::RemoveFinishedTweens() {
    // 消える動きが「直前に始めた動き」なら、SetDelayなどで触らないように忘れる
    for (auto& tween : tweens)
        if (tween->removed && tween.get() == lastTween) lastTween = nullptr;
    for (auto& tween : pending)
        if (tween->removed && tween.get() == lastTween) lastTween = nullptr;

    tweens.erase(
        std::remove_if(tweens.begin(), tweens.end(),
            [](const std::unique_ptr<Tween>& tween) { return tween->removed; }),
        tweens.end());
}

void TweenComponent::Uninit() {
    // 持っているコールバック(ラムダ)を、オブジェクトと一緒に手放す
    Stop();
}
