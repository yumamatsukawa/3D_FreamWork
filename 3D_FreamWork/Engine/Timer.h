#pragma once
#include <cmath>

// 「○秒たったら」を数えるタイマー。Componentのメンバに持ち、毎フレームUpdate(dt)を呼ぶ。
//
// 使い方:
//   Timer spawnTimer{ 2.5f, true };            // 2.5秒ごと(繰り返す)
//   if (spawnTimer.Update(dt)) Spawn(...);     // 時間が来たフレームだけtrue
//
//   Timer invincibleTimer{ 1.0f };             // 1回だけ(繰り返さない)
//   invincibleTimer.Complete();                // 最初は「終わった状態」にしておき、
//   invincibleTimer.Reset();                   // きっかけがあったら数え始める
//   if (!invincibleTimer.IsFinished()) { ... } // 数えている間だけ
struct Timer {
    float duration = 1.0f;   // 何秒で時間が来るか
    bool  loop = false;      // 繰り返すか
    float elapsed = 0.0f;    // 経過時間(秒)
    bool  finished = false;  // 終わったか(loopでない時だけ使う。0秒のタイマーも1回は時間が来るように、経過時間とは別に持つ)

    Timer(float duration = 1.0f, bool loop = false) : duration(duration), loop(loop) {}

    // 時間を進める。時間が来たフレームだけtrueを返す
    // loopなら余った時間を次の周へ持ち越す(dtが大きくて何周分も進んでも、trueは1回だけ)
    // loopでなければ、一度trueを返した後はfalseのまま(Resetで数え直す)。0秒なら次のUpdateで1回だけtrue
    // ※loopでdurationが0以下の時は、Updateの度に毎回trueになる(dtが0のポーズ中も)
    bool Update(float dt) {
        if (IsFinished()) return false;
        elapsed += dt;
        if (elapsed < duration) return false;

        if (loop) {
            // 何周分進んでも、余りだけを残す(0秒以下のタイマーは0で割らないように0へ戻す)
            elapsed = (duration > 0.0f) ? fmodf(elapsed, duration) : 0.0f;
        } else {
            elapsed = duration;   // 終わった状態で止める
            finished = true;
        }
        return true;
    }

    // 最初から数え直す
    void Reset() {
        elapsed = 0.0f;
        finished = false;
    }

    // すぐに「終わった状態」にする(loopの時は、次のUpdateで時間が来る)
    void Complete() {
        elapsed = duration;
        finished = !loop;
    }

    // 進み具合(0〜1)
    float GetProgress() const {
        if (duration <= 0.0f) return IsFinished() ? 1.0f : 0.0f;   // 0で割らない
        float progress = elapsed / duration;
        if (progress < 0.0f) return 0.0f;
        if (progress > 1.0f) return 1.0f;
        return progress;
    }

    // 終わったか(loopなら終わらないので常にfalse)
    bool IsFinished() const { return !loop && finished; }
};
