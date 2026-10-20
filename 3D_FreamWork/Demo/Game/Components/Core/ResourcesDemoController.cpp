#include "ResourcesDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/Resources.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include <climits>
#include <cstdio>

namespace {
    const std::wstring texturePath = L"Assets/box.png";           // 何枚置いても読み込みは1回の画像
    const std::wstring soundPath = L"Assets/Sounds/shot.wav";     // 2回借りる音
    const std::wstring modelPath = L"Assets/Models/TestParts.fbx";   // 2回借りるモデル

    constexpr float infoX = -610.f;       // 状態の文字の左端
    constexpr float infoY = 190.f;        // 状態の1行目(説明の板の下)
    constexpr float infoGap = 30.f;       // 行の間隔
    constexpr float gridLeft = 70.f;      // 画像の並びの左端
    constexpr float gridTop = 150.f;      // 画像の並びの1行目
    constexpr float gridGap = 48.f;       // 画像の間隔
}

void ResourcesDemoController::CheckTextureId() {
    // 1回借りてすぐ返す(借りた回数は元に戻る)。同じ画像なら、いつも同じIDが返る
    textureId = Resources::LoadTexture(texturePath);
    Resources::ReleaseTexture(textureId);
}

void ResourcesDemoController::AddSprites(int count) {
    // 同じ画像のスプライトを置く(SpriteRendererが中で Resources::LoadTexture を呼ぶ。2枚目からは読み込まずにIDを共有する)
    for (int i = 0; i < count && (int)sprites.size() < maxSprites; i++) {
        const int index = (int)sprites.size();
        GameObject* sprite = GetOwner()->GetScene()->CreateObject("ResourceSprite");
        sprite->transform.position = { gridLeft + gridGap * (float)(index % columns), gridTop - gridGap * (float)(index / columns), 0.f };
        sprite->transform.scale = { 38.f, 38.f, 1.f };
        sprite->AddComponent<SpriteRenderer>(texturePath, 3, 3)->SetSpriteIndex(index % 9);
        sprites.push_back(sprite);
    }
    CheckTextureId();
    SetMessage(L"画像を増やしました。", L"何枚でも読み込んだのは1回だけ");
}

void ResourcesDemoController::DestroySprites() {
    // Destroyすると、SpriteRendererが画像を返す(参照は減るが、メモリにはまだ残る)
    for (GameObject* sprite : sprites) sprite->Destroy();
    sprites.clear();
    SetMessage(L"画像をすべて消しました。", L"借りる人は0だが、まだ残っている");
}

void ResourcesDemoController::UnloadUnused() {
    // 誰も借りていない画像・音・モデルを解放する(借りている物は残る)
    Resources::UnloadUnused();
    textureId = UINT_MAX;
    if (sprites.empty()) SetMessage(L"UnloadUnused: 使われていない", L"画像を解放した(次は読み直す)");
    else SetMessage(L"UnloadUnused: 画像はまだ", L"使われているので解放されない");
}

void ResourcesDemoController::LoadSounds() {
    // 同じ音を2回借りる(読み込みは1回。IDは同じになる)
    soundIds.push_back(Resources::LoadSound(soundPath));
    soundIds.push_back(Resources::LoadSound(soundPath));
    SetMessage(L"音を2回借りました。", L"IDが同じなら読み込みは1回");
}

void ResourcesDemoController::LoadModels() {
    // 同じモデルを2回借りる(読み込みは1回。同じ物を指す)
    models.push_back(Resources::LoadModel(modelPath));
    models.push_back(Resources::LoadModel(modelPath));
    SetMessage(L"モデルを2回借りました。", L"同じ物を指せば読み込みは1回");
}

void ResourcesDemoController::ReleaseHeld() {
    // 借りている音・モデルを返す(解放されるのは UnloadUnused を呼んだ時)
    for (unsigned int id : soundIds) Resources::ReleaseSound(id);
    soundIds.clear();
    models.clear();
    SetMessage(L"音とモデルを返しました。", L"UnloadUnused で解放される");
}

void ResourcesDemoController::Draw() {
    wchar_t buf[128];
    float y = infoY;

    // 画像: 置いた数と、box.png のID(全員同じ)
    swprintf_s(buf, L"置いた画像 (box.png)  %d 枚", (int)sprites.size());
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(22.f)); y -= infoGap;
    if (textureId == UINT_MAX) swprintf_s(buf, L"LoadTexture の ID  - (未確認 / 解放済み)");
    else swprintf_s(buf, L"LoadTexture の ID  %u  (全部同じ)", textureId);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(22.f, { 1.0f, 0.9f, 0.4f, 1.0f })); y -= infoGap;

    // 音: 借りたIDの並び
    std::wstring sounds = L"-";
    if (!soundIds.empty()) {
        sounds.clear();
        for (unsigned int id : soundIds) sounds += (id == UINT_MAX ? L"失敗" : std::to_wstring(id)) + L" ";
    }
    Text::Draw(L"LoadSound の ID  " + sounds, infoX, y, MakeDemoTextStyle(22.f, { 0.7f, 1.0f, 0.7f, 1.0f })); y -= infoGap;

    // モデル: 同じ物を指しているか
    std::wstring modelText = L"-";
    if (models.size() >= 2) modelText = (!models[0]) ? L"読み込み失敗" : (models[0].get() == models[1].get() ? L"同じ物を指している" : L"別の物");
    Text::Draw(L"LoadModel  " + modelText, infoX, y, MakeDemoTextStyle(22.f, { 0.6f, 0.9f, 1.0f, 1.0f })); y -= infoGap * 1.4f;

    // 最後の操作の結果(2行)
    Text::Draw(message1, infoX, y, MakeDemoTextStyle(20.f, { 0.9f, 0.9f, 0.9f, 1.0f }));
    Text::Draw(message2, infoX, y - 26.f, MakeDemoTextStyle(20.f, { 0.9f, 0.9f, 0.9f, 1.0f }));
}

void ResourcesDemoController::Uninit() {
    // 借りた音とモデルを返す(置いた画像のスプライトは、Sceneが片付ける時にSpriteRendererが返す)
    ReleaseHeld();
}
