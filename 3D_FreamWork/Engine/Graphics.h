#pragma once
#include <d3d11.h>
#include <windows.h>
#pragma comment(lib, "d3d11.lib")

namespace Graphics {
    extern ID3D11Device* device;
    extern ID3D11DeviceContext* context;
    extern IDXGISwapChain* swapChain;
    extern ID3D11RenderTargetView* renderTarget;
    extern float                    width;
    extern float                    height;

    bool Init(HWND hwnd, int w, int h);
    void Uninit();
}