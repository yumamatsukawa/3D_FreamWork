#pragma once
#include "Text.h"
#include <d3d11.h>
#include <string>
#include <cstddef>
#include <DirectXMath.h>

// 文字を描いたテクスチャを作って、同じ絵のテクスチャをみんなで共有する道具(WorldTextRenderer が使う Engine 内部用)。
//
// 決まり:
// - 絵を決める設定(文字・フォント・大きさ・太字・色・縁取り・寄せ方・背景・テクスチャの幅と高さ・余白)がすべて同じなら、同じテクスチャを共有する。
//   1つでも違えば別のテクスチャ。板のワールドでの大きさ・向き・光などは絵に関係しないので、ここには入らない
// - 共有テクスチャは作ったあと書き換えない。文字などを変える時は、今の参照を手放して、新しい設定のテクスチャを借り直す
// - 借りた数(refCount)が0になったら、すぐ解放する(毎フレーム文字が変わっても、古いテクスチャが残り続けない)
// - 文字の色の濃さ(color.w)は絵に入れない(板の色の濃さで表す)。縁取りの色の濃さ・背景の色の濃さは絵に入る
// - GameEngine::Uninit が ReleaseAll を呼ぶ(Direct3D のデバイスを解放するより前)
namespace TextTexture {
    // 絵を決める設定
    struct Desc {
        std::wstring text;
        TextStyle style;                                   // size はテクスチャ上のピクセル。color.w は無視
        DirectX::XMFLOAT4 background{ 0.f, 0.f, 0.f, 0.f };  // 背景の色(a=0なら背景なし)
        int width = 0;                                     // テクスチャの幅(ピクセル)
        int height = 0;                                    // テクスチャの高さ(ピクセル)
    };

    // 共有されるテクスチャ1枚
    struct Shared {
        ID3D11ShaderResourceView* srv = nullptr;
        int width = 0;
        int height = 0;
        size_t bytes = 0;      // おおよそのメモリ(ミップマップ込み)
        int refCount = 0;      // 借りている数
        Desc desc;             // 解放する時に共有表から探すための控え
    };

    // 文字の大きさ(ピクセル)を測る。余白(margin)は縁取りの太さと padding から決まる。測れなかったら false
    bool Measure(const std::wstring& text, const TextStyle& style, float padding, int& width, int& height);

    // 借りる(同じ設定のテクスチャがあれば共有。無ければ作る)。作れなかったら nullptr。使い終わったら Release する
    Shared* Acquire(const Desc& desc);
    // 返す。借りている数が0になったら解放する
    void Release(Shared* shared);

    // 共有表を空にする(Direct3D を解放する前に。何回呼んでもよい)
    void ReleaseAll();

    // デバッグ用: 今ある共有テクスチャの数と、おおよその合計のメモリ(バイト)
    int GetCount();
    size_t GetBytes();
}
