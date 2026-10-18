#include "VolumeSlider.h"
#include "../Components/TitleController.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/UI/SliderComponent.h"
#include "../../../Engine/Data/SaveData.h"

GameObject* CreateVolumeSlider(Scene& scene, TitleController* title) {
    GameObject* object = scene.CreateObject("VolumeSlider");

    // スライダー(画面の下の真ん中。値は0〜1)
    SliderComponent* slider = object->AddComponent<SliderComponent>();
    slider->SetRange(0.f, 1.f);
    slider->SetAnchor(Anchor::Bottom, { 0.f, 90.f });
    slider->SetSize(300.f, 12.f);

    // 文字(スライダーの上)
    TextStyle labelStyle;
    labelStyle.size = 28.f;
    labelStyle.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    slider->SetLabel(L"Volume", labelStyle);

    // 値の持ち主はTitleController。最初はその音量を表示して、動かしたらその音量を変えてもらう
    if (title) {
        slider->SetValue(title->GetVolume());
        slider->SetOnChanged([title](float volume) { title->SetVolume(volume); });
    }

    // つまみを離した時に、ファイルへ保存する(動かしている間は保存しない)
    slider->SetOnDragEnd([](float) { SaveData::Save(); });
    return object;
}
