#pragma once
#include "Graphics.h"
#include "Sprite.h"
#include "Texture.h"
#include "GameObject.h"
#include "Camera.h"
#include "Light.h"
#include <vector>
#include <string>

namespace Image
{
    bool Init(HWND hwnd, int width, int height);
    void BeginFrame();
    void EndFrame();
    void Uninit();

    // 2Dスプライト用のカメラ。position(x,y)を動かすと画面がスクロールする
    // 例: Image::GetCamera().position = player->transform.position;
    Camera& GetCamera();

    // 3Dメッシュ(MeshRenderer)用のカメラ。2D用のGetCamera()とはあえて別インスタンス。
    // 2Dカメラはプレイヤー追従などで毎フレーム動くことが多く、同じカメラを3D側でも
    // 使うと「3Dオブジェクトがカメラに合わせて動いて見える」おかしな挙動になるため
    Camera& GetCamera3D();

    // 3Dメッシュのライティングに使う、シーン共通の平行光源
    Light& GetLight();

    // 既定ではImage::BeginFrame()が毎フレーム3Dカメラを2Dカメラに自動で追従させている。
    // CameraController(一人称/三人称カメラ)のように3Dカメラを自分で操作したい場合は、
    // false にしてこの自動追従を止める(使い終わったら忘れずtrueに戻すこと)
    void SetCamera3DAutoSync(bool enabled);

    // ─── Engine内部用(Resourcesだけが呼ぶ。ゲーム側は Resources::LoadTexture / ReleaseTexture を使う) ───
    // ここで読むと、同じ画像でも呼ぶ度に読み直してしまう(共有されない)
    unsigned int LoadTexture(const std::wstring& filepath);  // 画像を読む(RGBA形式のpng)。失敗したらUINT_MAX
    void ReleaseTexture(unsigned int id);                     // 画像を解放する
    void ReleaseAllTextures();                                // すべての画像を解放する(Image::Uninitが呼ぶ)

    // 画像を1枚描く。画像(texID)はみんなで共有し、コマ・色は描く側(SpriteRendererなど)が1体ごとに渡す
    // color: 画像にかける色(白=そのまま)
    // sheet: 横・縦の分割数(cols/rows)と、何コマ目を描くか(index)
    // worldSpace = false(既定) : UIのように常に手前に描画される
    // worldSpace = true        : 3Dオブジェクトのように、奥行きで前後関係が決まる
    // lockX/lockY/lockZ: worldSpace時のビルボード回転を軸ごとに止める(SpriteRenderer::SetBillboardLock参照)
    void Draw(Transform transform, unsigned int texID,
        XMFLOAT4 color = XMFLOAT4(1.f, 1.f, 1.f, 1.f), const SpriteSheet& sheet = SpriteSheet(),
        bool worldSpace = false, bool lockX = false, bool lockY = false, bool lockZ = false);

    // Mesh(3D)など、Sprite以外の描画にテクスチャを使い回したい時に、生のSRVを取り出す
    ID3D11ShaderResourceView* GetTextureView(unsigned int texID);
}