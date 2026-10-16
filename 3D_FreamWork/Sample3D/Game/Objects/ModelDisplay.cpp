#include "ModelDisplay.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ModelRenderer.h"
#include "../../../Engine/ModelAnimationComponent.h"
#include "../Components/ModelDisplayController.h"
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

    return display;
}