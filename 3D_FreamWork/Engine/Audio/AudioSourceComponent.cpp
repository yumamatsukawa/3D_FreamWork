#include "AudioSourceComponent.h"
#include "../Core/GameObject.h"
#include "../Core/Resources.h"
#include <algorithm>

void AudioSourceComponent::Init() {
    // 音のデータを借りる(同じファイルは1回だけ読まれる)。読めなくても、Playが何もしないだけ
    soundID = Resources::LoadSound(path);
}

void AudioSourceComponent::Update(float dt) {
    // 位置で聞こえ方が変わる音は、オブジェクトの動きに合わせて毎フレーム位置を伝える
    // (鳴り終わっていれば再生口は自動で返されていて、伝えても何も起きない)
    if (spatial && Audio::IsVoicePlaying(voice)) Audio::UpdateVoice(voice, MakeParams());
}

void AudioSourceComponent::Uninit() {
    // 再生口と音のデータを返す(破棄・シーン切り替えで鳴りっぱなしが残らない)
    Stop();
    if (soundID != UINT_MAX) {
        Resources::ReleaseSound(soundID);
        soundID = UINT_MAX;
    }
}

void AudioSourceComponent::OnActiveChanged(bool active) {
    if (!active) Stop();
}

void AudioSourceComponent::Play() {
    // 最初から鳴らし直す
    Stop();

    // Initで読めなかった時は、もう一度読みにいく
    if (soundID == UINT_MAX) soundID = Resources::LoadSound(path);
    if (soundID == UINT_MAX) return;

    voice = Audio::StartVoice(soundID, MakeParams());
}

void AudioSourceComponent::Stop() {
    Audio::StopVoice(voice);
    voice = Audio::VoiceHandle{};
}

bool AudioSourceComponent::IsPlaying() const {
    return Audio::IsVoicePlaying(voice);
}

void AudioSourceComponent::SetVolume(float newVolume) {
    volume = (std::min)((std::max)(newVolume, 0.f), 1.f);
    if (Audio::IsVoicePlaying(voice)) Audio::UpdateVoice(voice, MakeParams());
}

void AudioSourceComponent::SetSpatial(bool enabled) {
    spatial = enabled;
    if (Audio::IsVoicePlaying(voice)) Audio::UpdateVoice(voice, MakeParams());
}

void AudioSourceComponent::SetDistance(float newMin, float newMax) {
    // 負の値は0にし、minがmaxより大きければ並べ直す
    newMin = (std::max)(newMin, 0.f);
    newMax = (std::max)(newMax, 0.f);
    minDistance = (std::min)(newMin, newMax);
    maxDistance = (std::max)(newMin, newMax);
    if (Audio::IsVoicePlaying(voice)) Audio::UpdateVoice(voice, MakeParams());
}

Audio::VoiceParams AudioSourceComponent::MakeParams() const {
    Audio::VoiceParams params;
    params.volume = volume;
    params.loop = loop;
    params.spatial = spatial;
    if (GetOwner()) params.position = GetOwner()->transform.GetWorldTransform().position;
    params.minDistance = minDistance;
    params.maxDistance = maxDistance;
    params.group = group;
    return params;
}
