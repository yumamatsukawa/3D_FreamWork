#pragma once
#include "../Core/Component.h"
#include <DirectXMath.h>
#include <climits>
#include <string>
#include <vector>

// パーティクルの設定(爆発・煙・火花などの見た目と動き)。長さの単位は、オブジェクトのposition/scaleと同じ
struct ParticleSettings {
    std::wstring texture;              // 粒の画像(空なら Assets/particle.png=柔らかい白い円)。作り方は ParticleRenderer のコメント参照
    float emitRate = 10.f;             // 1秒に出す数(Playしている間だけ出る。Burstには関係ない)
    float lifetimeMin = 0.5f, lifetimeMax = 1.0f;  // 粒が消えるまでの時間(秒。この間でランダム)
    float speedMin = 50.f, speedMax = 100.f;       // 粒の速さ(1秒に動く距離。この間でランダム)
    DirectX::XMFLOAT3 direction = { 0.f, 1.f, 0.f };   // 出る向き
    float spreadAngle = 30.f;          // 向きのばらつき(度。向きを中心に、全体でこの角度の範囲。360なら全方向)
    DirectX::XMFLOAT3 gravity = { 0.f, 0.f, 0.f };     // 重力(1秒に速度が変わる量。粒が落ちていく)
    float startSize = 16.f, endSize = 0.f;         // 出た時と消える時の大きさ(寿命に沿って変わる。0以下になった粒は描かない)
    DirectX::XMFLOAT4 startColor = { 1.f, 1.f, 1.f, 1.f };  // 出た時の色(wは濃さ)
    DirectX::XMFLOAT4 endColor = { 1.f, 1.f, 1.f, 0.f };    // 消える時の色(寿命に沿って変わる)
    bool additive = false;             // 加算合成(重なるほど明るくなる。炎・光向け)。falseは、ふわっと重なる普通の合成(煙向け)
    bool worldSpace = true;            // true=出た場所に残る(煙)、false=オブジェクトと一緒に動く(オーラ)
    bool use3D = false;                // false=2D画面(Sample2D)、true=3D空間(Sample3D。粒は常にカメラの方を向く板として描かれ、手前の物に隠れる)
};

// 小さい粒をたくさん出して、まとめて描くComponent(爆発・煙・火花など)。UnityのParticleSystemに相当する。
//
// 使い方:
//   ParticleSettings settings;
//   settings.spreadAngle = 360.f;
//   settings.additive = true;
//   auto* particles = obj->AddComponent<ParticleRenderer>(settings);
//   particles->Burst(30);    // 一度に30個出す(爆発)
//   particles->Play();       // または、emitRateの数ずつ出し続ける(Stopするまで)
// 使い捨てのエフェクトは、Effect.h の CreateEffect が便利(粒が消えたらオブジェクトも自動で消える)。
//
// 決まり:
// - 粒1つ1つはGameObjectではなく、このComponentの中のデータ。最大 maxParticles 個まで(超える分は出さない)
// - 作った直後は出していない(Play() かBurst() で出し始める)
// - 粒の動きはシーンの時間(Time::GetDeltaTime。ポーズ中・スローは粒も止まる/遅くなる)
// - 粒の位置は、2D(use3D=false)はposition.x,y(zは大きさにも前後にも使わない)、3Dはx,y,zで動く。向きのばらつき(spreadAngle)は、
//   2DはXY平面の回転、3Dは円錐の範囲(360なら全方向の球)
// - 粒の画像は、明るさ(RGB)で形と濃さを表す白黒の画像にする(中心が白、外側が黒、アルファは不透明。
//   Shader.hlsl が「アルファ0.5未満は描かない」ため、アルファで柔らかくできない)。Assets/particle.png がその例
// - 描画順は、このオブジェクトの drawPriority に従う(CreateEffect は手前に描かれるよう大きい値にする)。
//   2D(use3D=false)の粒はUIモードのスプライトと同じ扱いで、必ず最前面(drawPriority → 作った順)。3Dの粒は3Dモード(SpriteMode::World3D)で、深度で隠れる。2Dの粒を2Dモードにする設定は、今は無い
class ParticleRenderer : public Component {
public:
    static constexpr int maxParticles = 2000;   // 1つのParticleRendererが持てる粒の最大数

private:
    // 粒1つぶんのデータ
    struct Particle {
        DirectX::XMFLOAT3 position{};   // 位置(worldSpace=falseの時は、オブジェクトからの相対)
        DirectX::XMFLOAT3 velocity{};   // 速さ(1秒に動く量)
        float age = 0.f;                // 出てからの時間
        float lifetime = 1.f;           // 消えるまでの時間
    };

    ParticleSettings settings;
    std::vector<Particle> particles;   // 今出ている粒
    unsigned int texID = UINT_MAX;     // Resourcesから借りている画像
    bool playing = false;              // 出し続けているか
    float emitTimer = 0.f;             // emitRateで出す数の端数(0〜1)
    bool destroyWhenFinished = false;  // 粒がすべて消えたらオブジェクトをDestroyするか

    void Emit();   // 粒を1つ出す(上限なら出さない)

public:
    explicit ParticleRenderer(const ParticleSettings& settings);

    void Play();               // 出し始める(emitRateの数ずつ。Stopするまで出し続ける)
    void Stop();               // 新しく出すのをやめる(出ている粒は寿命まで残る)
    void Burst(int count);     // 一度にcount個出す(爆発。Play中かどうかに関係なく出る)
    bool IsPlaying() const;    // 出している、または粒が残っているか(Burstした粒が残っている間もtrue)
    int GetParticleCount() const { return (int)particles.size(); }   // 今出ている粒の数

    // trueにすると、IsPlaying()がfalseになった(粒がすべて消えた)時にオブジェクトを自動でDestroyする(CreateEffectが使う)
    void SetDestroyWhenFinished(bool v) { destroyWhenFinished = v; }

    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
