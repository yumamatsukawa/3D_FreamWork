#include "Graphics.h"

namespace Graphics {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* renderTarget = nullptr;
    ID3D11DepthStencilView* depthStencilView = nullptr;
    float                   width = 0.f;
    float                   height = 0.f;

    bool Init(HWND hwnd, int w, int h) {
        width = (float)w;
        height = (float)h;

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = w;
        sd.BufferDesc.Height = h;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;

        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &sd,
            &swapChain,
            &device,
            &featureLevel,
            &context
        );
        if (FAILED(hr)) return false;

        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        device->CreateRenderTargetView(backBuffer, nullptr, &renderTarget);
        backBuffer->Release();

        // ★ 深度バッファ(Z-buffer)を作成する。3Dメッシュの前後関係を正しく描画するために使う
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = w;
        depthDesc.Height = h;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D* depthTexture = nullptr;
        hr = device->CreateTexture2D(&depthDesc, nullptr, &depthTexture);
        if (FAILED(hr)) return false;
        hr = device->CreateDepthStencilView(depthTexture, nullptr, &depthStencilView);
        depthTexture->Release();
        if (FAILED(hr)) return false;

        context->OMSetRenderTargets(1, &renderTarget, depthStencilView);

        D3D11_VIEWPORT vp = {};
        vp.Width = (float)w;
        vp.Height = (float)h;
        vp.MaxDepth = 1.f;
        context->RSSetViewports(1, &vp);

        return true;
    }

    void Uninit() {
        if (renderTarget) { renderTarget->Release(); renderTarget = nullptr; }
        if (depthStencilView) { depthStencilView->Release(); depthStencilView = nullptr; }
        if (swapChain) { swapChain->Release();    swapChain = nullptr; }
        if (context) { context->Release();      context = nullptr; }
        if (device) { device->Release();       device = nullptr; }
    }
}