#include "Image.h"
#include "../UI/Text.h"
#include <cmath>
#include <algorithm>
#include <functional>
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

    // 3D描画のアスペクト比(Mesh::Init・Sprite::BuildBillboardMatrixと同じ)
    float GetAspectRatio() {
        return (Graphics::height > 0.f) ? (Graphics::width / Graphics::height) : 1.f;
    }

    // ワールド→画面: ワールド - カメラ位置(zは大きさに影響しない)
    XMFLOAT2 WorldToScreen2D(XMFLOAT3 worldPosition) {
        return {
            worldPosition.x - mainCamera.position.x,
            worldPosition.y - mainCamera.position.y
        };
    }

    // 画面→ワールド: WorldToScreen2Dの逆(カメラ位置 + 画面座標。zは0)
    XMFLOAT3 ScreenToWorld2D(XMFLOAT2 screenPosition) {
        return {
            mainCamera.position.x + screenPosition.x,
            mainCamera.position.y + screenPosition.y,
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

    namespace {
        // DrawRect用のブラシ(毎回作らず、最初に要った時に作って使い回す。ReleaseDrawRectBrushで解放)
        ID2D1RenderTarget* rectBrushTarget = nullptr;   // ブラシを作った描画先(違う描画先では使えないため覚えておく)
        ID2D1SolidColorBrush* rectBrush = nullptr;      // 色は描く時に変える
    }

    void ReleaseDrawRectBrush() {
        if (rectBrush) { rectBrush->Release(); rectBrush = nullptr; }
        rectBrushTarget = nullptr;
    }

    namespace {
        // オーバーレイキュー: Scene::Drawの間、Direct2Dの描画(文字・四角)を積んでおいて、UIスプライトの後に描く
        // (Direct2Dは命令がたまると途中で画面に出すので、その場で描くと、後から描くUIスプライトに上書きされてしまう)
        std::vector<std::function<void()>> overlayQueue;
        bool overlayQueueActive = false;
    }

    bool DeferOverlay(std::function<void()> command) {
        if (!overlayQueueActive) return false;
        overlayQueue.push_back(std::move(command));
        return true;
    }

    void FlushOverlayQueue() {
        overlayQueueActive = false;
        // 呼んだ順に描く(四角を描いてから文字を描けば、四角の上に文字が出る)。実行中に積まれることは無い(activeはfalse)
        for (std::function<void()>& command : overlayQueue) command();
        overlayQueue.clear();   // capacityは残す
    }

    namespace {
    // 画面に四角を塗る: ゲーム座標(中央が原点、Y+=上)をDirect2Dの座標(左上が原点、Y+=下)に直して、Text::Drawと同じ描画先に塗る
    void DrawRectNow(XMFLOAT2 position, XMFLOAT2 size, XMFLOAT4 color) {
        ID2D1RenderTarget* target = Text::GetRenderTarget();
        if (!target) return;
        if (!(color.w > 0.f)) return;                       // 透明なら描かない
        if (!(size.x > 0.f) || !(size.y > 0.f)) return;     // 大きさが0以下(やNaN)なら描かない
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(size.x) || !std::isfinite(size.y)) return;

        // 描画先が変わっていたら、前の描画先で作ったブラシは使えないので作り直す
        if (target != rectBrushTarget) {
            ReleaseDrawRectBrush();
            rectBrushTarget = target;
        }
        if (!rectBrush) {
            target->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 0.f), &rectBrush);
            if (!rectBrush) return;
        }

        float centerX = position.x + Graphics::width * 0.5f;
        float centerY = -position.y + Graphics::height * 0.5f;
        rectBrush->SetColor(D2D1::ColorF(color.x, color.y, color.z, color.w));
        target->FillRectangle(
            D2D1::RectF(centerX - size.x * 0.5f, centerY - size.y * 0.5f, centerX + size.x * 0.5f, centerY + size.y * 0.5f),
            rectBrush);
    }

    }   // namespace

    void DrawRect(XMFLOAT2 position, XMFLOAT2 size, XMFLOAT4 color) {
        // Scene::Drawの間は積んでおく(値をコピーして持つ)。それ以外はその場で描く
        if (DeferOverlay([position, size, color] { DrawRectNow(position, size, color); })) return;
        DrawRectNow(position, size, color);
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

    namespace {
        // UIモードの描画待ち1枚ぶん(Scene::Drawの間、UIは積んでおいて、最後にまとめて描く)
        struct UiDraw {
            Transform transform;
            unsigned int texID;
            XMFLOAT4 color;
            SpriteSheet sheet;
            BlendMode blend;
            int priority;   // 描く順その1: drawPriority
            int order;      // 描く順その2: オブジェクトを作った順
        };
        std::vector<UiDraw> uiQueue;
        bool uiQueueActive = false;   // Scene::Drawの間だけtrue(それ以外のUIモードの描画は、その場で描く)
        int currentPriority = 0;      // 今描いているオブジェクトのdrawPriority
        int currentOrder = 0;         // 今描いているオブジェクトを作った順

        // 画像を1枚その場で描く
        void DrawNow(const Transform& transform, unsigned int texID, XMFLOAT4 color, const SpriteSheet& sheet,
            SpriteMode mode, bool lockX, bool lockY, bool lockZ, BlendMode blend, bool depthWrite) {
            if (texID >= textures.size()) return;
            auto& tex = textures[texID];
            if (!tex.srv) return;

            sprite.SetTexture(tex.srv);
            sprite.SetTextureSize(tex.width, tex.height);
            sprite.Draw(transform, color, sheet, mode, lockX, lockY, lockZ, blend, depthWrite);
        }
    }

    void Draw(Transform transform, unsigned int texID, XMFLOAT4 color, const SpriteSheet& sheet,
        bool worldSpace, bool lockX, bool lockY, bool lockZ, BlendMode blend, bool depthWrite) {
        Draw(transform, texID, color, sheet, worldSpace ? SpriteMode::World3D : SpriteMode::UI, lockX, lockY, lockZ, blend, depthWrite);
    }

    void Draw(Transform transform, unsigned int texID, XMFLOAT4 color, const SpriteSheet& sheet,
        SpriteMode mode, bool lockX, bool lockY, bool lockZ, BlendMode blend, bool depthWrite) {
        // UIモードは積んでおく(FlushUiQueueで、World側が全部描かれた後に、必ず最前面に描く)
        if (mode == SpriteMode::UI && uiQueueActive) {
            if (texID >= textures.size() || !textures[texID].srv) return;
            uiQueue.push_back({ transform, texID, color, sheet, blend, currentPriority, currentOrder });
            return;
        }
        DrawNow(transform, texID, color, sheet, mode, lockX, lockY, lockZ, blend, depthWrite);
    }

    void BeginUiQueue() {
        uiQueue.clear();
        uiQueueActive = true;
        overlayQueue.clear();
        overlayQueueActive = true;
        currentPriority = 0;
        currentOrder = 0;
    }

    void SetDrawOrder(int priority, int order) {
        currentPriority = priority;
        currentOrder = order;
    }

    void FlushUiQueue() {
        uiQueueActive = false;

        // drawPriority → 作った順。同じオブジェクトの中は積んだ順のまま(stable_sort)。z座標は並びに使わない
        std::stable_sort(uiQueue.begin(), uiQueue.end(), [](const UiDraw& a, const UiDraw& b) {
            if (a.priority != b.priority) return a.priority < b.priority;
            return a.order < b.order;
        });
        for (const UiDraw& d : uiQueue)
            DrawNow(d.transform, d.texID, d.color, d.sheet, SpriteMode::UI, false, false, false, d.blend, true);
        uiQueue.clear();
    }

    ID3D11ShaderResourceView* GetTextureView(unsigned int texID) {
        if (texID >= textures.size()) return nullptr;
        return textures[texID].srv;
    }

    void BeginFrame() {
        uiQueue.clear();   // 前のフレームの描き残しが無いように(普通は空)
        uiQueueActive = false;
        overlayQueue.clear();
        overlayQueueActive = false;

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
        overlayQueue.clear();
        ReleaseDrawRectBrush();   // 普通はGameEngine::Uninitで解放済み(何回呼んでもよい)
        ReleaseAllTextures();
        sprite.Uninit();
        Graphics::Uninit();
    }
}