#pragma once
#include "Component.h"
#include "Mesh.h"
#include "Image.h"
#include <memory>

// GameObjectに「3Dメッシュを描画する見た目」を持たせるComponent。
// SpriteRendererの3D版。Cameraを使ってWorld/View/Projectionで描画する。
class MeshRenderer : public Component {
private:
    std::shared_ptr<Mesh> mesh;
    unsigned int texID = UINT_MAX;
    DirectX::XMFLOAT4 color{ 1.f, 1.f, 1.f, 1.f };
    bool unlit = false;

public:
    // 例: obj->AddComponent<MeshRenderer>(Mesh::CreateCube());
    explicit MeshRenderer(const std::vector<MeshVertex>& vertices) {
        mesh = std::make_shared<Mesh>();
        mesh->Init(vertices);
    }

    void SetTexture(const std::wstring& filepath) {
        texID = Image::LoadTexture(filepath);
        mesh->SetTexture(Image::GetTextureView(texID));
    }

    void SetColor(float r, float g, float b, float a) { color = { r, g, b, a }; }

    // スカイボックスなど、光源の影響を受けたくない(常に一定の明るさで表示したい)場合に使う
    void SetUnlit(bool value) { unlit = value; }

    void Draw() override {
        // Transform::GetWorldMatrix()が親子階層を内部で処理してくれるので、
        // ここではローカルのtransformをそのまま渡せばよい。
        // カメラは2D(GetCamera)とは別の3D専用カメラ(GetCamera3D)を使う
        if (unlit) {
            static const Light fullBright{ {0.f,-1.f,0.f}, {0.f,0.f,0.f}, {1.f,1.f,1.f} };
            mesh->Draw(GetOwner()->transform, Image::GetCamera3D(), color, fullBright);
        }
        else {
            mesh->Draw(GetOwner()->transform, Image::GetCamera3D(), color, Image::GetLight());
        }
    }

    void Uninit() override {
        mesh->Uninit();
    }
};
