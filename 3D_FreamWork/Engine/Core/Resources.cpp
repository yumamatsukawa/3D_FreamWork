#include "Resources.h"
#include "../Render/Image.h"
#include "../Audio/Audio.h"
#include "../Render3D/Model.h"
#include <unordered_map>
#include <cwctype>
#include <climits>

namespace {
    // 読み込んだ画像1枚ぶんの管理情報
    struct TextureEntry {
        std::wstring key;    // そろえた後のパス(textureIDsから消す時に使う)
        int refCount = 0;    // 何人が使っているか
    };

    std::unordered_map<std::wstring, unsigned int> textureIDs;  // そろえた後のパス → 画像のID
    std::unordered_map<unsigned int, TextureEntry> textures;    // 画像のID → 管理情報

    // 音も画像と同じ仕組み(読み込み・解放はAudioがする。IDはAudio::LoadWaveのID)
    std::unordered_map<std::wstring, unsigned int> soundIDs;    // そろえた後のパス → 音のID
    std::unordered_map<unsigned int, TextureEntry> sounds;      // 音のID → 管理情報

    // モデルは shared_ptr で共有する。Resourcesも1人として持つので、use_count()が1なら「誰も使っていない」
    std::unordered_map<std::wstring, std::shared_ptr<Model>> models;  // そろえた後のパス → モデル

    // 書き方が違うだけの同じパスを、同じ文字列にそろえる("Assets/Player.png" と "assets\player.png" を同じにする)
    std::wstring NormalizePath(const std::wstring& path) {
        std::wstring key = path;
        for (wchar_t& c : key) {
            if (c == L'/') c = L'\\';
            else c = (wchar_t)std::towlower(c);
        }
        return key;
    }
}

namespace Resources {
    unsigned int LoadTexture(const std::wstring& path) {
        std::wstring key = NormalizePath(path);

        // 読み込み済みなら、読み直さずに同じIDを返す
        auto found = textureIDs.find(key);
        if (found != textureIDs.end()) {
            textures[found->second].refCount++;
            return found->second;
        }

        // 初めての画像だけ読み込む(失敗した時は登録しないので、次に呼ばれた時にもう一度読みにいく)
        unsigned int id = Image::LoadTexture(path);
        if (id == UINT_MAX) return UINT_MAX;

        textureIDs[key] = id;
        textures[id] = { key, 1 };
        return id;
    }

    void ReleaseTexture(unsigned int id) {
        auto found = textures.find(id);
        if (found == textures.end()) return;

        // 0になってもここでは解放しない(UnloadUnusedでまとめて解放する)
        if (found->second.refCount > 0) found->second.refCount--;
    }

    unsigned int LoadSound(const std::wstring& path) {
        std::wstring key = NormalizePath(path);

        // 読み込み済みなら、読み直さずに同じIDを返す
        auto found = soundIDs.find(key);
        if (found != soundIDs.end()) {
            sounds[found->second].refCount++;
            return found->second;
        }

        // 初めての音だけ読み込む(失敗した時は登録しないので、次に呼ばれた時にもう一度読みにいく)
        unsigned int id = Audio::LoadWave(path);
        if (id == UINT_MAX) return UINT_MAX;

        soundIDs[key] = id;
        sounds[id] = { key, 1 };
        return id;
    }

    void ReleaseSound(unsigned int id) {
        auto found = sounds.find(id);
        if (found == sounds.end()) return;

        // 0になってもここでは解放しない(UnloadUnusedでまとめて解放する)
        if (found->second.refCount > 0) found->second.refCount--;
    }

    std::shared_ptr<Model> LoadModel(const std::wstring& path) {
        std::wstring key = NormalizePath(path);

        // 読み込み済みなら、読み直さずに同じモデルを返す
        auto found = models.find(key);
        if (found != models.end()) return found->second;

        // 初めてのモデルだけ読み込む(失敗した時は登録しないので、次に呼ばれた時にもう一度読みにいく)
        std::shared_ptr<Model> model = Model::Load(path);
        if (model) models[key] = model;
        return model;
    }

    void UnloadUnused() {
        for (auto it = models.begin(); it != models.end();) {
            if (it->second.use_count() <= 1) it = models.erase(it);
            else ++it;
        }

        for (auto it = textures.begin(); it != textures.end();) {
            if (it->second.refCount <= 0) {
                Image::ReleaseTexture(it->first);
                textureIDs.erase(it->second.key);
                it = textures.erase(it);
            }
            else {
                ++it;
            }
        }

        // 音も同じ。鳴っている間は借りているので(参照カウントが1以上)、鳴っている音は解放されない
        for (auto it = sounds.begin(); it != sounds.end();) {
            if (it->second.refCount <= 0) {
                Audio::ReleaseWave(it->first);
                soundIDs.erase(it->second.key);
                it = sounds.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void UnloadAll() {
        for (auto& pair : models) {
            // 終了時にまだ使われている = どこかでModelRendererが残っている(モデルは、最後の1人が手放した時に解放される)
            if (pair.second.use_count() > 1) {
                OutputDebugStringW((L"★ Resources: 使われたままのモデルがあります: " + pair.first + L"\n").c_str());
            }
        }
        models.clear();

        for (auto& pair : sounds) {
            // 終了時にまだ使われている = どこかでReleaseSoundし忘れている(鳴っている音は、解放する時にAudioが止める)
            if (pair.second.refCount > 0) {
                OutputDebugStringW((L"★ Resources: 返し忘れの音があります: " + pair.second.key + L"\n").c_str());
            }
            Audio::ReleaseWave(pair.first);
        }
        sounds.clear();
        soundIDs.clear();

        for (auto& pair : textures) {
            // 終了時にまだ使われている = どこかでReleaseTextureし忘れている
            if (pair.second.refCount > 0) {
                OutputDebugStringW((L"★ Resources: 返し忘れの画像があります: " + pair.second.key + L"\n").c_str());
            }
            Image::ReleaseTexture(pair.first);
        }
        textures.clear();
        textureIDs.clear();
    }
}
