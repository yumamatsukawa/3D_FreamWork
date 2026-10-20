#include "ParticleDemo.h"
#include "../../Components/Render/ParticleDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateParticleDemo(Scene& scene, ParticleRenderer* campfire, DemoMode mode) {
    GameObject* demo = scene.CreateObject("ParticleDemo");
    // 操作と表示、と、表示する値の持ち主
    demo->AddComponent<ParticleDemoController>(mode)->Setup(campfire);
    return demo;
}
