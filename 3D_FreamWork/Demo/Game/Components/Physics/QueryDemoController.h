#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Physics/Physics.h"
#include <DirectXMath.h>
#include <string>
#include <vector>

class GameObject;

// Queryデモの操作役。物理への問い合わせ(Physics::Raycast / OverlapSphere / OverlapBox)を、毎フレーム行って、結果を目に見える形で出す。
// マウスの場所を使う: Raycast は出発点からマウスへ向けて線を飛ばして、最初に当たった物と場所を出す。
// OverlapSphere / OverlapBox はマウスの場所を中心にした範囲に入っている物を探して、見つかった物を光らせる(2Dは黄色い枠、3Dは黄色い枠線)。
// 4 でタグ(Target / Decoy)による絞り込み、5 でトリガーも探すか(QueryTriggers)を切り替える。
// 調べる物は全部動かない(Static)。2D版はマウスのワールドの位置、3D版はマウスの下の「線の高さ」の水平な面の位置を使う
class QueryDemoController : public Component {
public:
    // 問い合わせの種類
    enum class Query { Raycast, OverlapSphere, OverlapBox };

private:
    DemoMode mode;                                  // 2D版か3D版か
    Query query = Query::Raycast;                   // 今の問い合わせ(1・2・3キー)
    int tagIndex = 0;                               // タグでの絞り込み(4キー): 0=指定なし、1=Target、2=Decoy
    bool includeTriggers = false;                   // トリガーも探すか(5キー。QueryTriggers::Include)
    std::vector<GameObject*> targets;               // 調べる物(光らせる物)
    DirectX::XMFLOAT3 origin{};                     // Raycast の出発点
    float queryHeight = 0.f;                        // 3D版: 線・範囲の高さ(マウスの下のこの高さの面を使う)

    // 今の結果(Update で調べて、Draw で出す)
    bool hasPoint = false;                          // マウスが指す点があるか
    DirectX::XMFLOAT3 point{};                      // マウスが指す点
    bool rayHit = false;                            // Raycast が当たったか
    RaycastHit hit;                                 // 当たった物の情報
    DirectX::XMFLOAT3 rayEnd{};                     // 線の先(当たらなければ届く限界)
    std::vector<GameObject*> found;                 // Overlap で見つかった物

    void RunQuery();                                // 今の問い合わせを調べる
    void UpdateHighlight();                         // 3D版: 見つかった物の枠線を付け外しする
    bool IsHighlighted(const GameObject* object) const;   // 見つかった(当たった)物か
    void DrawFrames() const;                        // 2D版: 見つかった(当たった)物の周りに枠を描く
    void DrawQueryShape() const;                    // 線・範囲を描く

public:
    explicit QueryDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 調べる物・出発点・高さを、CreateQueryDemo()から渡してもらう(Init の後)
    void AddTarget(GameObject* object) { targets.push_back(object); }
    void SetOrigin(DirectX::XMFLOAT3 position) { origin = position; }
    void SetQueryHeight(float height) { queryHeight = height; }

    void Update(float dt) override;
    void Draw() override;
};
