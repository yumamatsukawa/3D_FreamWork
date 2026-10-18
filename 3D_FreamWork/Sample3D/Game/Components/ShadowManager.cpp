#include "ShadowManager.h"
#include "../../../Engine/Render3D/Shadow.h"

void ShadowManager::Init() {
    // 影を有効にする(平行光源 Image::GetLight() の向きで影ができる。ほかの値は既定のまま)
    Shadow::Settings& settings = Shadow::GetSettings();
    settings.enabled = true;
    settings.distance = shadowDistance;
}

void ShadowManager::Uninit() {
    // 設定は全シーンで共通なので、このシーンが終わったら既定(影なし)に戻す
    Shadow::SetSettings(Shadow::Settings());
}
