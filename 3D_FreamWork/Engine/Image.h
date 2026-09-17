#pragma once
#include "Graphics.h"
#include "Sprite.h"
#include "Texture.h"
#include "GameObject.h"
#include "Camera.h"
#include <vector>
#include <string>

namespace Image
{
    bool Init(HWND hwnd, int width, int height);
    void BeginFrame();
    void EndFrame();
    void Uninit();

    // 現在の描画に使われているカメラ。position(x,y)を動かすと画面がスクロールする
    // 例: Image::GetCamera().position = player->transform.position;
    Camera& GetCamera();

    unsigned int LoadTexture(const std::wstring& filepath, int cols = 1, int rows = 1);
    void         ReleaseTexture(unsigned int id);
    void         ReleaseAllTextures();
    void         SetSpriteIndex(unsigned int id, int index);

    void Draw(Transform transform, unsigned int texID);

    void SetColor(unsigned int texID, float r, float g, float b, float a);

    XMFLOAT4 GetColor(unsigned int texID);
}