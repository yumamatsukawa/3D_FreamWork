#pragma once

class LightComponent;

// 点光源・スポットライト(LightComponent)の一覧を集めて、シェーダーへ送る仕組み。
// ゲーム側は LightComponent を付けるだけでよく、このnamespaceは直接呼ばない。
//
// 決まり:
// - 同時に使える光は maxLights 個。超えた時は、3Dカメラに近い順に maxLights 個だけを使う(1フレームに1回だけ選ぶ)
// - アクティブ(SetActive(true))で有効(SetEnabled(true))な光だけが数えられる
// - 光は定数バッファのb4に入る(Shader3DCommon.hlsli の LightBuffer)。1つも無い時は、見た目は光が無かった頃と同じ
namespace Lighting {
    constexpr int maxLights = 8;   // 同時に使える光の数(Shader3DCommon.hlsli の MAX_LIGHTS と同じ)

    // シェーダー(Shader3DCommon.hlsli の GpuLight)に送る1つ分の光。float4を4本(64バイト)
    struct GpuLight {
        float positionRange[4];    // xyz=位置, w=届く距離
        float colorType[4];        // rgb=色×強さ, w=種類(0=無し, 1=点, 2=スポット)
        float directionOuter[4];   // xyz=向き(スポットの中心へ進む向き), w=cos(外側の角度)
        float innerPad[4];         // x=cos(内側の角度), y=影を落とす光(1=この光の影を使う。Shadowが描いた1つだけ)。zwは予約
    };

    // シェーダー(Shader3DCommon.hlsli の LightBuffer, b4)に送る形。16バイトずつ区切って並べる
    struct LightConstantBuffer {
        int counts[4];                 // x=光の数
        GpuLight lights[maxLights];
    };
    static_assert(sizeof(GpuLight) == 64, "GpuLightはfloat4を4本");
    static_assert(sizeof(LightConstantBuffer) % 16 == 0, "定数バッファは16バイト区切り");

    // 光の一覧から、送る内容を作る(アクティブで有効な光だけを数え、cameraPositionに近い順に maxLights 個を選ぶ)。
    // 超えた分を捨てた時は overflowed に true を入れる。Update が使う(確認用に公開)
    // shadowLight には、影を落とすスポットライト(使われる光の中でカメラに一番近い、castShadow の1つ)を入れる(無ければnullptr)。
    // その光の innerPad[1] を1にする(シェーダーが、その光の影を引く)
    LightConstantBuffer Build(const LightComponent* const* lights, int lightCount,
        float cameraX, float cameraY, float cameraZ, bool* overflowed = nullptr, const LightComponent** shadowLight = nullptr);

    // 登録・解除(LightComponent の Init / Uninit が呼ぶ)
    void Register(LightComponent* light);
    void Unregister(LightComponent* light);

    // ─── Engine内部用(GameEngine と Mesh / SkinMesh だけが呼ぶ) ───
    void Update();     // 毎フレーム1回(描画の前)。光を集め、カメラに近い順に maxLights 個を選んで送る内容を作る
    void Bind();       // Mesh / SkinMesh が描く時に、光の定数バッファ(b4)をピクセルシェーダーにセットする
    void Shutdown();   // 終了時に、バッファと登録を片付ける

    int GetActiveCount();   // 今フレームに使っている光の数(確認用)

    // 今フレームの、影を落とすスポットライト(無ければnullptr)。Shadow::Render が影のパスで使う
    const LightComponent* GetShadowSpotLight();
}
