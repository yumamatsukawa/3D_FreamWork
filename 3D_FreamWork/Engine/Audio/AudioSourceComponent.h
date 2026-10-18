#pragma once
#include "../Core/Component.h"
#include "Audio.h"
#include <string>
#include <climits>

// 音を鳴らす側を表すComponent。音を出すオブジェクト(敵・機械・焚き火など)に付ける。
// 位置で聞こえ方が変わる: 聞く側(AudioListenerComponent。無ければ2Dカメラ)から遠いほど小さく、左にあれば左から聞こえる。
// 同じ音のデータ(WAV)はResourcesで共有する。再生口はPlayする度に借り、止めた時・鳴り終わった時・オブジェクトが
// 破棄された時に返す(オブジェクトを破棄しても、鳴りっぱなしの音は残らない)
//
// 使い方:
//   auto* fire = campfire->AddComponent<AudioSourceComponent>(L"Assets/Sounds/fire.wav");
//   fire->SetLoop(true);
//   fire->SetDistance(200.f, 1500.f);   // 200までは最大音量、1500で聞こえなくなる
//   fire->Play();
//
// 決まり:
// - 音のグループは既定でSE(SetGroupでBGMにも変えられる)。グループの音量は Audio::SetGroupVolume
// - SetSpatial(false)にすると位置を使わず、どこにいても同じ音量で聞こえる(BGM・UIの音)
// - SetVolume / SetDistance / SetSpatial は鳴っている間でも変えられる。SetLoop / SetGroup は、次のPlayから効く
// - SetActive(false)になると止まる(ObjectPoolに返した時など)。自動では再開しないので、必要ならPlayし直す
// - ポーズ中(Time::IsPaused)も止めずに鳴り続ける
class AudioSourceComponent : public Component {
private:
    std::wstring path;                       // 鳴らす音のファイル
    unsigned int soundID = UINT_MAX;         // Resourcesから借りている音のデータ
    Audio::VoiceHandle voice;                // 借りている再生口(鳴っていなければ無効)
    float volume = 1.f;                      // 音量(0〜1)
    bool loop = false;                       // 繰り返すか
    bool spatial = true;                     // 位置で聞こえ方を変えるか
    float minDistance = Audio::defaultMinDistance;   // この距離までは最大音量
    float maxDistance = Audio::defaultMaxDistance;   // この距離で聞こえなくなる
    AudioGroup group = AudioGroup::SE;       // 音量を変えるグループ

    Audio::VoiceParams MakeParams() const;   // 今の設定と、オブジェクトの位置から、再生口に渡す設定を作る

public:
    explicit AudioSourceComponent(const std::wstring& path) : path(path) {}

    void Init() override;
    void Update(float dt) override;
    void Uninit() override;
    void OnActiveChanged(bool active) override;

    void Play();                          // 最初から鳴らす(鳴っていれば鳴らし直す)
    void Stop();                          // 止める
    bool IsPlaying() const;               // 鳴っているか(ループしない音が鳴り終わったらfalse)
    void SetVolume(float newVolume);      // 音量(0〜1。範囲外は0〜1に収める)
    void SetLoop(bool enabled) { loop = enabled; }          // 繰り返すか(次のPlayから)
    void SetSpatial(bool enabled);        // 位置で聞こえ方を変えるか(false=BGM・UIの音)
    void SetDistance(float newMin, float newMax);           // newMinまでは最大音量、newMaxで聞こえなくなる
    void SetGroup(AudioGroup newGroup) { group = newGroup; }   // 音量を変えるグループ(次のPlayから)
};
