#include "SpriteAnimationComponent.h"
#include "../Core/GameObject.h"
#include "../Render/SpriteRenderer.h"
#include "../Core/MathUtil.h"
#include <cmath>
#include <windows.h>

// ─── 登録・再生 ─────────────────────────────
void SpriteAnimationComponent::AddClip(const std::string& name, std::vector<int> frames,
    float frameDuration, bool loop) {
    // コマが無い・時間が0以下だとコマを送れない(0で割ってしまう)ので登録しない
    if (frames.empty() || frameDuration <= 0.0f) {
        OutputDebugStringA(("★ SpriteAnimationComponent::AddClip: \"" + name +
            "\" はコマが空か、1コマの秒数が0以下なので登録しません\n").c_str());
        return;
    }

    Clip& clip = clips[name];
    clip.frames = std::move(frames);
    clip.frameDuration = frameDuration;
    clip.loop = loop;

    // 再生中のクリップを上書きした時は、コマ数が変わるかもしれないので最初から
    if (current == &clip) Restart();
}

void SpriteAnimationComponent::Play(const std::string& name) {
    // 今と同じ名前なら何もしない(毎フレーム呼ばれても最初に戻らないように)
    if (current && name == currentName) return;

    auto it = clips.find(name);
    if (it == clips.end()) {
        // 毎フレーム呼ばれても出力ウィンドウが埋まらないように、名前ごとに1回だけ知らせる
        if (warnedNames.insert(name).second) {
            OutputDebugStringA(("★ SpriteAnimationComponent::Play: \"" + name +
                "\" は登録されていません(AddClipを確認してください)\n").c_str());
        }
        return;
    }

    currentName = name;
    current = &it->second;
    Restart();
}

void SpriteAnimationComponent::Stop() {
    // 今のクリップを忘れる(SpriteRendererのコマは今のまま残る)
    current = nullptr;
    currentName.clear();
    finished = false;
}

bool SpriteAnimationComponent::IsFinished() const {
    return current && !current->loop && finished;
}

void SpriteAnimationComponent::SetSpeed(float newSpeed) {
    speed = (newSpeed > 0.0f) ? newSpeed : 0.0f;   // 逆再生はしない
}

void SpriteAnimationComponent::SetRandomStart(bool enabled) {
    randomStart = enabled;
}

// ─── 毎フレームの更新 ─────────────────────────────
void SpriteAnimationComponent::Update(float dt) {
    if (!current) return;

    // コマ送り: 1コマ分の時間がたった数だけ進める(余った時間は次のコマへ持ち越す)
    if (!finished && speed > 0.0f && dt > 0.0f) {
        elapsed += dt * speed;
        float duration = current->frameDuration;
        if (elapsed >= duration) {
            float steps = floorf(elapsed / duration);   // 何コマ進んだか
            elapsed -= steps * duration;
            if (elapsed < 0.0f) elapsed = 0.0f;        // 小数の誤差でわずかにマイナスになった時

            int count = (int)current->frames.size();
            int remaining = count - frameIndex;         // 最後のコマを見せ終わるまでに進むコマ数
            if (current->loop) {
                frameIndex = (frameIndex + (int)fmodf(steps, (float)count)) % count;
            } else if (steps >= (float)remaining) {
                // 最後のコマを見せ終わった: 最後のコマのまま止める
                frameIndex = count - 1;
                elapsed = 0.0f;
                finished = true;
            } else {
                frameIndex += (int)steps;
            }
        }
    }

    ApplyFrame();
}

void SpriteAnimationComponent::OnActiveChanged(bool active) {
    // ObjectPoolで再利用された時は、前回の続きからではなく最初のコマから
    if (active && current) Restart();
}

// ─── 内部 ─────────────────────────────
void SpriteAnimationComponent::Restart() {
    frameIndex = 0;
    elapsed = 0.0f;
    finished = false;

    // ランダムな所から(ループするクリップだけ。何体も並んだ時に足踏みがそろわないように)
    if (randomStart && current->loop) {
        frameIndex = Math::RandomRange(0, (int)current->frames.size() - 1);
        elapsed = Math::RandomRange(0.0f, current->frameDuration);
        if (elapsed >= current->frameDuration) elapsed = 0.0f;   // 小数の誤差で1コマ分ちょうどになった時
    }

    ApplyFrame();   // Updateを待たずに、すぐ今のコマにする
}

void SpriteAnimationComponent::ApplyFrame() {
    SpriteRenderer* sprite = GetOwner()->GetComponent<SpriteRenderer>();
    if (!sprite) {
        if (!warnedNoSprite) {
            OutputDebugStringA(("★ SpriteAnimationComponent: SpriteRendererがありません(" +
                GetOwner()->GetName() + ")\n").c_str());
            warnedNoSprite = true;
        }
        return;
    }
    sprite->SetSpriteIndex(current->frames[frameIndex]);
}
