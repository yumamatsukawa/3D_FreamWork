#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Transform.h"
#include "Component.h"
#include "Collider.h"

class Scene;

// GameObjectはそれ自体では何もしない「入れ物」。
// 実際の見た目や動きは、AddComponentで追加したComponent(部品)が担う。
class GameObject {
private:
    std::string name;
    std::string tag;
    bool isActive = true;
    bool isDestroyed = false;  // trueになったら、Sceneが次のUpdateで実際に破棄する
    Scene* scene = nullptr;    // 自分がどのSceneに属しているか(Scene::CreateObjectが設定する)
    int drawPriority = 0;      // Scene::Draw()の描画順。小さいほど先に描画される(既定0)。
                                // スカイボックスのように「必ず一番最初に描画したい」時に使う

    std::vector<std::unique_ptr<Component>> components;

public:
    // すべてのGameObjectが必ず持つ、特別な存在としてのTransform
    Transform transform;

    GameObject() = default;
    explicit GameObject(std::string name, std::string tag = "")
        : name(std::move(name)), tag(std::move(tag)) {
    }
    virtual ~GameObject() {};

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

    // ─── 当たり判定のコールバック転送 ───────────
    // Collider側から呼ばれ、付いている全Componentに通知する。
    // 反応したいComponent側でOnTriggerStay2Dなどをoverrideすれば受け取れる。
    void OnTriggerEnter2D(Collider2D* other) { for (auto& c : components) c->OnTriggerEnter2D(other); }
    void OnTriggerStay2D(Collider2D* other) { for (auto& c : components) c->OnTriggerStay2D(other); }
    void OnTriggerExit2D(Collider2D* other) { for (auto& c : components) c->OnTriggerExit2D(other); }

    void OnCollisionEnter2D(CollisionInfo info) { for (auto& c : components) c->OnCollisionEnter2D(info); }
    void OnCollisionStay2D(CollisionInfo info) { for (auto& c : components) c->OnCollisionStay2D(info); }
    void OnCollisionExit2D(CollisionInfo info) { for (auto& c : components) c->OnCollisionExit2D(info); }

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

    // SetActive(false)は「非表示・Update/Draw停止」なだけで、破棄はされない
    // (プーリングなどで、後で再利用するために取っておける)
    bool GetIsActive() const { return isActive; }
    void SetActive(bool v) { isActive = v; }

    // Destroy()は「本当に破棄する」印を付ける。次のScene::Updateで実際に取り除かれる
    void Destroy() { isDestroyed = true; }
    bool GetIsDestroyed() const { return isDestroyed; }

    // 自分が属しているScene。Componentから「新しいオブジェクトを生成したい」時などに使う
    // 例: GetOwner()->GetScene()->CreateObject(...)
    Scene* GetScene() const { return scene; }
    void SetScene(Scene* s) { scene = s; }

    // Scene::Draw()での描画順。小さいほど先に描画される。
    // 例: スカイボックスは深度を書き込まずに必ず最初に描画したいので、非常に小さい値にする
    int GetDrawPriority() const { return drawPriority; }
    void SetDrawPriority(int priority) { drawPriority = priority; }
};
