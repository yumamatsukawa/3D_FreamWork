#include "Texture.h"
#include "Graphics.h"
#include "WICTextureLoader11.h"

namespace Texture {
    LoadResult Load(const std::wstring& filepath) {
        LoadResult result;

        ID3D11Resource* res = nullptr;
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            Graphics::device, Graphics::context,
            filepath.c_str(), &res, &result.srv);

        if (FAILED(hr)) {
            OutputDebugStringA("★ テクスチャ読み込み失敗\n");
            return result;
        }

        ID3D11Texture2D* tex2D = nullptr;
        if (res) {
            res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex2D);
            if (tex2D) {
                D3D11_TEXTURE2D_DESC desc;
                tex2D->GetDesc(&desc);
                result.width = (int)desc.Width;
                result.height = (int)desc.Height;
                tex2D->Release();
            }
            res->Release();
        }

        return result;
    }
}