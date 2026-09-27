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
    //
    // ★ "fmt "チャンクの直後に必ず"data"チャンクが来る44バイト固定ヘッダー、という
    //   決め打ちでは読めないファイルがある(例: ffmpeg/Lavfで書き出したWAVは、"fmt "と
    //   "data"の間に"LIST"(メタデータ)チャンクが挟まっていることが多い)。
    //   そのため、チャンクを先頭から1つずつ読み進めて、"fmt "と"data"を名前で探す
    static bool LoadWAV(const std::wstring& filepath,
        WAVEFORMATEX& fmt, BYTE*& data, UINT32& size) {
        FILE* file = nullptr;
        _wfopen_s(&file, filepath.c_str(), L"rb");
        if (!file) {
            OutputDebugStringA("★ Audio: ファイルが見つかりません\n");
            return false;
        }

        char riff[4], wave[4];
        UINT32 riffSize;
        fread(riff, 1, 4, file);
        fread(&riffSize, sizeof(UINT32), 1, file);
        fread(wave, 1, 4, file);
        if (memcmp(riff, "RIFF", 4) != 0 || memcmp(wave, "WAVE", 4) != 0) {
            OutputDebugStringA("★ Audio: WAVファイルではありません\n");
            fclose(file);
            return false;
        }

        bool foundFmt = false, foundData = false;
        data = nullptr;
        size = 0;

        char chunkId[4];
        UINT32 chunkSize;
        while (fread(chunkId, 1, 4, file) == 4 && fread(&chunkSize, sizeof(UINT32), 1, file) == 1) {
            if (memcmp(chunkId, "fmt ", 4) == 0) {
                UINT16 audioFormat = 0, channels = 0, blockAlign = 0, bitsPerSample = 0;
                UINT32 sampleRate = 0, byteRate = 0;
                fread(&audioFormat, sizeof(UINT16), 1, file);
                fread(&channels, sizeof(UINT16), 1, file);
                fread(&sampleRate, sizeof(UINT32), 1, file);
                fread(&byteRate, sizeof(UINT32), 1, file);
                fread(&blockAlign, sizeof(UINT16), 1, file);
                fread(&bitsPerSample, sizeof(UINT16), 1, file);
                // fmtチャンクが16バイトより大きい(WAVE_FORMAT_EXTENSIBLEなど)場合は、残りを読み飛ばす
                long read = 16;
                if ((long)chunkSize > read) fseek(file, chunkSize - read, SEEK_CUR);

                fmt.wFormatTag = audioFormat;
                fmt.nChannels = channels;
                fmt.nSamplesPerSec = sampleRate;
                fmt.nAvgBytesPerSec = byteRate;
                fmt.nBlockAlign = blockAlign;
                fmt.wBitsPerSample = bitsPerSample;
                fmt.cbSize = 0;
                foundFmt = true;
            }
            else if (memcmp(chunkId, "data", 4) == 0) {
                size = chunkSize;
                data = new BYTE[size];
                fread(data, size, 1, file);
                foundData = true;
                break;  // 音声データは見つかったので、以降のチャンク(メタデータ等)は読まない
            }
            else {
                // 知らないチャンク(LIST/fact/JUNKなど)は読み飛ばす。
                // RIFFのチャンクは2バイト単位に揃えられるため、奇数サイズの時は1バイト分の
                // パディングも一緒に読み飛ばす
                fseek(file, chunkSize + (chunkSize & 1), SEEK_CUR);
            }
        }

        fclose(file);
        if (!foundFmt || !foundData) {
            OutputDebugStringA("★ Audio: fmt/dataチャンクが見つかりません\n");
            delete[] data;
            data = nullptr;
            return false;
        }
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