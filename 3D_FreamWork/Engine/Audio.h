#pragma once
#include <xaudio2.h>
#include <string>
#pragma comment(lib, "xaudio2.lib")

namespace Audio {
    bool Init();
    void Uninit();

    /* =======================================================
    *   Audioのロード
    * ".wavのファイルのみがロードできる"
    * BGMなどループする  ファイルはloopをtrueに
    * SE などループしないファイルはloopをfalseに
    ======================================================== */
    unsigned int Load(const std::wstring& filepath,
        bool loop = false);
    // 再生
    void Play(unsigned int id, float volume = 1.f);
    // 停止
    void Stop(unsigned int id);
    // 一時停止
    void Pause(unsigned int id);
    // 再開
    void Resume(unsigned int id);
    // 音量調整
    void SetVolume(unsigned int id, float volume);
    // ループするかどうか
    void SetLoop(unsigned int id, bool loop);
    // 削除
    void Release(unsigned int id);
    // すべて削除
    void ReleaseAll();
}