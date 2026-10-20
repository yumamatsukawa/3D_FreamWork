#include "TouchCounterController.h"

// どのコールバックも、相手のタグが targetTag の時だけ数える

void TouchCounterController::OnTriggerEnter(Collider2D* other) {
    if (!other || other->GetTag() != targetTag) return;
    trigger.enter++;
    touching++;
}

void TouchCounterController::OnTriggerStay(Collider2D* other) {
    if (!other || other->GetTag() != targetTag) return;
    trigger.stay++;
}

void TouchCounterController::OnTriggerExit(Collider2D* other) {
    if (!other || other->GetTag() != targetTag) return;
    trigger.exit++;
    if (touching > 0) touching--;
}

void TouchCounterController::OnCollisionEnter(CollisionInfo info) {
    if (!info.other || info.other->GetTag() != targetTag) return;
    collision.enter++;
}

void TouchCounterController::OnCollisionStay(CollisionInfo info) {
    if (!info.other || info.other->GetTag() != targetTag) return;
    collision.stay++;
}

void TouchCounterController::OnCollisionExit(CollisionInfo info) {
    if (!info.other || info.other->GetTag() != targetTag) return;
    collision.exit++;
}
