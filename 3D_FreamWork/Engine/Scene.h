#pragma once
#include <vector>
#include <memory>
#include "GameObject.h"

class Scene : public GameObject
{
protected:
    std::vector<std::unique_ptr<GameObject>> objects;
public:
    virtual ~Scene() = default;
    void Init()           override;
    void Update(float dt) override;
    void Draw()           override;
    void Uninit()         override;

    template<typename T, typename... Args>
    T* AddObject(Args&&... args) {
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();
        obj->Init();
        objects.push_back(std::move(obj));
        return ptr;
    }
};