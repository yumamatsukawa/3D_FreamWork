#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include "../Core/Transform.h"
#include "Camera.h"
#pragma comment(lib, "d3dcompiler.lib")

struct SpriteSheet {
    int cols = 1;
    int rows = 1;
    int index = 0;
};

// 画像の重ね方(パーティクルなどが使う。普通のスプライトはNormalのまま)
enum class BlendMode {
    Normal,     // 普通のアルファ合成(今までのスプライトと同じ)
    Additive,   // 加算合成(重なるほど明るくなる。炎・光向け)。colorのaで明るさを変える
    Soft,       // 画像の明るさを濃さとして重ねる、ふわっとした合成(煙向け)。colorのaで濃さを変える
                // ※Shader.hlsl は「アルファ0.5未満は描かない」ので、アルファで半透明にする普通の方法ではふわっとしない。
                //   そのためAdditive/Softは、画像の明るさ(RGB)で形と濃さを表す(アルファは不透明のままにする)
};

// スプライトの描き方(3種類)
enum class SpriteMode {
    UI,        // UI: zで大きさは変わらない。深度を使わず、必ず最前面(drawPriority → 作った順)。HUD・ボタン・背景など
    World2D,   // 2D: zで大きさは変わらない(遠近なし)。2Dカメラでスクロールし、zが小さい(手前)ほど手前に描く(深度バッファ)
    World3D    // 3D: 3Dカメラの透視(奥ほど小さい)。カメラの方を向く板(ビルボード)で、深度バッファで前後が決まる
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
    ID3D11BlendState* blendStateAdditive = nullptr;            // 加算合成(BlendMode::Additive)
    ID3D11BlendState* blendStateSoft = nullptr;                // ふわっとした合成(BlendMode::Soft)
    ID3D11DepthStencilState* depthStencilStateUI = nullptr;    // UIモード: 深度無視、常に手前に描画される
    ID3D11DepthStencilState* depthStencilStateWorld = nullptr; // Worldモード: 深度テストあり、3Dメッシュと正しく前後する
    ID3D11DepthStencilState* depthStencilStateWorldNoWrite = nullptr; // Worldモード: 深度テストだけ(深度は書かない。半透明の粒用)
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
    XMMATRIX BuildWorldMatrix(const Transform& transform, SpriteMode mode,
        bool lockX, bool lockY, bool lockZ) const;

    // Worldモード用: 3Dメッシュと同じ(camera3Dの本物のView/Projection行列を使う)、
    // 常にカメラの方を向く板(ビルボード)としての行列を組み立てる。
    // lockX/lockY/lockZ: trueにした軸はカメラに合わせて回転させず固定する
    XMMATRIX BuildBillboardMatrix(const Transform& transform, bool lockX, bool lockY, bool lockZ) const;

public:
    bool Init();
    // 注意: World2Dの深度は3Dカメラからの距離(viewZ)から作るので、zは「3Dカメラのz(既定-500)より+5以上奥」にすること(viewZ <= 0 は、深度が0=最前面になる。2DのzをカメラのzやそれよりZが小さい値にしない)
    // mode: SpriteMode(UI / World2D / World3D)。bool版の worldSpace は false=UI、true=World3D(今までの呼び方)
    // UIは深度を使わず描いた順に重なる(必ず最前面にしたい時はImage::Draw経由で積む)
    // blend: 画像の重ね方(BlendMode参照)。depthWrite: Worldモードで深度を書くか(半透明の粒はfalse。UIモードでは関係ない)
    void Draw(Transform transform, XMFLOAT4 color, const SpriteSheet& sheet = SpriteSheet(), bool worldSpace = false,
        bool lockX = false, bool lockY = false, bool lockZ = false,
        BlendMode blend = BlendMode::Normal, bool depthWrite = true);
    void Draw(Transform transform, XMFLOAT4 color, const SpriteSheet& sheet, SpriteMode mode,
        bool lockX, bool lockY, bool lockZ, BlendMode blend, bool depthWrite);
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
