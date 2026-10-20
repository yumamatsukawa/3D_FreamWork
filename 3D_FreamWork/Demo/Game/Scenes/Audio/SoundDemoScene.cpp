#include "SoundDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Audio/SoundDemo.h"

void SoundDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"音のデモ(BGM・SE・音量グループ・フェード)",
        L"BGM(ループ)とSE(1回)を鳴らします。音量はBGM/SEのグループごとに変え、フェードは TweenValue で作ります。",
        L"操作: 1 BGM 入/切 / 2 SE / 3 フェードアウト / 4 フェードイン / 5 SE(hit) / スライダー 音量 / Esc: 戻る",
        L"使っているAPI: AudioSourceComponent(Play, SetVolume, SetGroup), PlayOneShot, SetGroupVolume, TweenValue" });
    CreateSoundDemo(*this);
}

void SoundDemoScene::Uninit()
{
    Scene::Uninit();
}
