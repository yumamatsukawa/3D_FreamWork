#include "Sound3DDemo.h"
#include "../Behavior/DemoAgent.h"
#include "../../Components/Audio/Sound3DDemoController.h"
#include "../../Components/Common/DemoCameraController.h"
#include "../../Components/Common/DemoPlane.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Audio/AudioSourceComponent.h"
#include "../../../../Engine/Audio/AudioListenerComponent.h"

GameObject* CreateSound3DDemo(Scene& scene, DemoMode mode, GameObject* cameraObject) {
    // 動ける範囲(外の壁。見た目だけで、聞く側は壁に当たらず、操作役が範囲に収める)
    CreateDemoField(scene, mode, GetDemoField(mode));

    // 音を出す物(橙): 自分の位置から鳴る、ループする音(自動生成した DemoTone.wav)
    GameObject* emitter = CreateDemoGoal(scene, mode, { 0.f, 0.f }, 1, "SoundSource");
    auto* source = emitter->AddComponent<AudioSourceComponent>(L"Assets/Demo/DemoTone.wav");
    source->SetLoop(true);

    // 聞く側(青): ここが「耳の位置」になる。3Dは3Dカメラの向きで聞く(2Dは画面の上が正面)
    GameObject* listener = CreateDemoGoal(scene, mode, { 0.f, 0.f }, 6, "SoundListener");
    auto* ears = listener->AddComponent<AudioListenerComponent>();
    if (mode == DemoMode::Mode3D) ears->SetUse3DCamera(true);

    // 操作役(回す・動かす・距離の設定・表示)
    DemoCameraController* camera = cameraObject ? cameraObject->GetComponent<DemoCameraController>() : nullptr;
    GameObject* demo = scene.CreateObject("Sound3DDemo");
    demo->AddComponent<Sound3DDemoController>(mode)->Setup(emitter, listener, source, camera);
    return demo;
}
