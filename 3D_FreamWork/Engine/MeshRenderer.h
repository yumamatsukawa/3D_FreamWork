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

    void Draw() override {
        // Transform::GetWorldMatrix()が親子階層を内部で処理してくれるので、
        // ここではローカルのtransformをそのまま渡せばよい。
        // カメラは2D(GetCamera)とは別の3D専用カメラ(GetCamera3D)を使う
        mesh->Draw(GetOwner()->transform, Image::GetCamera3D(), color);
    }

    void Uninit() override {
        mesh->Uninit();
    }
};
