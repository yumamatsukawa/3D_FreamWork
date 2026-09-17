#include "Image.h"
using namespace DirectX;

namespace Image
{
    Sprite sprite;
    Camera mainCamera;

    Camera& GetCamera() { return mainCamera; }

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

    void Draw(Transform transform, unsigned int texID) {
        if (texID >= textures.size()) return;
        auto& tex = textures[texID];
        if (!tex.srv) return;

        sprite.SetTexture(tex.srv);
        sprite.SetTextureSize(tex.width, tex.height);
        sprite.Draw(transform, textures[texID].color, tex.sheet);
    }

    void BeginFrame() {
        float clearColor[4] = { 0.f, 0.f, 0.5f, 1.f };
        Graphics::context->ClearRenderTargetView(Graphics::renderTarget, clearColor);

        Graphics::context->OMSetRenderTargets(1, &Graphics::renderTarget, nullptr);
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