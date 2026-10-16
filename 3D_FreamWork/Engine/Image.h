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
    // オブジェクトを追いかけさせる時は CameraFollowComponent、揺らす時は CameraShake を使う
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

    // 3D描画の画面の縦横比(幅÷高さ)。3D用カメラの投影行列(GetProjectionMatrix)に渡す値。Visibility(視界判定)が使う
    float GetAspectRatio();

    // ─── 座標の変換 ─────────────────────────────
    // 画面座標は「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」(Text::Drawと同じ)。結果をそのままText::Drawに渡せる。
    // ※Input::GetMousePosition()はY+=下なので、ここに渡す時はYの符号を反転する(Input::GetMouseWorldPosition()は反転済み)
    // ※Drawの中で呼ぶと、CameraShakeの揺れが入ったカメラで計算する(描いた絵と一緒に揺れる)。Updateの中では揺れは入らない

    // 2D用カメラ(GetCamera())で、ワールド座標→画面座標。SpriteRenderer(UIモード)と同じ計算
    // (奥(Z+)にあるほど小さく、画面の中心に寄って見える。カメラの focalLength の距離=Z=0 なら等倍)
    // 例: XMFLOAT2 p = Image::WorldToScreen2D(enemy->transform.position); Text::Draw(L"!", p.x, p.y + 60.f, 30.f);
    XMFLOAT2 WorldToScreen2D(XMFLOAT3 worldPosition);
    // 2D用カメラで、画面座標→ワールド座標(Z=0の平面の上の点。zは0)。WorldToScreen2Dの逆
    XMFLOAT3 ScreenToWorld2D(XMFLOAT2 screenPosition);

    // 3D用カメラ(GetCamera3D())で、ワールド座標→画面座標。MeshRendererと同じ透視投影(同じview/proj・アスペクト比)
    // 戻り値の x,y: 画面座標。z: カメラからの奥行き(カメラの向きに沿った距離)。
    // ★ z が0以下の時は、点がカメラの後ろにある(画面には映らない)。この時の x,y は使えない(0を返す)ので、
    //   使う前に必ず z > 0 を確かめる。画面の外(左右・上下)にある点は、画面の端より外の座標がそのまま返る
    // 例: XMFLOAT3 p = Image::WorldToScreen3D(enemy->transform.position);
    //     if (p.z > 0.f) Text::Draw(L"HP", p.x, p.y + 40.f, 24.f);
    XMFLOAT3 WorldToScreen3D(XMFLOAT3 worldPosition);
    // 3D用カメラで、画面の点から奥へ伸びる線(origin=近くの描画限界(nearZ)の面の上の点、direction=長さ1の向き)。
    // Physics::Raycast(origin, direction, ...) と組み合わせると、画面のその点に映っている物を探せる
    void ScreenPointToRay3D(XMFLOAT2 screenPosition, XMFLOAT3& origin, XMFLOAT3& direction);

    // ─── 画面に四角を塗る ─────────────────────────────
    // 画面に四角を1つ塗る(HPバーの帯など)。Text::Draw と同じ Direct2D で描くので、カメラの影響を受けず(スクロールしない)、
    // 画像や3Dの描画が全部終わった後に重なる。Text::Draw とは「呼んだ順」に重なる(帯を描いてから数字を描けば、帯の上に数字が出る)
    // position: 四角の「中心」の画面座標(画面中央が(0,0)、Y+=上。Text::Drawと同じ。GetAnchoredPosition(Anchor.h)の結果をそのまま渡せる)
    // size: 幅と高さ(ピクセル)。color: 色(wは濃さ。0なら描かない)
    // Component の Draw の中(GameEngine が Text::BeginDraw 〜 EndDraw で囲んでいる間)で呼ぶ
    // 例: Image::DrawRect({ 0.f, 0.f }, { 200.f, 20.f }, { 1.f, 0.f, 0.f, 1.f });   // 画面の中央に赤い帯
    void DrawRect(XMFLOAT2 position, XMFLOAT2 size, XMFLOAT4 color);

    // ─── Engine内部用(GameEngineだけが呼ぶ。ゲーム側からは呼ばない) ───
    // DrawRectが使い回しているブラシを解放する(Text::Uninitより先に。ブラシはTextの描画先から作っているため。何回呼んでもよい)
    void ReleaseDrawRectBrush();

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
    // blend: 画像の重ね方(Sprite.hのBlendMode。パーティクル(ParticleRenderer)が使う。普通はNormalのまま)
    // depthWrite: worldSpace時に深度を書くか(半透明の粒はfalse)
    void Draw(Transform transform, unsigned int texID,
        XMFLOAT4 color = XMFLOAT4(1.f, 1.f, 1.f, 1.f), const SpriteSheet& sheet = SpriteSheet(),
        bool worldSpace = false, bool lockX = false, bool lockY = false, bool lockZ = false,
        BlendMode blend = BlendMode::Normal, bool depthWrite = true);

    // Mesh(3D)など、Sprite以外の描画にテクスチャを使い回したい時に、生のSRVを取り出す
    ID3D11ShaderResourceView* GetTextureView(unsigned int texID);
}