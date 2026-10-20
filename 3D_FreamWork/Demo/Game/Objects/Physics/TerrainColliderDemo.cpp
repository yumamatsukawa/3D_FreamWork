#include "TerrainColliderDemo.h"
#include "../../Components/Physics/TerrainColliderDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateTerrainColliderDemo(Scene& scene, TerrainRenderer* terrain, GameObject* walker) {
    GameObject* demo = scene.CreateObject("TerrainColliderDemo");
    // 表示と、表示する物
    demo->AddComponent<TerrainColliderDemoController>()->Setup(terrain, walker);
    return demo;
}
