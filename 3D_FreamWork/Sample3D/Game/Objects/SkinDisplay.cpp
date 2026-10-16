#include "SkinDisplay.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ModelRenderer.h"
#include "../../../Engine/ModelAnimationComponent.h"
#include "../Components/HighlightController.h"
#include <Windows.h>

GameObject* CreateSkinDisplay(Scene& scene) {
    const wchar_t* path = L"Assets/Models/TestSkin.fbx";

    // ファイルが無い時は何も出さない(あってもなくてもサンプルが動くように)
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
        OutputDebugStringW(L"★ CreateSkinDisplay: FBXが見つからないので置きません: Assets/Models/TestSkin.fbx\n");
        return nullptr;
    }

    // 地面(上面の高さ-50)の上、ModelDisplayの隣に置く
    GameObject* display = scene.CreateObject("SkinDisplay");
    display->transform.position = { 1000.0f, -50.0f, 800.0f };

    // 見た目(骨で曲げるFBXモデル。骨のパーツ自体は描かれず、箱が曲がる)
    auto* renderer = display->AddComponent<ModelRenderer>(path);
    renderer->GetMaterial().castShadow = true;      // 骨で曲げた形のまま、地面に影が落ちる
    renderer->GetMaterial().receiveShadow = true;

    // アニメーション "Bend"(先端の骨が曲がって戻る)をループ再生。ModelRendererの後に付ける
    auto* animation = display->AddComponent<ModelAnimationComponent>();
    animation->Play("Bend");

    // Playerが近づくと、骨で曲げた形の外側に枠線が光る(スキンメッシュにも枠線が付く)
    display->AddComponent<HighlightController>()->Setup({ 0.3f, 1.0f, 0.5f, 1.0f }, 3.0f, 700.0f);

    return display;
}