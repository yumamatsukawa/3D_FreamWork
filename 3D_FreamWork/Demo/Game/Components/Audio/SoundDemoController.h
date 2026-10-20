#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

class AudioSourceComponent;
class TweenComponent;

// Soundデモの操作役。BGM(AudioSourceComponent・グループBGM・ループ)とSE(PlayOneShot・AudioSourceComponent)を鳴らす。
// 音量グループ(BGM/SE)はスライダーで変える。フェードは Engine に専用の関数が無いので、TweenValue で BGM自体の音量を動かす。
// 借りた音は AudioSourceComponent が返す。BGMはこの画面を出る時に止める(次の画面で鳴り続けない)
class SoundDemoController : public Component {
private:
    enum class Fade { None, Out, In };

    AudioSourceComponent* bgm = nullptr;    // BGM(TitleBGM.wav。ループ)
    AudioSourceComponent* hit = nullptr;    // SEの1つ(hit.wav。鳴らし直すと最初から)
    TweenComponent* tween = nullptr;        // フェードで音量を動かす
    float bgmVolume = 1.f;                  // BGM自体の音量(フェードで動かす。グループの音量とは別)
    Fade fade = Fade::None;                 // フェード中か
    int shotCount = 0;                      // PlayOneShot で鳴らした回数
    std::wstring message;                   // 最後の操作の結果
    bool messageError = false;              // messageが「鳴らせなかった」知らせか

    static constexpr float fadeSeconds = 2.f;   // フェードにかける時間

    void StartFade(float from, float to, bool stopAtEnd);   // BGMの音量を from から to へ動かす

public:
    // 鳴らす物とフェードの役を、CreateSoundDemo()から渡してもらう(Init の後)
    void Setup(AudioSourceComponent* bgmSource, AudioSourceComponent* hitSource, TweenComponent* tweenComponent);

    void ToggleBgm();      // 1: BGMを鳴らす/止める(すぐ)
    void PlayShot();       // 2: SE(shot.wav)を PlayOneShot で鳴らす(重なる)
    void FadeOutBgm();     // 3: BGMをフェードアウトして止める
    void FadeInBgm();      // 4: BGMをフェードインして鳴らす
    void PlayHit();        // 5: SE(hit.wav)を AudioSourceComponent で鳴らす

    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
