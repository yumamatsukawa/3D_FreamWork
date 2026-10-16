#pragma once
#include <string>

// 画像を何度も読み込まないように、みんなで使い回す仕組み。
// 同じパスの画像は1回だけ読み、「何人が使っているか」(参照カウント)を数えて管理する。
// SpriteRenderer / MeshRenderer は中で自動で使うので、ゲーム側は普段意識しなくてよい
namespace Resources {
    // 画像を借りる。同じパスは2回目以降読み込まずに同じIDを返す(区切り文字 \ と / 、大文字小文字の違いは同じものとして扱う)
    // 読み込みに失敗したらUINT_MAX
    unsigned int LoadTexture(const std::wstring& path);

    // 借りた画像を返す(参照カウントを1減らす)。0になってもすぐには解放せず、UnloadUnused()まで残す
    // (弾や敵のように出たり消えたりするものが、その度に読み直さないように)
    void ReleaseTexture(unsigned int id);

    // 音(WAV)の波形データを借りる。画像と同じ仕組みで、同じパスは1回だけ読み、2回目以降は同じIDを返す。
    // Audio::PlayOneShot / AudioSourceComponent が中で自動で使うので、ゲーム側は普段意識しなくてよい
    // 読み込みに失敗したらUINT_MAX
    unsigned int LoadSound(const std::wstring& path);

    // 借りた音を返す(参照カウントを1減らす)。0になってもすぐには解放せず、UnloadUnused()まで残す
    void ReleaseSound(unsigned int id);

    // 誰も使っていない(参照カウントが0の)画像と音をまとめて解放する。SceneManagerがシーン切り替えの時に呼ぶ
    void UnloadUnused();

    // ─── Engine内部用(GameEngineだけが呼ぶ。ゲーム側からは呼ばない) ─────────────
    void UnloadAll();   // 参照カウントに関係なく、すべての画像と音を解放する(終了時。鳴っている音は止める)
}
