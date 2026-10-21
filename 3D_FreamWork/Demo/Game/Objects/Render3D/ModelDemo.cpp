#include "ModelDemo.h"
#include "../../Components/Render3D/ModelDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/ModelRenderer.h"

namespace {
    constexpr float groundTop = -50.f;      // 地面の上面の高さ(DemoGroundと同じ)
    const wchar_t* modelPath = L"Assets/Models/TestParts.fbx";   // 置くFBX(大きな箱の上に、小さな箱が乗った2パーツ)
    const wchar_t* robotPath = L"Assets/Models/robot.fbx";   // 試しに読むFBX(自作のロボット。32パーツの親子、マテリアルは4色)

    // ロボットの大きさと置き場所。ロボットは元が 幅約2.6 x 高さ約6.0(1単位=1cmなので高さ約6cm)ととても小さいので、28倍にして高さ約167にする
    // (他のモデルの高さ約300前後と釣り合う大きさ)。足元は原点より約2.995(元の値)下にあるので、拡大した分(約84)だけ持ち上げて、足を地面に着ける
    // 置く高さ = 地面の上面 + 元の足元の距離 x 大きさ。顔は-Z側を向くので、-Z側にいる既定のカメラからは正面が見える(回さない)
    constexpr float robotScale = 28.f;
    constexpr float robotBottom = 2.995f * robotScale;   // 拡大した後の、原点から足元までの距離
    constexpr float robotX = -450.f;                     // ロボットの横の位置(動かす方の左)

    const wchar_t* colorPath = L"Assets/Models/TestColor.fbx";   // マテリアルの色つきのFBX(赤・緑・青の箱が3つ)
    constexpr float colorX = -150.f;                   // 色つきの箱の真ん中の横の位置(動かす方の奥)
    constexpr float colorZ = 450.f;                    // 色つきの箱の奥行き

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

    // 試しに読むFBXのロボットを置く(ファイルが無い・読めない時は何も描かず、出力ウィンドウに理由が出る。落ちない)
    GameObject* robotObject = scene.CreateObject("ModelRobot");
    robotObject->transform.position = { robotX, groundTop + robotBottom, 100.f };
    robotObject->transform.scale = { robotScale, robotScale, robotScale };
    ModelRenderer* robot = robotObject->AddComponent<ModelRenderer>(robotPath);
    robot->GetMaterial().specularStrength = 0.3f;

    // マテリアルの色つきのFBX(パーツごとに色が違う。FBXの色は ModelRenderer が自動で使う)
    GameObject* colorObject = scene.CreateObject("ModelColor");
    colorObject->transform.position = { colorX, groundTop, colorZ };
    ModelRenderer* colorModel = colorObject->AddComponent<ModelRenderer>(colorPath);
    colorModel->GetMaterial().specularStrength = 0.3f;

    // 操作役(パーツ・全体の動きと、モデルの情報の表示)
    GameObject* demo = scene.CreateObject("ModelDemo");
    demo->AddComponent<ModelDemoController>()->Setup(moving, reference, robot, colorModel);
    return demo;
}
