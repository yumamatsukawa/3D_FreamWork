#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>
#include <vector>

// 協力パズルの最小形: スイッチ2つを両方押すと扉が開く。ルール(スイッチの数え・扉を開ける)はホストだけが決める。
// - スイッチを押す → 「SwitchPressed」をホストへ送る(出来事)
// - ホストが「今の状態」を共有の値(SwitchA / SwitchB / Door)に入れる → 途中参加の人にも渡る
// - 扉が開いた瞬間は「DoorOpened」の出来事を全員へ(演出用。EventBusでも受けられる)
class NetworkDoorController : public Component {
private:
    float openAmount = 0.f;                 // 扉の開き具合(0=閉、1=開。見た目だけの値で、共有の値に追いつくように動く)
    std::vector<std::wstring> log;          // 受け取った出来事の記録(新しい順)

    void AddLog(const std::wstring& text);

public:
    void PressSwitch(int index);            // スイッチ index(0=A、1=B)を押す(ボタンから呼ぶ)
    void ResetDoor();                       // 扉とスイッチを元に戻す(ボタンから呼ぶ)

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};