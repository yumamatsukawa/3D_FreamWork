#include "SceneTransition.h"
#include "../UI/Text.h"

void SceneTransition::Begin() {
    phase = Phase::Covering;
    elapsed = 0.f;
    skipNextUpdate = false;
    OnBegin();
}

void SceneTransition::Update(float unscaledDt) {
    // シーンを切り替えた直後のフレームは、新しいシーンのInit(画像の読み込みなど)の分だけ1フレームが長くなるので、
    // その時間では進めない(開き始めのコマが飛ばないように)
    if (skipNextUpdate) {
        skipNextUpdate = false;
        return;
    }

    // 覆う: 時間になったら、シーンの切り替え待ちにする(切り替えはSceneManagerがこのすぐ後に行う)
    if (phase == Phase::Covering) {
        elapsed += unscaledDt;
        if (elapsed >= GetCoverDuration()) phase = Phase::Covered;
    }
    // 開く: 時間になったら終わり
    else if (phase == Phase::Opening) {
        elapsed += unscaledDt;
        if (elapsed >= GetOpenDuration()) phase = Phase::Finished;
    }
}

void SceneTransition::BeginOpen() {
    phase = Phase::Opening;
    elapsed = 0.f;
    skipNextUpdate = true;
}

void SceneTransition::Draw() {
    ID2D1RenderTarget* target = Text::GetRenderTarget();
    if (!target) return;

    if (phase == Phase::Covering) DrawCover(target, GetProgress(GetCoverDuration()));
    else if (phase == Phase::Covered) DrawCover(target, 1.f);
    else if (phase == Phase::Opening) DrawOpen(target, GetProgress(GetOpenDuration()));
}

bool SceneTransition::IsCovered() const {
    return phase == Phase::Covered;
}

bool SceneTransition::IsFinished() const {
    return phase == Phase::Finished;
}

float SceneTransition::GetProgress(float duration) const {
    if (duration <= 0.f || elapsed >= duration) return 1.f;   // 時間が0以下なら、すぐに終わった扱い
    return elapsed / duration;
}
