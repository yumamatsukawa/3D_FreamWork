#include "ModelDemo.h"
#include "../../Components/Render3D/ModelDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/ModelRenderer.h"

namespace {
    constexpr float groundTop = -50.f;      // 地面の上面の高さ(DemoGroundと同じ)
    const wchar_t* modelPath = L"Assets/Models/TestParts.fbx";   // 置くFBX(大きな箱の上に、小さな箱が乗った2パーツ)

    // モデルを1体置く(x: 横の位置。モデルの底が地面に着くよう、地面の上面に置く)
    ModelRenderer* CreateModelObject(Scene& scene, const char* name, float x) {
        GameObject* object = scene.CreateObject(name);
        object->transform.position = { x, groundTop, 100.f };
        ModelRenderer* renderer = object->AddComponent<ModelRenderer>(modelPath);
        renderer->GetMaterial().specularStrength = 0.3f;
        return renderer;
    }
}

GameObject* CreateModelDemo(Scene& scene) {
    // 同じFBXを2体置く(モデルのデータは Resources が共有する。パーツの今の動きは1体ごと)
    ModelRenderer* moving = CreateModelObject(scene, "ModelMoving", 0.f);
    ModelRenderer* reference = CreateModelObject(scene, "ModelReference", 420.f);

    // 操作役(パーツ・全体の動きと、モデルの情報の表示)
    GameObject* demo = scene.CreateObject("ModelDemo");
    demo->AddComponent<ModelDemoController>()->Setup(moving, reference);
    return demo;
}
