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
    ID3D11DepthStencilState* depthStencilStateUI = nullptr;    // UIモード: 深度無視、常に手前に描画される
    ID3D11DepthStencilState* depthStencilStateWorld = nullptr; // Worldモード: 深度テストあり、3Dメッシュと正しく前後する
    ID3D11RasterizerState* rasterizerStateWorld = nullptr;     // Worldモード: カリング無効(裏面も描画する)

    float resW = 0, resH = 0;
    int   texWidth = 0, texHeight = 0;

    const Camera* camera = nullptr;    // 2D用: 設定されていれば、描画時にこのカメラのx,yぶんだけ画面をずらす
    const Camera* camera3D = nullptr;  // Worldモード用: 深度バッファに書き込むZ値の計算に使う

    bool CreateShaders();
    bool CreateBuffers();
    bool CreateSampler();
    bool CreateBlendState();
    bool CreateDepthStencilState();
    bool CreateRasterizerState();
    XMMATRIX BuildWorldMatrix(const Transform& transform, bool worldSpace,
        bool lockX, bool lockY, bool lockZ) const;

    // Worldモード用: 3Dメッシュと同じ(camera3Dの本物のView/Projection行列を使う)、
    // 常にカメラの方を向く板(ビルボード)としての行列を組み立てる。
    // lockX/lockY/lockZ: trueにした軸はカメラに合わせて回転させず固定する
    XMMATRIX BuildBillboardMatrix(const Transform& transform, bool lockX, bool lockY, bool lockZ) const;

public:
    bool Init();
    // worldSpace = false(既定): UIのように常に手前に描画される(今までの挙動)
    // worldSpace = true        : 3Dオブジェクトのように、奥行きで前後関係が決まる
    void Draw(Transform transform, XMFLOAT4 color, const SpriteSheet& sheet = SpriteSheet(), bool worldSpace = false,
        bool lockX = false, bool lockY = false, bool lockZ = false);
    void Uninit();

    int GetTextureWidth()  const { return texWidth; }
    int GetTextureHeight() const { return texHeight; }

    void SetTexture(ID3D11ShaderResourceView* srv) { this->srv = srv; }
    void SetTextureSize(int w, int h) { texWidth = w; texHeight = h; }

    // このカメラの position(x, y)ぶんだけ、以降の描画位置をずらすようになる
    void SetCamera(const Camera* cam) { camera = cam; }
    // Worldモードで使う、3D用カメラ(奥行きZの計算に使う)
    void SetCamera3D(const Camera* cam) { camera3D = cam; }
};
