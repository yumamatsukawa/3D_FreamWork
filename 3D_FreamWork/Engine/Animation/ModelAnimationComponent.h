#pragma once
#include "../Core/Component.h"
#include "../Render3D/Model.h"
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class ModelRenderer;

// FBXモデルのパーツの動き(Mayaで付けたキーフレーム)を再生するComponent。同じオブジェクトの ModelRenderer に付ける。
// 位置・スケールは線形、回転はクォータニオンで補間して、毎フレーム ModelRenderer のパーツの動きに書き込む。
// 骨に合わせて体を曲げる動き(スキン)は、まだ対応していない
class ModelAnimationComponent : public Component {
private:
    ModelRenderer* renderer = nullptr;       // 動かす相手(Initで探す)
    std::shared_ptr<Model> model;            // クリップを持っているモデル
    int clipIndex = -1;                      // 再生中のクリップ(-1なら何も再生していない)
    float time = 0.f;                        // クリップの中の今の時刻(秒)
    float speed = 1.f;                       // 再生の速さ
    bool loop = true;
    bool finished = false;                   // ループしない動きが終わった
    std::vector<ModelPose> fadeFrom;         // CrossFade を始めた時のポーズ(パーツごと)
    float fadeTime = 0.f;                    // CrossFade が進んだ秒数
    float fadeDuration = 0.f;                // CrossFade にかける秒数(0なら切り替え中ではない)
    std::unordered_set<std::string> warnedNames;   // 警告を出した名前(毎フレーム呼ばれても1回だけ知らせる)

    bool StartClip(const std::string& clipName, bool loopPlay);
    void CapturePose(std::vector<ModelPose>& out) const;
    void SamplePose(int node, ModelPose& out) const;
    void Apply();

public:
    void Init() override;
    void Update(float dt) override;

    // クリップを最初から再生する(Mayaで付けたクリップ名)。再生中の同じクリップなら何もしない(毎フレーム呼んでも最初に戻らない)。
    // 名前が無い時は出力ウィンドウに知らせて、何もしない
    void Play(const std::string& clipName, bool loopPlay = true);
    // 今のポーズから、新しいクリップへ seconds 秒かけて滑らかに切り替える。再生中の同じクリップなら何もしない
    void CrossFade(const std::string& clipName, float seconds);
    // ループしない動きが最後まで終わったか(終わったら最後のポーズで止まる)
    bool IsFinished() const { return finished; }
    // 再生の速さ(1=等倍、0=止まる。0未満は0)
    void SetSpeed(float newSpeed) { speed = (newSpeed > 0.f) ? newSpeed : 0.f; }
    // 入っているクリップ名の一覧(確認用)
    std::vector<std::string> GetClipNames() const;
};
