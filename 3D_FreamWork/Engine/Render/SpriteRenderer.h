#pragma once
#include "../Core/Component.h"
#include "Image.h"
#include <string>

// GameObjectに「テクスチャを描画する見た目」を持たせるComponent。
// UnityのSpriteRendererに相当する。
class SpriteRenderer : public Component {
private:
    unsigned int texID = UINT_MAX;   // Resourcesから借りている画像(同じ画像の人みんなで共有)

    // ここから下は1体ごとの状態(同じ画像を使う他のオブジェクトとは連動しない)
    SpriteSheet sheet;                                  // 横・縦の分割数とコマ番号
    DirectX::XMFLOAT4 color{ 1.f, 1.f, 1.f, 1.f };      // 画像にかける色

    // UI(既定): zで大きさが変わらず、必ず最前面。UIどうしは drawPriority → 作った順(後に作った物が手前)
    // World2D   : zで大きさが変わらず(遠近なし)、2Dカメラでスクロール。zが小さいほど手前(深度バッファ)
    // World3D   : 3Dカメラの透視のビルボード。深度バッファ(position.z)で前後関係が決まる(3Dメッシュと同じ)
    SpriteMode mode = SpriteMode::UI;

    // Worldモードのビルボード回転を、軸ごとに止める(ロックする)かどうか。
    // 全てfalse(既定)なら常にカメラの方を完全に向く。trueにした軸は回転せず固定される
    bool billboardLockX = true;
    bool billboardLockY = true;
    bool billboardLockZ = true;

    bool frustumCulling = false;   // trueなら、画面の外にある時は描かない

public:
    SpriteRenderer() = default;
    // 生成と同時にテクスチャを指定したい場合はこちら
    // 例: obj->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3);
    SpriteRenderer(const std::wstring& filepath, int cols = 1, int rows = 1);

    void SetTexture(const std::wstring& filepath, int cols = 1, int rows = 1);
    void SetSpriteIndex(int index);
    void SetColor(float r, float g, float b, float a);
    DirectX::XMFLOAT4 GetColor() const;

    // 描き方を選ぶ(3種類)。例: sprite->SetMode(SpriteMode::World2D);
    //   SpriteMode::UI      : HUD・ボタン・背景など。zで大きさが変わらず、必ず最前面。UIどうしは drawPriority → 作った順(既定)
    //   SpriteMode::World2D : 2Dのゲームの物。zで大きさが変わらず(遠近なし)、2Dカメラでスクロール。zが小さいほど手前
    //   SpriteMode::World3D : 3D空間に置く木の看板など。3Dカメラの透視のビルボードで、3Dメッシュと同じ深度で前後が決まる
    void SetMode(SpriteMode value) { mode = value; }
    SpriteMode GetMode() const { return mode; }

    // 今までの書き方(互換)。true=SpriteMode::World3D、false=SpriteMode::UI
    void SetWorldSpace(bool v) { mode = v ? SpriteMode::World3D : SpriteMode::UI; }

    // trueにした軸は、ビルボードがカメラに合わせて回転しないよう固定する。
    // 例: SetBillboardLock(true, false, true) で、水平方向(Y軸)だけカメラに向く
    //     昔ながらの「立て看板」ビルボードになる(X/Zの傾きは常に固定)。
    // (3Dモードだけに効く)3軸すべてtrueにすると、ビルボード自体が無効になり常に一定の向きで表示される
    void SetBillboardLockX(bool lockX) { billboardLockX = lockX; }
    void SetBillboardLockY(bool lockY) { billboardLockY = lockY; }
    void SetBillboardLockZ(bool lockZ) { billboardLockZ = lockZ; }
    void SetBillboardLock(bool lockX, bool lockY, bool lockZ) {
        SetBillboardLockX(lockX);
        SetBillboardLockY(lockY);
        SetBillboardLockZ(lockZ);
    }

    // trueにすると、カメラに映らない(画面の外の)時は描かなくなる(既定はfalse)。物理(当たり判定)は止まらない。
    // UI・2Dモードは2Dカメラの画面、3Dモードは3Dカメラの視錐台で調べる。
    // 大きさは transform.scale の幅・高さから求める(scaleが0以下で画像の大きさを使う時は、大きさが分からないので判定しない)。
    // ※HPバーや文字のように画面に固定する物は、falseのままにする
    void SetFrustumCulling(bool enabled) { frustumCulling = enabled; }
    bool IsFrustumCulling() const { return frustumCulling; }

    void Draw() override;
    void Uninit() override;
};
