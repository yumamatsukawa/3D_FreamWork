#pragma once
#include "../../../../Engine/Core/Component.h"
#include <climits>
#include <memory>
#include <string>
#include <vector>

class GameObject;
class Model;

// Resourcesデモの操作役。同じ画像・音・モデルを何回借りても読み込みは1回だけ(IDやポインタが同じ)で、
// 使う人がいなくなっても UnloadUnused を呼ぶまでは残ることを見せる。借りた音・モデルは、Uninitで必ず返す
class ResourcesDemoController : public Component {
private:
    static constexpr int maxSprites = 72;                     // 同時に置ける画像の数
    static constexpr int columns = 12;                        // 画像の並べ方(1行の数)

    std::vector<GameObject*> sprites;                         // 置いた画像(box.png)
    std::vector<unsigned int> soundIds;                       // 借りている音のID
    std::vector<std::shared_ptr<Model>> models;               // 借りているモデル
    unsigned int textureId = UINT_MAX;                      // 最後に調べた box.png のID(調べていない時は UINT_MAX)
    std::wstring message1 = L"ボタンを押してみてください";    // 最後の操作の結果(1行目)
    std::wstring message2;                                    // 最後の操作の結果(2行目)

    void CheckTextureId();                                    // box.png のIDを調べる(1回借りて、すぐ返す)
    void SetMessage(const wchar_t* line1, const wchar_t* line2) { message1 = line1; message2 = line2; }   // 結果の文字を決める

public:
    void AddSprites(int count);        // 同じ画像のスプライトを増やす
    void DestroySprites();             // 置いた画像をすべてDestroyする
    void UnloadUnused();               // 使われていない画像・音・モデルを解放する
    void LoadSounds();                 // 同じ音を2回借りる
    void LoadModels();                 // 同じモデルを2回借りる
    void ReleaseHeld();                // 借りている音・モデルを返す

    void Draw() override;
    void Uninit() override;
};
