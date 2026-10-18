#include "AudioListenerComponent.h"
#include "../Core/GameObject.h"
#include "Audio.h"
#include "../Render/Image.h"
#include <vector>
#include <algorithm>

namespace {
    // 有効なリスナー(有効になった順。いちばん後ろが「最後に有効になった」=使われるもの)
    std::vector<AudioListenerComponent*> activeListeners;
    bool warnedMultiple = false;   // 複数あることを知らせたか

    void Register(AudioListenerComponent* listener) {
        // すでに入っていれば一度外す(入れ直して「最後に有効になった」にする)
        activeListeners.erase(std::remove(activeListeners.begin(), activeListeners.end(), listener), activeListeners.end());
        if (!activeListeners.empty() && !warnedMultiple) {
            warnedMultiple = true;
            OutputDebugStringA("★ AudioListenerComponent: 2つ以上あります。最後に有効になったものを使います\n");
        }
        activeListeners.push_back(listener);
    }

    void Unregister(AudioListenerComponent* listener) {
        activeListeners.erase(std::remove(activeListeners.begin(), activeListeners.end(), listener), activeListeners.end());
    }
}

void AudioListenerComponent::Init() {
    Register(this);
}

void AudioListenerComponent::Update(float dt) {
    // 使われるのは最後に有効になった1つだけ(ほかのリスナーは何も伝えない)
    if (activeListeners.empty() || activeListeners.back() != this) return;

    XMFLOAT3 position = GetOwner()->transform.GetWorldTransform().position;

    if (use3DCamera) {
        // 3D: 3D用カメラの向き(カメラから注視点へ向かう方向が正面)
        const Camera& camera = Image::GetCamera3D();
        XMFLOAT3 front = { camera.target.x - camera.position.x, camera.target.y - camera.position.y, camera.target.z - camera.position.z };
        Audio::SetListener(position, front, camera.up);
    }
    else {
        // 2D: 画面の上(+Y)が正面。頭の上は画面の手前(-Z)にすると、右が+Xになる
        Audio::SetListener(position, { 0.f, 1.f, 0.f }, { 0.f, 0.f, -1.f });
    }
}

void AudioListenerComponent::Uninit() {
    Unregister(this);
}

void AudioListenerComponent::OnActiveChanged(bool active) {
    if (active) Register(this);
    else Unregister(this);
}
