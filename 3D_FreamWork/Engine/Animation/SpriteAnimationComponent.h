#pragma once
#include "../Core/Component.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// スプライトシートのコマを、時間に合わせて送るComponent(同じGameObjectのSpriteRendererのコマ番号を毎フレーム変える)。
//
// 使い方:
//   auto* anim = obj->AddComponent<SpriteAnimationComponent>();   // SpriteRendererの後に付ける
//   anim->AddClip("Walk", { 4, 5 }, 0.3f, true);                  // 名前, コマ番号の並び, 1コマの秒数, 繰り返すか
//   anim->AddClip("Idle", { 4 }, 1.0f, true);
//   if (moving) anim->Play("Walk"); else anim->Play("Idle");      // Controllerが毎フレーム呼んでよい
//
// 決まり:
// - Playに今と同じ名前を渡した時は何もしない(毎フレーム呼んでも最初のコマに戻らない)。別の名前なら最初のコマから。
//   ループしないクリップを同じ名前でもう一度最初から再生したい時は、Stop()してからPlayする
// - 登録されていない名前のPlayは何もしない(今のクリップのまま。出力ウィンドウに名前ごとに1回知らせる)
// - framesが空、またはframeDurationが0以下のクリップは登録しない(出力ウィンドウに知らせる)。
//   止め絵にしたい時は、コマを1つだけにする(例: AddClip("Idle", { 4 }, 1.0f, true))
// - 同じ名前でAddClipし直すと上書きする(再生中のクリップなら、最初のコマからやり直す)
// - ループしないクリップは、最後のコマを1コマ分の時間見せ終わったら止まり(最後のコマのまま)、IsFinished()がtrueになる
// - dtが大きくて1フレームで何コマも進む時も、進んだコマ数だけ送る(余った時間は次のコマへ持ち越す)
// - SetSpeedの負の値は0として扱う(逆再生はしない)
// - SetRandomStart(true)は、ループするクリップだけ、始まるコマと経過時間をランダムにする
//   (ループしないクリップは必ず最初のコマから。攻撃・やられなどが途中から始まらないように)
// - ObjectPoolで再利用された時(SetActive(true))は、今のクリップを最初のコマから(SetRandomStartならランダムなコマから)やり直す
// - 時間はUpdateのdtで進む(ポーズ・シーン切り替え中は止まる)。GameObjectが非アクティブの間も止まる
// - 同じGameObjectにSpriteRendererが無ければ何もしない(出力ウィンドウに1回知らせる)
class SpriteAnimationComponent : public Component {
private:
    // 1つのアニメーション
    struct Clip {
        std::vector<int> frames;      // コマ番号の並び(この順に表示)
        float frameDuration = 0.1f;   // 1コマを表示する時間(秒)
        bool loop = true;             // 繰り返すか
    };

    std::unordered_map<std::string, Clip> clips;     // 登録したアニメーション(名前で探す)
    std::string currentName;                         // 再生中のクリップの名前(止まっていれば空)
    const Clip* current = nullptr;                   // 再生中のクリップ(止まっていればnullptr)
    int frameIndex = 0;                              // 今表示しているコマ(framesの何番目か)
    float elapsed = 0.0f;                            // 今のコマを表示してからの時間(秒)
    bool finished = false;                           // ループしないクリップが最後まで行ったか
    float speed = 1.0f;                              // 再生の速さ(1=通常、0=一時停止)
    bool randomStart = false;                        // 始まるコマをランダムにするか
    bool warnedNoSprite = false;                     // 「SpriteRendererがありません」を出力したか
    std::unordered_set<std::string> warnedNames;     // 「登録されていません」を出力した名前

    void Restart();      // 今のクリップを最初のコマから(SetRandomStartならランダムなコマから)
    void ApplyFrame();   // 今のコマをSpriteRendererに渡す

public:
    void AddClip(const std::string& name, std::vector<int> frames,
        float frameDuration, bool loop);   // アニメーションを登録(名前, コマ番号の並び, 1コマの秒数, 繰り返すか)
    void Play(const std::string& name);    // 再生(今と同じ名前なら何もしない)
    void Stop();                           // 止める(コマは今のまま。次のPlayは同じ名前でも最初から)
    bool IsFinished() const;               // ループしないクリップが最後まで行って止まったか(ループするクリップは常にfalse)
    void SetSpeed(float newSpeed);         // 再生の速さ(1=通常、2=倍速、0=一時停止。負の値は0)
    void SetRandomStart(bool enabled);     // 始まるコマをランダムにする(敵の足踏みがそろわないように)

    const std::string& GetCurrentClip() const { return currentName; }   // 再生中のクリップの名前(止まっていれば空)
    float GetSpeed() const { return speed; }

    void Update(float dt) override;
    void OnActiveChanged(bool active) override;
};
