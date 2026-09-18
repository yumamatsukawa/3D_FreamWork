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

    // 2Dスプライト用のカメラ。position(x,y)を動かすと画面がスクロールする
    // 例: Image::GetCamera().position = player->transform.position;
    Camera& GetCamera();

    // 3Dメッシュ(MeshRenderer)用のカメラ。2D用のGetCamera()とはあえて別インスタンス。
    // 2Dカメラはプレイヤー追従などで毎フレーム動くことが多く、同じカメラを3D側でも
    // 使うと「3Dオブジェクトがカメラに合わせて動いて見える」おかしな挙動になるため
    Camera& GetCamera3D();

    unsigned int LoadTexture(const std::wstring& filepath, int cols = 1, int rows = 1);
    void         ReleaseTexture(unsigned int id);
    void         ReleaseAllTextures();
    void         SetSpriteIndex(unsigned int id, int index);

    // worldSpace = false(既定): UIのように常に手前に描画される
    // worldSpace = true        : 3Dオブジェクトのように、奥行きで前後関係が決まる
    void Draw(Transform transform, unsigned int texID, bool worldSpace = false);

    // Mesh(3D)など、Sprite以外の描画にテクスチャを使い回したい時に、生のSRVを取り出す
    ID3D11ShaderResourceView* GetTextureView(unsigned int texID);

    void SetColor(unsigned int texID, float r, float g, float b, float a);

    XMFLOAT4 GetColor(unsigned int texID);
}