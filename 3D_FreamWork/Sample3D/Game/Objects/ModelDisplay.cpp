#include "ModelDisplay.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render3D/ModelRenderer.h"
#include "../../../Engine/Animation/ModelAnimationComponent.h"
#include "../Components/ModelDisplayController.h"
#include "../Components/HighlightController.h"
#include <Windows.h>

namespace {
    bool FileExists(const wchar_t* path) {
        return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
    }
}

GameObject* CreateModelDisplay(Scene& scene) {
    const wchar_t* animatedPath = L"Assets/Models/TestAnim.fbx";
    const wchar_t* staticPath = L"Assets/Models/TestParts.fbx";

    // ファイルが無い時は何も出さない(あってもなくてもサンプルが動くように)
    bool animated = FileExists(animatedPath);
    if (!animated && !FileExists(staticPath)) {
        OutputDebugStringW(L"★ CreateModelDisplay: FBXが見つからないので置きません: Assets/Models/\n");
        return nullptr;
    }

    // 地面(上面の高さ-50)の上、プレイヤーから離れた端に置く
    GameObject* display = scene.CreateObject("ModelDisplay");
    display->transform.position = { 800.0f, -50.0f, 800.0f };

    // 見た目(FBXモデル。パーツの親子関係を保ったまま描く)
    auto* renderer = display->AddComponent<ModelRenderer>(animated ? animatedPath : staticPath);
    renderer->GetMaterial().castShadow = true;
    renderer->GetMaterial().receiveShadow = true;

    // アニメーションの再生役(ModelRendererの後に付ける)。最初は "Spin"
    ModelAnimationComponent* animation = nullptr;
    if (animated) {
        animation = display->AddComponent<ModelAnimationComponent>();
        animation->Play("Spin");
    }

    // クリップの切り替え(アニメ無しの時は、Topパーツを手で回す)
    display->AddComponent<ModelDisplayController>(renderer, animation);

    // Playerが近づくと、枠線が光る(取れる物・調べられる物の合図)
    display->AddComponent<HighlightController>()->Setup({ 1.0f, 0.9f, 0.2f, 1.0f }, 3.0f, 700.0f);

    return display;
}