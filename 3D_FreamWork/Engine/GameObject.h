#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Transform.h"
#include "Component.h"

// GameObjectはそれ自体では何もしない「入れ物」。
// 実際の見た目や動きは、AddComponentで追加したComponent(部品)が担う。
class GameObject {
private:
    std::string name;
    std::string tag;
    bool isActive = true;

    std::vector<std::unique_ptr<Component>> components;

public:
    // すべてのGameObjectが必ず持つ、特別な存在としてのTransform
    Transform transform;

    GameObject() = default;
    explicit GameObject(std::string name, std::string tag = "")
        : name(std::move(name)), tag(std::move(tag)) {
    }
    virtual ~GameObject() = default;

    // ─── ライフサイクル ─────────────────────────
    // 持っている全Componentを更新する(GameObject自身は何も処理しない)
    void Update(float dt) {
        if (!isActive) return;
        for (auto& c : components)
            if (c->IsEnabled()) c->Update(dt);
    }

    void Draw() {
        if (!isActive) return;
        for (auto& c : components)
            if (c->IsEnabled()) c->Draw();
    }

    void Uninit() {
        for (auto& c : components) c->Uninit();
    }

    // ─── コンポーネント操作 ─────────────────────
    // 新しいComponentを追加する。例: obj->AddComponent<SpriteRenderer>();
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        comp->SetOwner(this);
        T* ptr = comp.get();
        components.push_back(std::move(comp));
        ptr->Init();
        return ptr;
    }

    // 型を指定して、付いているComponentを探す。無ければnullptr。
    template<typename T>
    T* GetComponent() {
        for (auto& c : components) {
            if (T* casted = dynamic_cast<T*>(c.get())) return casted;
        }
        return nullptr;
    }

    // ─── アクセサ ───────────────────────────────
    const std::string& GetName() const { return name; }
    const std::string& GetTag()  const { return tag; }
    bool GetIsActive() const { return isActive; }
    void SetActive(bool v) { isActive = v; }
};
