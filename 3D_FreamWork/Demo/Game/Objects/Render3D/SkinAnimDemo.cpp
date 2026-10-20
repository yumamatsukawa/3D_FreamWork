#include "SkinAnimDemo.h"
#include "../../Components/Render3D/SkinAnimDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/ModelRenderer.h"
#include "../../../../Engine/Animation/ModelAnimationComponent.h"

namespace {
    constexpr float groundTop = -50.f;      // 地面の上面の高さ(DemoGroundと同じ)
    constexpr float modelZ = 100.f;         // モデルを置く奥行き
    constexpr float boxX = -420.f;          // 箱のx
    constexpr float loopArmX = 0.f;         // ずっと曲がる腕のx
}

GameObject* CreateSkinArm(Scene& scene, float x, bool loop) {
    GameObject* arm = scene.CreateObject(loop ? "SkinArmLoop" : "SkinArmOnce");
    arm->transform.position = { x, groundTop, modelZ };

    // 見た目(骨で曲げるFBX。骨のパーツ自体は描かれず、細長い箱が曲がる)
    arm->AddComponent<ModelRenderer>(L"Assets/Models/TestSkin.fbx");

    // アニメーション "Bend"(先端の骨が曲がって戻る)。ModelRendererの後に付ける
    arm->AddComponent<ModelAnimationComponent>()->Play("Bend", loop);
    return arm;
}

GameObject* CreateSkinAnimDemo(Scene& scene) {
    // 左: 骨の無い箱(Spin と Bob の2つのクリップ)
    GameObject* box = scene.CreateObject("AnimBox");
    box->transform.position = { boxX, groundTop, modelZ };
    box->AddComponent<ModelRenderer>(L"Assets/Models/TestAnim.fbx");
    ModelAnimationComponent* boxAnimation = box->AddComponent<ModelAnimationComponent>();
    boxAnimation->Play("Spin");

    // 真ん中: ずっと曲がる腕、右: 1回だけ曲がる腕
    GameObject* loopArm = CreateSkinArm(scene, loopArmX, true);
    GameObject* onceArm = CreateSkinArm(scene, onceArmX, false);

    // 操作役(クリップの切り替え・速さ・名前と状態の表示)
    GameObject* demo = scene.CreateObject("SkinAnimDemo");
    demo->AddComponent<SkinAnimDemoController>()->Setup(box, boxAnimation, loopArm, onceArm);
    return demo;
}
