#include "DemoBodyReset.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <PxPhysicsAPI.h>

void ResetDemoBody(GameObject* body, DirectX::XMFLOAT3 position) {
    if (!body) return;
    RigidbodyComponent* rigidbody = body->GetComponent<RigidbodyComponent>();
    if (!rigidbody) return;

    // PhysXの剛体を、指定の位置へ移す(向きは元に戻す)。速さと回る速さも0にする
    if (physx::PxRigidActor* actor = rigidbody->GetActor()) {
        actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z)));
        if (physx::PxRigidDynamic* dynamic = actor->is<physx::PxRigidDynamic>()) {
            dynamic->setLinearVelocity(physx::PxVec3(0.f));
            dynamic->setAngularVelocity(physx::PxVec3(0.f));
        }
    }
    body->transform.position = position;
    body->transform.rotate = { 0.f, 0.f, 0.f };
}
