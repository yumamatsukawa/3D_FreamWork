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
    void Play(unsigned int id, float volume = 1.f);
    void Stop(unsigned int id);
    void Pause(unsigned int id);
    void Resume(unsigned int id);
    void SetVolume(unsigned int id, float volume);
    void SetLoop(unsigned int id, bool loop);
    void Release(unsigned int id);
    void ReleaseAll();
}