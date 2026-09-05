#include "Game/Component/ComponentPlayerPickUpTrigger.h"
#include "Game/Component/ComponentStateIdleWalk.h"
#include "Game/MainStage.h"

void ComponentPlayerPickUpTrigger::Init()
{
    Super::Init();
    // nothing to spawn here - this component just watches for nearby items
}

void ComponentPlayerPickUpTrigger::Update()
{
    Super::Update();

    // already holding something? don't look for more, wait for a throw first
    if(HasItem())
        return;
}

void ComponentPlayerPickUpTrigger::ThrowItem()
{
    if(auto item = heldItem_.lock()) {
        if(auto pickup = item->GetComponent<ComponentItemPickUp>())
            pickup->OnThrown();
    }

    heldItem_.reset();
}

void ComponentPlayerPickUpTrigger::PickUpItem()
{
    // check every item in the scene that has ComponentItemPickUp
    auto items = Scene::GetObjectsPtr<PoittersPoint::Item>();
    for(auto& item : items) {
        auto pickup = item->GetComponent<ComponentItemPickUp>();
        if(!pickup || pickup->IsHeld())
            continue;    // no pickup component, or already held by someone else

        float dist = length(item->GetTranslate() - GetOwner()->GetTranslate());
        if(dist <= pickupRange_) {
            pickup->OnPickedUp(GetOwner());
            heldItem_ = item;
            break;    // only grab one item per frame
        }
    }
}

PoittersPoint::ItemPtr ComponentPlayerPickUpTrigger::GetNearbyItem()
{
    // check every item in the scene that has ComponentItemPickUp
    auto items = Scene::GetObjectsPtr<PoittersPoint::Item>();
    for(auto& item : items) {
        auto pickup = item->GetComponent<ComponentItemPickUp>();
        if(!pickup || pickup->IsHeld())
            continue;    // no pickup component, or already held by someone else

        float dist = length(item->GetTranslate() - GetOwner()->GetTranslate());
        if(dist <= pickupRange_)
            return item;    // found the closest-checked one in range
    }
    return nullptr;    // nothing nearby
}

void ComponentPlayerPickUpTrigger::GUI()
{
    __super::GUI();

    // GUI内に出現させる
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("PickUpTrigger")) {
            // 有効/無効
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text(HasItem() ? "Holding item" : "Empty handed");
            ImGui::DragFloat("Pickup Range", &pickupRange_, 0.5f, 0.0f, 100.0f);

            // GUI上でオーナーから自分(SampleObjectController)を削除します
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentPlayerPickUpTrigger)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentPlayerPickUpTrigger)
