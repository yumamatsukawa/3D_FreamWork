#pragma once
#include "../../../../Engine/Core/Component.h"
#include <vector>

class GameObject;

// ロボットの大群の管理役(負荷テスト)。ロボットを体数に合わせて作る・消し、物理あり/なしの切り替え(全員を作り直す)と、
// 影・枠線・LOD・画面外を省く設定のオン/オフを行う。FPS・1フレームの時間・描画の数(RenderStats)を画面に出して、どれくらい重いかを見せる。
// 全体の設定(Shadow・Lod)を変えるので、出る時(Uninit)に既定へ戻す
class RobotCrowdManager : public Component {
private:
    static constexpr int startCount = 100;           // 最初の体数
    static constexpr int countStep = 50;             // 1回のキーで増減する体数
    static constexpr int maxCount = 1000;            // 体数の上限
    static constexpr float fpsInterval = 0.5f;       // FPSを平均する間隔(秒)

    std::vector<GameObject*> robots;                 // 今いるロボット(体数)

    bool physicsOn = false;                          // 物理あり(剛体を付ける)か(Pキー。切り替えると全員を作り直す)
    bool shadowOn = false;                           // ロボットが影を落とすか(1キー)
    bool outlineOn = false;                          // ロボットに枠線を付けるか(2キー)
    bool lodOn = false;                              // 遠いロボットを省く設定(LOD)を使うか(3キー)
    bool cullingOn = false;                          // 画面の外のロボットを描かないか(4キー)

    float fpsTimer = 0.f;                            // FPSを数え始めてからの時間(秒)
    int fpsFrames = 0;                               // その間のフレーム数
    float worstFrame = 0.f;                          // その間でいちばん長かった1フレーム(秒)
    float fps = 0.f;                                 // 表示するFPS(0.5秒ごとに更新)
    float frameMs = 0.f;                             // 表示する1フレームの時間(ミリ秒。同じく平均)
    float worstMs = 0.f;                             // 表示する、いちばん長かった1フレーム(ミリ秒)

    void SpawnRobot();                               // ロボットを1体、ほかのロボットから離れた場所に出す
    void SetCount(int count);                        // 体数をcountにする(足りなければ出し、多ければ消す)
    void Rebuild();                                  // 今の体数のまま、全員を消して作り直す(物理の切り替え用)
    void ApplyLook(GameObject* robot) const;         // 影・枠線・画面外を省く設定を、ロボット1体に入れる
    void ApplySettings();                            // 影・LODの全体の設定と、全員の見た目を更新する
    void MeasureFrame();                             // FPSと1フレームの時間を測る

public:
    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
