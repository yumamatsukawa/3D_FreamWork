#include "Image.h"
using namespace DirectX;

namespace Image
{
    Sprite sprite;
    Camera mainCamera;    // 2Dスプライト用
    Camera mainCamera3D;  // 3Dメッシュ用(2D用とはあえて別インスタンス)
    Light mainLight;      // 3Dメッシュのライティング用

    bool camera3DAutoSync = true;

    Camera& GetCamera() { return mainCamera; }
    Camera& GetCamera3D() { return mainCamera3D; }
    Light& GetLight() { return mainLight; }
    void SetCamera3DAutoSync(bool enabled) { camera3DAutoSync = enabled; }

    struct TextureData {
        ID3D11ShaderResourceView* srv = nullptr;
        int width = 0, height = 0;
        SpriteSheet sheet;
        XMFLOAT4 color = { 1.f, 1.f, 1.f, 1.f };
    };
    std::vector<TextureData> textures;

    bool Init(HWND hwnd, int width, int height) {
        if (!Graphics::Init(hwnd, width, height)) return false;
        if (!sprite.Init()) return false;
        sprite.SetCamera(&mainCamera);
        sprite.SetCamera3D(&mainCamera3D);
        return true;
    }

    unsigned int LoadTexture(const std::wstring& filepath, int cols, int rows) {
        Texture::LoadResult result = Texture::Load(filepath);
        if (!result.srv) return UINT_MAX;

        TextureData data;
        data.srv = result.srv;
        data.width = result.width;
        data.height = result.height;
        data.sheet = { cols, rows, 0 };

        textures.push_back(data);
        return (unsigned int)(textures.size() - 1);
    }

    void ReleaseTexture(unsigned int id) {
        if (id >= textures.size()) return;
        if (textures[id].srv) { textures[id].srv->Release(); textures[id].srv = nullptr; }
    }

    void ReleaseAllTextures() {
        for (auto& t : textures)
            if (t.srv) t.srv->Release();
        textures.clear();
    }

    void SetSpriteIndex(unsigned int id, int index) {
        if (id >= textures.size()) return;
        textures[id].sheet.index = index;
    }

    void Draw(Transform transform, unsigned int texID, bool worldSpace,
        bool lockX, bool lockY, bool lockZ) {
        if (texID >= textures.size()) return;
        auto& tex = textures[texID];
        if (!tex.srv) return;

        sprite.SetTexture(tex.srv);
        sprite.SetTextureSize(tex.width, tex.height);
        sprite.Draw(transform, textures[texID].color, tex.sheet, worldSpace, lockX, lockY, lockZ);
    }

    ID3D11ShaderResourceView* GetTextureView(unsigned int texID) {
        if (texID >= textures.size()) return nullptr;
        return textures[texID].srv;
    }

    void BeginFrame() {
        // CameraController(一人称/三人称カメラ)などが3Dカメラを自分で操作している間は、
        // ここで毎フレーム上書きしてしまわないよう、自動追従を止める
        if (camera3DAutoSync) {
            // 3Dカメラを2Dカメラのx,y移動量に合わせて平行移動させる。
            // position/targetを同じ量だけ動かすので「向き」は変わらず、カメラごと
            // スライドするだけになる。これにより3Dオブジェクトも2D側と同じように
            // ワールド座標に固定されたまま、プレイヤーが動くと画面上を正しく
            // スクロールして離れていくようになる(2Dのdepth.z/target.zは触らない)。
            // ★ Yだけ符号を反転させる: 2D(Sprite)はposition.y+=画面下方向だが、
            //   3D(LookAtLH, up=(0,1,0))はY+=上方向なので、そのままコピーすると
            //   2Dと3Dでオブジェクトが逆方向にスクロールしてしまう
            mainCamera3D.position.x = mainCamera.position.x;
            mainCamera3D.position.y = -mainCamera.position.y;
            mainCamera3D.target.x = mainCamera.position.x;
            mainCamera3D.target.y = -mainCamera.position.y;

            // ★ 2D(疑似的な正射影、距離に関係なく一定速度でスクロール)と
            //   3D(本物の透視投影、遠近感でスクロール速度が変わる)の間で、
            //   focalLengthの距離にあるオブジェクトだけは同じ速度で動いて見えるように、
            //   fovYを逆算する。もっと遠い/近いものはズレるが、それは正しい遠近感として扱う
            if (Graphics::height > 0.f) {
                float halfH = Graphics::height * 0.5f;
                mainCamera3D.fovY = 2.0f * atanf(halfH / mainCamera3D.focalLength);
            }
        }

        float clearColor[4] = { 0.f, 0.f, 0.5f, 1.f };
        Graphics::context->ClearRenderTargetView(Graphics::renderTarget, clearColor);
        Graphics::context->ClearDepthStencilView(Graphics::depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

        Graphics::context->OMSetRenderTargets(1, &Graphics::renderTarget, Graphics::depthStencilView);
    }

    void EndFrame() {
        Graphics::swapChain->Present(1, 0);
    }

    void Uninit() {
        ReleaseAllTextures();
        sprite.Uninit();
        Graphics::Uninit();
    }

    void SetColor(unsigned int texID, float r, float g, float b, float a)
    {
        textures[texID].color = { r, g, b, a };
    }

    XMFLOAT4 GetColor(unsigned int texID)
    {
        return textures[texID].color;
    }
}