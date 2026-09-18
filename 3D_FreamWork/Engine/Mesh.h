#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include "Transform.h"
#include "Camera.h"
#include "Light.h"
#pragma comment(lib, "d3dcompiler.lib")

struct MeshVertex {
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT4 Color;
    XMFLOAT2 TexCoord;
};

struct MeshConstantBuffer {
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX proj;
    XMFLOAT4 lightDir;
    XMFLOAT4 lightColor;
    XMFLOAT4 ambient;
};

// 3Dの頂点データ(立体)を持ち、World/View/Projection行列を使って描画するクラス。
// SpriteがZ軸回転のみ・奥行き無しの2D専用だったのに対し、こちらはXYZ全軸の回転や
// 奥行きのある立体を、深度バッファ有り(前後関係が正しい)で描画できる。
class Mesh {
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
    ID3D11DepthStencilState* depthStencilState = nullptr;
    ID3D11RasterizerState* rasterizerState = nullptr;

    std::vector<MeshVertex> vertices;
    float aspectRatio = 1.f;

    bool CreateShaders();
    bool CreateBuffers();
    bool CreateSampler();
    bool CreateBlendState();
    bool CreateDepthStencilState();
    bool CreateRasterizerState();

public:
    bool Init(const std::vector<MeshVertex>& verts);
    void Draw(const Transform& transform, const Camera& camera, XMFLOAT4 color, const Light& light);
    void Uninit();

    void SetTexture(ID3D11ShaderResourceView* s) { srv = s; }

    // 単位立方体(-0.5〜0.5)の頂点データを作る。obj->transform.scaleで大きさを変えられる
    static std::vector<MeshVertex> CreateCube();

    // 半径0.5の単位球の頂点データを作る。obj->transform.scaleで大きさを変えられる。
    // rings: 緯度方向の分割数、segments: 経度方向の分割数(大きいほど滑らかで頂点数が増える)
    static std::vector<MeshVertex> CreateSphere(int rings = 16, int segments = 24);

    // Wavefront OBJ(.obj)ファイルを読み込んで頂点データを作る。
    // v(頂点座標)/vt(UV座標)/f(面)に対応。四角形以上の面は三角形に自動分割する。
    // 読み込みに失敗した場合は空のvectorを返す
    static std::vector<MeshVertex> LoadOBJ(const std::wstring& filepath);
};
