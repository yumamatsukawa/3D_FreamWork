#include "SoundDemoController.h"
#include "../Common/DemoStatus.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Animation/TweenComponent.h"
#include "../../../../Engine/Audio/Audio.h"
#include "../../../../Engine/Audio/AudioSourceComponent.h"
#include "../../../../Engine/Render/Image.h"
#include <cstdio>
#include <vector>

namespace {
    const std::wstring shotPath = L"Assets/Sounds/shot.wav";   // PlayOneShot で鳴らすSE

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 blue = { 0.5f, 0.75f, 1.f, 1.f };
    const DirectX::XMFLOAT4 orange = { 1.f, 0.7f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 red = { 1.f, 0.5f, 0.5f, 1.f };

    // BGM自体の音量の棒(画面の中央から。スライダーの下に出す)
    constexpr DirectX::XMFLOAT2 barCenter = { -330.f, -190.f };
    constexpr DirectX::XMFLOAT2 barSize = { 400.f, 16.f };
}

void SoundDemoController::Setup(AudioSourceComponent* bgmSource, AudioSourceComponent* hitSource, TweenComponent* tweenComponent) {
    bgm = bgmSource;
    hit = hitSource;
    tween = tweenComponent;
    message = L"1〜5 かボタンで鳴らしてみよう";
}

void SoundDemoController::StartFade(float from, float to, bool stopAtEnd) {
    // 前のフェードを止めて、BGM自体の音量を from から to へ動かす(グループの音量とは別の値)
    tween->Stop();
    fade = stopAtEnd ? Fade::Out : Fade::In;
    tween->TweenValue(from, to, fadeSeconds, [this](float v) {
        bgmVolume = v;
        bgm->SetVolume(v);
    }, Ease::Linear).SetOnComplete([this, stopAtEnd] {
        fade = Fade::None;
        if (stopAtEnd) {
            // 止めたあと、次に鳴らす時のために音量を戻しておく
            bgm->Stop();
            bgmVolume = 1.f;
            bgm->SetVolume(1.f);
        }
    });
}

void SoundDemoController::ToggleBgm() {
    // フェード中でも、すぐ切り替える
    tween->Stop();
    fade = Fade::None;
    bgmVolume = 1.f;
    bgm->SetVolume(1.f);
    if (bgm->IsPlaying()) {
        bgm->Stop();
        message = L"BGM を止めました(Stop)";
        messageError = false;
        return;
    }
    bgm->Play();
    messageError = !bgm->IsPlaying();
    message = messageError ? L"鳴らせませんでした(音声デバイスが無い・ファイルが読めない)" : L"BGM を鳴らしました(Play。ループ)";
}

void SoundDemoController::PlayShot() {
    Audio::PlayOneShot(shotPath);
    shotCount++;
    message = L"SE を鳴らしました(PlayOneShot。連打すると重なる)";
    messageError = false;
}

void SoundDemoController::FadeOutBgm() {
    if (!bgm->IsPlaying()) {
        message = L"BGM が鳴っていません(先に 1 か 4)";
        messageError = false;
        return;
    }
    StartFade(bgmVolume, 0.f, true);
    message = L"BGM をフェードアウトします(2秒)";
    messageError = false;
}

void SoundDemoController::FadeInBgm() {
    // 音量0から鳴らし始めて、上げていく(鳴っている途中なら、今の音量から上げる)
    if (!bgm->IsPlaying()) {
        bgmVolume = 0.f;
        bgm->SetVolume(0.f);
        bgm->Play();
        if (!bgm->IsPlaying()) {
            bgmVolume = 1.f;
            bgm->SetVolume(1.f);
            message = L"鳴らせませんでした(音声デバイスが無い・ファイルが読めない)";
            messageError = true;
            return;
        }
    }
    StartFade(bgmVolume, 1.f, false);
    message = L"BGM をフェードインします(2秒)";
    messageError = false;
}

void SoundDemoController::PlayHit() {
    hit->Play();
    message = L"SE を鳴らしました(AudioSourceComponent の Play。鳴らし直すと最初から)";
    messageError = false;
}

void SoundDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused() || !bgm) return;

    if (Input::GetKeyDown(KEY_1)) ToggleBgm();
    if (Input::GetKeyDown(KEY_2)) PlayShot();
    if (Input::GetKeyDown(KEY_3)) FadeOutBgm();
    if (Input::GetKeyDown(KEY_4)) FadeInBgm();
    if (Input::GetKeyDown(KEY_5)) PlayHit();
}

void SoundDemoController::Draw() {
    if (!bgm) return;

    const bool playing = bgm->IsPlaying();
    const wchar_t* fadeText = (fade == Fade::Out) ? L"フェードアウト中" : (fade == Fade::In) ? L"フェードイン中" : L"";

    wchar_t buf[200];
    std::vector<DemoStatusLine> lines;
    swprintf_s(buf, L"BGM  %ls   BGM自体の音量 SetVolume: %.2f  %ls", playing ? L"再生中" : L"停止", bgmVolume, fadeText);
    lines.push_back({ buf, blue });
    swprintf_s(buf, L"音量グループ SetGroupVolume:  BGM %.2f   SE %.2f   (実際の音量 = 自体の音量 x グループ)",
        Audio::GetGroupVolume(AudioGroup::BGM), Audio::GetGroupVolume(AudioGroup::SE));
    lines.push_back({ buf, orange });
    swprintf_s(buf, L"SE  PlayOneShot(shot.wav) %d 回   hit.wav は AudioSourceComponent(グループSE)", shotCount);
    lines.push_back({ buf, white });
    lines.push_back({ message, messageError ? red : green });
    lines.push_back({ L"1 BGM 入/切   2 SE(shot)   3 フェードアウト   4 フェードイン   5 SE(hit)", green });
    DrawDemoStatus(lines, 820.f);

    // BGM自体の音量の棒(フェードで動くのが見える)
    Text::Draw(L"BGM の音量(SetVolume。フェードで動く)", barCenter.x - barSize.x * 0.5f, barCenter.y + 26.f, MakeDemoTextStyle(18.f));
    Image::DrawRect(barCenter, barSize, { 0.f, 0.f, 0.f, 0.7f });
    const float fillWidth = barSize.x * (playing ? bgmVolume : 0.f);
    if (fillWidth > 0.5f) {
        Image::DrawRect({ barCenter.x - (barSize.x - fillWidth) * 0.5f, barCenter.y }, { fillWidth, barSize.y }, { 0.4f, 0.8f, 1.f, 1.f });
    }
}

void SoundDemoController::Uninit() {
    // この画面を出る時にBGMを止める(音のデータの返却は AudioSourceComponent が行う)
    if (bgm) bgm->Stop();
    if (hit) hit->Stop();
}
