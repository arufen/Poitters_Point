#include "Game/Component/ComponentItemPickUp.h"
#include "Game/Component/ComponentStateIdleWalk.h"
#include "Game/MainStage.h"

void ComponentItemPickUp::Init()
{
    Super::Init();
    isHeld_ = false;
}

void ComponentItemPickUp::Update()
{
    Super::Update();
}

void ComponentItemPickUp::OnPickedUp(Object* holder)
{
    if(isHeld_)
        return;    // already held, don't pick up twice

    isHeld_ = true;

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

void ComponentItemPickUp::OnThrown()
{
    if(!isHeld_)
        return;

    isHeld_ = false;

    // figure out which way the player is facing, same way ComponentStateThrow does it
    float3 direction = {0.0f, 0.0f, 1.0f};    // fallback if we can't find a model
    if(holder_) {
        if(auto model = holder_->GetComponent<ComponentModel>())
            direction = -model->GetWorldVectorAxisZ();
    }

    // detach from the hand
    GetOwner()->RemoveComponent<ComponentAttachModel>();

    // turn collision back on so it can hit stuff / be picked up again
    if(auto col = GetOwner()->GetComponent<ComponentCollision>())
        col->SetStatus(Component::StatusBit::Enable, true);

    // TODO: actually launch the item using `direction` - see my question below
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

            ImGui::Text(isHeld_ ? "Held" : "On ground");

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
