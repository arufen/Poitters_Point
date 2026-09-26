#include "Game/Component/ComponentItemPickUp.h"
#include "Game/Component/ComponentStateIdleWalk.h"
#include "Game/MainStage.h"

void ComponentItemPickUp::Init()
{
    Super::Init();
    isHeld_   = false;
    isThrown_ = false;
    isGround_ = false;
    velocity_ = {0.0f, 0.0f, 0.0f};
}

void ComponentItemPickUp::Update()
{
    Super::Update();

    // while the item is flying, only move it sideways (x/z) ourselves.
    // y is left alone on purpose - the collision's UseGravity(true) already
    // pulls it down and resolves ground overlap. Moving y ourselves too
    // caused two systems to fight over position (item would freeze then sink).
    if(isThrown_) {
        float delta = GetDeltaTime();

        float3 pos  = GetOwner()->GetTranslate();
        pos.x      += velocity_.x * delta;
        pos.z      += velocity_.z * delta;
        GetOwner()->SetTranslate(pos);

        CheckGround();
    }
    else {
        velocity_ = {0.0f, 0.0f, 0.0f};
    }
}

void ComponentItemPickUp::CheckGround()
{
    // isGround_ gets set to true by OnLanded(), which Item::OnHit() calls
    // when this item's collision actually touches the "Ground" object
    if(isGround_) {
        isThrown_ = false;
        velocity_ = {0.0f, 0.0f, 0.0f};
    }
}

void ComponentItemPickUp::OnLanded()
{
    float3 pos = GetOwner()->GetTranslate();
    printfDx("OnLanded! pos: %.2f, %.2f, %.2f\n", pos.x, pos.y, pos.z);
    isGround_ = true;
}

void ComponentItemPickUp::OnPickedUp(Object* holder)
{
    if(isHeld_)
        return;    // already held, don't pick up twice

    isHeld_   = true;
    isThrown_ = false;    // just in case it was still flying when picked up

    // turn off this item's own collision so it stops blocking the player
    if(auto col = GetOwner()->GetComponent<ComponentCollision>())
        col->SetStatus(Component::StatusBit::Enable, false);

    //Size reset
    float3 defaultSize = GetOwner()->GetScaleAxisXYZ();

    // attach itself to the holder's hand
    auto attach = GetOwner()->AddComponent<ComponentAttachModel>();
    attach->SetAttachObject(holder->GetName(), "Arm:Right:Lower_end_end");
    //attach->SetAttachOffset({12, 17, 0});

    GetOwner()->SetScaleAxisXYZ(defaultSize);

    // remember who's holding us, so we know which way to throw later
    holder_ = holder;
}

void ComponentItemPickUp::ResetRotation()
{
    float3 zero{0.0f, 0.0f, 0.0f};
    GetOwner()->SetRotationAxisXYZ(zero);
}

void ComponentItemPickUp::OnThrow()
{
    if(!isHeld_)
        return;

    isHeld_ = false;

    // figure out which way the player is facing, same way ComponentStateThrow does it
    float3 direction = {0.0f, 0.0f, 1.0f};    // fallback if we can't find a model

    if(holder_) {
        if(auto model = holder_->GetComponent<ComponentModel>())
            direction = normalize(-model->GetWorldVectorAxisZ());
    }

    // detach from the hand
    GetOwner()->RemoveComponent<ComponentAttachModel>();

    //Reset rot
    ResetRotation();

    // about to fly again, so it's not touching ground anymore
    isGround_ = false;

    // move it up FIRST, before turning collision back on
    auto   owner        = GetOwner();
    float  throwOffset  = 30.0f;
    float3 currentPos   = owner->GetTranslate();
    currentPos.y       += throwOffset;
    owner->SetTranslate(currentPos);

    printfDx("after move up, pos: %f, %.2f, %.2f\n", currentPos.x, currentPos.y, currentPos.z);

    // NOW turn collision back on, so it starts checking from the safe, new position
    if(auto col = GetOwner()->GetComponent<ComponentCollision>())
        col->SetStatus(Component::StatusBit::Enable, true);

    velocity_    = direction * throwSpeed_;
    velocity_.y += upBoost_;
    isThrown_    = true;

    holder_ = nullptr;
}

void ComponentItemPickUp::GUI()
{
    __super::GUI();

    // GUI内に出現させる
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("PickupAbleItem")) {
            // 有効/無効
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text(isHeld_ ? "Held" : (isThrown_ ? "Thrown" : "On ground"));

            // 移動の基本情報
            ImGui::DragFloat(u8"投げるの速さ", &throwSpeed_, 0.1f);
            ImGui::DragFloat(u8"上の投げる速さ", &upBoost_, 0.1f);
            float vel[3] = {velocity_.x, velocity_.y, velocity_.z};
            if(ImGui::DragFloat3(u8"速度", vel, 0.1f))
                velocity_ = {vel[0], vel[1], vel[2]};

            // GUI上でオーナーから自分(SampleObjectController)を削除します
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentItemPickUp)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentItemPickUp)
