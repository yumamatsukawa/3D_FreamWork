#include "ShapesDemo.h"
#include "../../Components/Render3D/ShapesDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    using Row = ShapesDemoController::Row;

    constexpr float groundTop = -50.f;     // 地面の上面の高さ(DemoGroundと同じ)
    constexpr float spacing = 230.f;       // 見本を並べる間隔(X)
    constexpr float shapesZ = -250.f;      // 手前の列(形)のZ
    constexpr float spheresZ = 110.f;      // 真ん中の列(球)のZ
    constexpr float cubesZ = 470.f;        // 奥の列(立方体)のZ
    constexpr float sphereSize = 120.f;    // 球の直径
    constexpr float cubeSize = 110.f;      // 立方体の一辺

    // 見本1つを置く(column: 左から何番目か。-2〜2。scale: 大きさ。カプセルだけは実際の大きさで作るので1)
    void AddSample(Scene& scene, ShapesDemoController* controller, Row row, float z, int column, const std::vector<MeshVertex>& vertices,
        float scale, float halfHeight, const std::wstring& label, const std::wstring& detail, const Material& material) {
        GameObject* sample = scene.CreateObject("ShapeSample");
        sample->transform.position = { spacing * (float)column, groundTop + halfHeight, z };
        sample->transform.scale = { scale, scale, scale };

        // 半透明の物は、不透明の物より後に描く(drawPriorityを大きくする)
        if (material.color.w < 1.f) sample->SetDrawPriority(10);

        // 見た目: マテリアルをまとめて設定する
        sample->AddComponent<MeshRenderer>(vertices)->SetMaterial(material);

        controller->AddSample(sample, row, label, detail, halfHeight + 45.f);
    }

    // 形の見本のマテリアル(色だけ変えて、ハイライトを少し付けて立体が分かるようにする)
    Material MakeShapeMaterial(DirectX::XMFLOAT4 color) {
        Material material;
        material.color = color;
        material.specularStrength = 0.5f;
        return material;
    }

    // 手前の列: 形(立方体・粗い球・細かい球・カプセル・OBJ)
    void CreateShapesRow(Scene& scene, ShapesDemoController* controller) {
        AddSample(scene, controller, Row::Shapes, shapesZ, -2, Mesh::CreateCube(), cubeSize, cubeSize * 0.5f,
            L"立方体", L"CreateCube()", MakeShapeMaterial({ 0.9f, 0.6f, 0.3f, 1.0f }));
        AddSample(scene, controller, Row::Shapes, shapesZ, -1, Mesh::CreateSphere(4, 6), sphereSize, sphereSize * 0.5f,
            L"球(粗い)", L"CreateSphere(4, 6)", MakeShapeMaterial({ 0.4f, 0.8f, 0.5f, 1.0f }));
        AddSample(scene, controller, Row::Shapes, shapesZ, 0, Mesh::CreateSphere(16, 24), sphereSize, sphereSize * 0.5f,
            L"球(細かい)", L"CreateSphere(16, 24)", MakeShapeMaterial({ 0.4f, 0.8f, 0.5f, 1.0f }));
        AddSample(scene, controller, Row::Shapes, shapesZ, 1, Mesh::CreateCapsule(45.f, 170.f), 1.f, 85.f,
            L"カプセル", L"CreateCapsule(45, 170)", MakeShapeMaterial({ 0.5f, 0.6f, 0.95f, 1.0f }));

        // OBJファイル(読み込めなかった時は何も置かない)
        const std::vector<MeshVertex> objVertices = Mesh::LoadOBJ(L"Assets/test_cube.obj");
        if (!objVertices.empty()) {
            Material textured = MakeShapeMaterial({ 1.0f, 1.0f, 1.0f, 1.0f });
            textured.texture = L"Assets/grid.png";
            AddSample(scene, controller, Row::Shapes, shapesZ, 2, objVertices, cubeSize, cubeSize * 0.5f,
                L"OBJ読み込み", L"LoadOBJ(test_cube.obj)", textured);
        }
    }

    // 真ん中の列: 球(同じオレンジ色で、光沢・発光・縁の光を変える)
    void CreateSpheresRow(Scene& scene, ShapesDemoController* controller) {
        const std::vector<MeshVertex> sphere = Mesh::CreateSphere();

        Material base;
        base.color = { 0.9f, 0.5f, 0.3f, 1.0f };
        AddSample(scene, controller, Row::Spheres, spheresZ, -2, sphere, sphereSize, sphereSize * 0.5f, L"標準", L"(既定のまま)", base);

        Material glossy = base;                     // 光沢(鋭い): 光が当たる所に小さく強いハイライト
        glossy.specularStrength = 0.9f;
        glossy.shininess = 64.f;
        AddSample(scene, controller, Row::Spheres, spheresZ, -1, sphere, sphereSize, sphereSize * 0.5f, L"光沢(鋭い)", L"shininess 64", glossy);

        Material satin = base;                      // 光沢(ぼんやり): shininessを小さくすると、ハイライトが広がる
        satin.specularStrength = 0.9f;
        satin.shininess = 8.f;
        AddSample(scene, controller, Row::Spheres, spheresZ, 0, sphere, sphereSize, sphereSize * 0.5f, L"光沢(ぼんやり)", L"shininess 8", satin);

        Material glowing = base;                    // 発光: 光が当たらない所でも、自分で光る
        glowing.color = { 0.2f, 0.4f, 0.3f, 1.0f };
        glowing.emissiveColor = { 0.3f, 1.0f, 0.5f };
        glowing.emissiveStrength = 0.6f;
        AddSample(scene, controller, Row::Spheres, spheresZ, 1, sphere, sphereSize, sphereSize * 0.5f, L"発光", L"emissiveStrength", glowing);

        Material rim = base;                        // 縁の光: 斜めに見える縁が青く光る
        rim.color = { 0.3f, 0.3f, 0.35f, 1.0f };
        rim.rimColor = { 0.3f, 0.7f, 1.0f };
        rim.rimStrength = 1.5f;
        AddSample(scene, controller, Row::Spheres, spheresZ, 2, sphere, sphereSize, sphereSize * 0.5f, L"縁の光", L"rimStrength", rim);
    }

    // 奥の列: 立方体(色・画像・半透明・光を無視・枠線)
    void CreateCubesRow(Scene& scene, ShapesDemoController* controller) {
        const std::vector<MeshVertex> cube = Mesh::CreateCube();

        Material red;
        red.color = { 0.9f, 0.2f, 0.2f, 1.0f };
        AddSample(scene, controller, Row::Cubes, cubesZ, -2, cube, cubeSize, cubeSize * 0.5f, L"色", L"color", red);

        Material textured;
        textured.texture = L"Assets/grid.png";
        AddSample(scene, controller, Row::Cubes, cubesZ, -1, cube, cubeSize, cubeSize * 0.5f, L"画像", L"texture", textured);

        Material glass;                             // 半透明: 色のaを下げて、深度を書かない
        glass.color = { 0.4f, 0.7f, 1.0f, 0.4f };
        glass.depthWrite = false;
        glass.specularStrength = 0.9f;
        AddSample(scene, controller, Row::Cubes, cubesZ, 0, cube, cubeSize, cubeSize * 0.5f, L"半透明", L"color.w + depthWrite", glass);

        Material unlit;                             // 光を無視: 光が当たらなくても、いつも同じ明るさ
        unlit.color = { 1.0f, 1.0f, 0.4f, 1.0f };
        unlit.lit = false;
        AddSample(scene, controller, Row::Cubes, cubesZ, 1, cube, cubeSize, cubeSize * 0.5f, L"光を無視", L"lit = false", unlit);

        Material outlined;                          // 枠線: 画面上で3ピクセルの太さの線
        outlined.color = { 0.6f, 0.6f, 0.7f, 1.0f };
        outlined.outlineColor = { 1.0f, 0.9f, 0.2f, 1.0f };
        outlined.outlineWidth = 3.f;
        AddSample(scene, controller, Row::Cubes, cubesZ, 2, cube, cubeSize, cubeSize * 0.5f, L"枠線", L"outlineWidth", outlined);
    }
}

GameObject* CreateShapesDemo(Scene& scene) {
    // 操作役(見本の表示のオン/オフ・形の回転・名前の表示)
    GameObject* demo = scene.CreateObject("ShapesDemo");
    ShapesDemoController* controller = demo->AddComponent<ShapesDemoController>();

    CreateShapesRow(scene, controller);
    CreateSpheresRow(scene, controller);
    CreateCubesRow(scene, controller);
    return demo;
}
