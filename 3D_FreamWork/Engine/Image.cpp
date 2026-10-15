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

    namespace {
        // 2D描画の遠近の倍率(Sprite::BuildWorldMatrixと同じ式)。focalLengthの距離で等倍、カメラより手前なら等倍のまま
        float GetDepthScale2D(float worldZ) {
            float depth = worldZ - mainCamera.position.z;
            return (depth > 0.f) ? mainCamera.focalLength / depth : 1.f;
        }

        // 3D描画のアスペクト比(Mesh::Init・Sprite::BuildBillboardMatrixと同じ)
        float GetAspectRatio() {
            return (Graphics::height > 0.f) ? (Graphics::width / Graphics::height) : 1.f;
        }
    }

    // ワールド→画面: (ワールド - カメラ位置) × 遠近の倍率
    XMFLOAT2 WorldToScreen2D(XMFLOAT3 worldPosition) {
        float scale = GetDepthScale2D(worldPosition.z);
        return {
            (worldPosition.x - mainCamera.position.x) * scale,
            (worldPosition.y - mainCamera.position.y) * scale
        };
    }

    // 画面→ワールド: WorldToScreen2Dを Z=0 について逆にたどる(カメラ位置 + 画面座標 ÷ 遠近の倍率)
    XMFLOAT3 ScreenToWorld2D(XMFLOAT2 screenPosition) {
        float scale = GetDepthScale2D(0.f);
        return {
            mainCamera.position.x + screenPosition.x / scale,
            mainCamera.position.y + screenPosition.y / scale,
            0.f
        };
    }

    // ワールド→画面: view・projを掛けてwで割る(-1〜1の範囲)→ 画面の半分の大きさを掛けてピクセルにする
    XMFLOAT3 WorldToScreen3D(XMFLOAT3 worldPosition) {
        XMMATRIX viewProj = mainCamera3D.GetViewMatrix() * mainCamera3D.GetProjectionMatrix(GetAspectRatio());
        XMVECTOR clip = XMVector4Transform(XMVectorSet(worldPosition.x, worldPosition.y, worldPosition.z, 1.f), viewProj);

        // 透視投影では w = カメラからの奥行き。0以下はカメラの後ろ(割ると上下左右が逆になるので、x,yは返さない)
        float w = XMVectorGetW(clip);
        if (w <= 0.f) return { 0.f, 0.f, w };
        return {
            XMVectorGetX(clip) / w * (Graphics::width * 0.5f),
            XMVectorGetY(clip) / w * (Graphics::height * 0.5f),
            w
        };
    }

    // 画面→線: 画面座標を-1〜1の範囲に直し、view・projの逆行列で、手前の面(深さ0)と奥の面(深さ1)のワールド座標に戻す
    void ScreenPointToRay3D(XMFLOAT2 screenPosition, XMFLOAT3& origin, XMFLOAT3& direction) {
        float ndcX = (Graphics::width > 0.f) ? screenPosition.x / (Graphics::width * 0.5f) : 0.f;
        float ndcY = (Graphics::height > 0.f) ? screenPosition.y / (Graphics::height * 0.5f) : 0.f;

        XMMATRIX viewProj = mainCamera3D.GetViewMatrix() * mainCamera3D.GetProjectionMatrix(GetAspectRatio());
        XMMATRIX inverse = XMMatrixInverse(nullptr, viewProj);
        XMVECTOR nearPoint = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.f, 1.f), inverse);
        XMVECTOR farPoint = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.f, 1.f), inverse);

        XMStoreFloat3(&origin, nearPoint);
        XMStoreFloat3(&direction, XMVector3Normalize(farPoint - nearPoint));
    }

    // 画像1枚ぶんの「みんなで共有するデータ」。コマ番号・色など1体ごとの状態は持たない
    struct TextureData {
        ID3D11ShaderResourceView* srv = nullptr;
        int width = 0, height = 0;
    };
    std::vector<TextureData> textures;

    bool Init(HWND hwnd, int width, int height) {
        if (!Graphics::Init(hwnd, width, height)) return false;
        if (!sprite.Init()) return false;
        sprite.SetCamera(&mainCamera);
        sprite.SetCamera3D(&mainCamera3D);
        return true;
    }

    unsigned int LoadTexture(const std::wstring& filepath) {
        Texture::LoadResult result = Texture::Load(filepath);
        if (!result.srv) return UINT_MAX;

        TextureData data;
        data.srv = result.srv;
        data.width = result.width;
        data.height = result.height;

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

    void Draw(Transform transform, unsigned int texID, XMFLOAT4 color, const SpriteSheet& sheet,
        bool worldSpace, bool lockX, bool lockY, bool lockZ) {
        if (texID >= textures.size()) return;
        auto& tex = textures[texID];
        if (!tex.srv) return;

        sprite.SetTexture(tex.srv);
        sprite.SetTextureSize(tex.width, tex.height);
        sprite.Draw(transform, color, sheet, worldSpace, lockX, lockY, lockZ);
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
            // 2D(Sprite)・3D(LookAtLH, up=(0,1,0))ともY+=上方向で統一されているので、
            // そのままコピーするだけでよい
            mainCamera3D.position.x = mainCamera.position.x;
            mainCamera3D.position.y = mainCamera.position.y;
            mainCamera3D.target.x = mainCamera.position.x;
            mainCamera3D.target.y = mainCamera.position.y;

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
}