#pragma once
#include <d3d11.h>
#include <string>

namespace Texture {
    // 読み込むだけ。サイズも一緒に返す
    struct LoadResult {
        ID3D11ShaderResourceView* srv = nullptr;
        int width = 0;
        int height = 0;
    };

    LoadResult Load(const std::wstring& filepath);
}