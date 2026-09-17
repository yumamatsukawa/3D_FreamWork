#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include "Transform.h"
#include "Camera.h"
#pragma comment(lib, "d3dcompiler.lib")

struct SpriteSheet {
    int cols = 1;
    int rows = 1;
    int index = 0;
};

struct Vertex {
    XMFLOAT2 Position;
    XMFLOAT4 Color;
    XMFLOAT2 TexCoord;
};

struct ConstantBuffer
{
    XMMATRIX world;   // 64 バイト, アライメント OK
};

class Sprite
{
private:
    ID3D11Buffer* constantBuffer = nullptr;
    ID3D11DeviceContext* context = nullptr;
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    ID3D11BlendState* blendState = nullptr;

    float resW = 0, resH = 0;
    int   texWidth = 0, texHeight = 0;

    const Camera* camera = nullptr;  // 設定されていれば、描画時にこのカメラの位置ぶんだけ画面をずらす

    bool CreateShaders();
    bool CreateBuffers();
    bool CreateSampler();
    bool CreateBlendState();
    XMMATRIX BuildWorldMatrix(const Transform& transform) const;

public:
    bool Init();
    void Draw(Transform transform, XMFLOAT4 color, const SpriteSheet& sheet = SpriteSheet());
    void Uninit();

    int GetTextureWidth()  const { return texWidth; }
    int GetTextureHeight() const { return texHeight; }

    void SetTexture(ID3D11ShaderResourceView* srv) { this->srv = srv; }
    void SetTextureSize(int w, int h) { texWidth = w; texHeight = h; }

    // このカメラの position(x, y)ぶんだけ、以降の描画位置をずらすようになる
    void SetCamera(const Camera* cam) { camera = cam; }
};
