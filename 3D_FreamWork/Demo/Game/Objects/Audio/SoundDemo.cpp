#include "SoundDemo.h"
#include "../Common/DemoButton.h"
#include "../../Components/Audio/SoundDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Animation/TweenComponent.h"
#include "../../../../Engine/Audio/Audio.h"
#include "../../../../Engine/Audio/AudioSourceComponent.h"
#include "../../../../Engine/UI/SliderComponent.h"

namespace {
    // 音量のスライダー1本を作る(つまみを動かすと、そのグループの音量が変わる)
    SliderComponent* AddVolumeSlider(GameObject* demo, const wchar_t* label, AudioGroup group, float y) {
        SliderComponent* slider = demo->AddComponent<SliderComponent>();
        slider->SetRange(0.f, 1.f);
        slider->SetValue(Audio::GetGroupVolume(group));
        slider->SetAnchor(Anchor::Center, { -330.f, y });
        slider->SetSize(400.f, 12.f);
        TextStyle labelStyle;
        labelStyle.size = 22.f;
        labelStyle.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        slider->SetLabel(label, labelStyle);
        slider->SetOnChanged([group](float v) { Audio::SetGroupVolume(group, v); });
        return slider;
    }
}

GameObject* CreateSoundDemo(Scene& scene) {
    GameObject* demo = scene.CreateObject("SoundDemo");

    // BGM: ループする音。位置は使わず(SetSpatial(false))、グループは BGM
    auto* bgm = demo->AddComponent<AudioSourceComponent>(L"Assets/TitleBGM.wav");
    bgm->SetGroup(AudioGroup::BGM);
    bgm->SetSpatial(false);
    bgm->SetLoop(true);

    // SE: 1回鳴る音。グループは SE(既定)
    auto* hit = demo->AddComponent<AudioSourceComponent>(L"Assets/Sounds/hit.wav");
    hit->SetSpatial(false);

    // フェードで音量を動かす役と、操作役
    auto* tween = demo->AddComponent<TweenComponent>();
    SoundDemoController* controller = demo->AddComponent<SoundDemoController>();
    controller->Setup(bgm, hit, tween);

    // 音量グループのスライダー(SE の方は、離した時に SE を鳴らして聞き比べられるようにする)
    AddVolumeSlider(demo, L"BGM Volume (AudioGroup::BGM)", AudioGroup::BGM, -10.f);
    SliderComponent* seSlider = AddVolumeSlider(demo, L"SE Volume (AudioGroup::SE)", AudioGroup::SE, -100.f);
    seSlider->SetOnDragEnd([controller](float) { controller->PlayShot(); });

    // ボタン(キーと同じ。左の列=BGM、右の列=SE)
    const DirectX::XMFLOAT2 size = { 210.f, 48.f };
    CreateDemoButton(scene, L"1 BGM On/Off", { 130.f, 20.f }, size, 24.f, [controller] { controller->ToggleBgm(); });
    CreateDemoButton(scene, L"3 Fade Out", { 130.f, -40.f }, size, 24.f, [controller] { controller->FadeOutBgm(); });
    CreateDemoButton(scene, L"4 Fade In", { 130.f, -100.f }, size, 24.f, [controller] { controller->FadeInBgm(); });
    CreateDemoButton(scene, L"2 SE (shot)", { 370.f, 20.f }, size, 24.f, [controller] { controller->PlayShot(); });
    CreateDemoButton(scene, L"5 SE (hit)", { 370.f, -40.f }, size, 24.f, [controller] { controller->PlayHit(); });
    return demo;
}
