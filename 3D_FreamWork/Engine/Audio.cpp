#include "Audio.h"
#include <vector>
#include <cassert>

namespace Audio {

    // 内部データ
    struct SoundData {
        IXAudio2SourceVoice* sourceVoice = nullptr;
        BYTE* audioData = nullptr;
        UINT32               audioSize = 0;
        bool                 loop = false;
        bool                 valid = false;
    };

    IXAudio2* xAudio2 = nullptr;
    IXAudio2MasteringVoice* masterVoice = nullptr;
    std::vector<SoundData>  sounds;

    // WAV読み込みヘルパー
    struct WAVHeader {
        char    riff[4];
        UINT32  fileSize;
        char    wave[4];
        char    fmt[4];
        UINT32  fmtSize;
        UINT16  audioFormat;
        UINT16  channels;
        UINT32  sampleRate;
        UINT32  byteRate;
        UINT16  blockAlign;
        UINT16  bitsPerSample;
        char    data[4];
        UINT32  dataSize;
    };

    static bool LoadWAV(const std::wstring& filepath,
        WAVEFORMATEX& fmt, BYTE*& data, UINT32& size) {
        FILE* file = nullptr;
        _wfopen_s(&file, filepath.c_str(), L"rb");
        if (!file) {
            OutputDebugStringA("★ Audio: ファイルが見つかりません\n");
            return false;
        }

        WAVHeader header;
        fread(&header, sizeof(header), 1, file);

        fmt.wFormatTag = header.audioFormat;
        fmt.nChannels = header.channels;
        fmt.nSamplesPerSec = header.sampleRate;
        fmt.nAvgBytesPerSec = header.byteRate;
        fmt.nBlockAlign = header.blockAlign;
        fmt.wBitsPerSample = header.bitsPerSample;
        fmt.cbSize = 0;

        size = header.dataSize;
        data = new BYTE[size];
        fread(data, size, 1, file);
        fclose(file);
        return true;
    }

    bool Init() {
        HRESULT hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) {
            OutputDebugStringA("★ XAudio2Create 失敗\n");
            return false;
        }

        hr = xAudio2->CreateMasteringVoice(&masterVoice);
        if (FAILED(hr)) {
            OutputDebugStringA("★ CreateMasteringVoice 失敗\n");
            return false;
        }

        return true;
    }

    void Uninit() {
        ReleaseAll();
        if (masterVoice) { masterVoice->DestroyVoice(); masterVoice = nullptr; }
        if (xAudio2) { xAudio2->Release();          xAudio2 = nullptr; }
    }


    unsigned int Load(const std::wstring& filepath, bool loop) {
        WAVEFORMATEX fmt;
        BYTE* data = nullptr;
        UINT32       size = 0;

        if (!LoadWAV(filepath, fmt, data, size)) return UINT_MAX;

        IXAudio2SourceVoice* sourceVoice = nullptr;
        HRESULT hr = xAudio2->CreateSourceVoice(&sourceVoice, &fmt);
        if (FAILED(hr)) {
            delete[] data;
            OutputDebugStringA("★ CreateSourceVoice 失敗\n");
            return UINT_MAX;
        }

        SoundData sound;
        sound.sourceVoice = sourceVoice;
        sound.audioData = data;
        sound.audioSize = size;
        sound.loop = loop;
        sound.valid = true;

        sounds.push_back(sound);
        return (unsigned int)(sounds.size() - 1);
    }

    // 再生
    void Play(unsigned int id, float volume) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        auto& s = sounds[id];

        // 再生前にバッファをリセット
        s.sourceVoice->Stop();
        s.sourceVoice->FlushSourceBuffers();

        XAUDIO2_BUFFER buffer = {};
        buffer.AudioBytes = s.audioSize;
        buffer.pAudioData = s.audioData;
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        buffer.LoopCount = s.loop ? XAUDIO2_LOOP_INFINITE : 0;

        s.sourceVoice->SubmitSourceBuffer(&buffer);
        s.sourceVoice->SetVolume(volume);
        s.sourceVoice->Start();
    }

    // 停止
    void Stop(unsigned int id) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].sourceVoice->Stop();
        sounds[id].sourceVoice->FlushSourceBuffers();
    }

    // 一時停止
    void Pause(unsigned int id) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].sourceVoice->Stop();
    }

    // 再開
    void Resume(unsigned int id) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].sourceVoice->Start();
    }

    // 音量調整
    void SetVolume(unsigned int id, float volume) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].sourceVoice->SetVolume(volume);
    }

    // ループ設定
    void SetLoop(unsigned int id, bool loop) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].loop = loop;
    }

    // 解放
    void Release(unsigned int id) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        auto& s = sounds[id];
        s.sourceVoice->Stop();
        s.sourceVoice->DestroyVoice();
        delete[] s.audioData;
        s.sourceVoice = nullptr;
        s.audioData = nullptr;
        s.valid = false;
    }

    void ReleaseAll() {
        for (int i = 0; i < (int)sounds.size(); i++) {
            Release(i);
        }
        sounds.clear();
    }
}