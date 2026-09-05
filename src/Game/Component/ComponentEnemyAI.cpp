#include <Game/Component/ComponentEnemyAI.h>
#include <Game/Component/ComponentEnemyController.h>

void ComponentEnemyAI::Init()
{
    __super::Init();
    current_state_ = AIState::Idle;
}

ObjectPtr ComponentEnemyAI::FindNearestItem()
{
    auto   owner  = GetOwner();
    float3 my_pos = owner->GetTranslate();

    ObjectPtr nearest_prop = nullptr;
    float     min_dist_sq  = search_range_ * search_range_;

    // Retrieve all scene objects
    auto objects = Scene::Object::GetArray<Object>();

    for(auto& obj : objects) {
        if(!obj || obj.get() == owner)
            continue;

        if(obj->GetName().find("Prop_") != std::string::npos || obj->GetName().find("Rock") != std::string::npos) {
            float3 prop_pos = obj->GetTranslate();
            float3 diff     = prop_pos - my_pos;

            // Cast or extract float from hlslpp dot product
            float dist_sq = dot(diff, diff).x;

            if(dist_sq < min_dist_sq) {
                min_dist_sq  = dist_sq;
                nearest_prop = obj;
            }
        }
    }
    return nearest_prop;
}

ObjectPtr ComponentEnemyAI::FindNearestPlayer()
{
    auto   owner  = GetOwner();
    float3 my_pos = owner->GetTranslate();

    ObjectPtr nearest_player = nullptr;
    float     min_dist_sq    = 10000.0f * 10000.0f;

    // Retrieve all scene objects
    auto objects = Scene::Object::GetArray<Object>();

    for(auto& obj : objects) {
        if(!obj || obj.get() == owner)
            continue;

        if(obj->GetName().find("Player") != std::string::npos) {
            float3 player_pos = obj->GetTranslate();
            float3 diff       = player_pos - my_pos;

            float dist_sq = dot(diff, diff).x;

            if(dist_sq < min_dist_sq) {
                min_dist_sq    = dist_sq;
                nearest_player = obj;
            }
        }
    }
    return nearest_player;
}

void ComponentEnemyAI::Update()
{
    __super::Update();

    if(!GetStatus(StatusBit::Enable))
        return;

    auto controller = GetOwner()->GetComponent<ComponentEnemyController>();
    if(!controller)
        return;

    switch(current_state_) {
    case AIState::Idle:
        {
            if(controller->IsCarryingProp()) {
                current_state_ = AIState::AimAndThrow;
                break;
            }

            if(auto prop = FindNearestItem()) {
                target_prop_   = prop;
                current_state_ = AIState::SeekProp;
            }
            // Fallback for testing: directly move to player if no props exist
            else if(auto player = FindNearestPlayer()) {
                controller->MoveTo(player->GetTranslate(), 0.3f);
            }
            break;
        }

    case AIState::SeekProp:
        {
            auto prop = target_prop_.lock();
            if(!prop) {
                current_state_ = AIState::Idle;
                break;
            }

            float3 my_pos   = GetOwner()->GetTranslate();
            float3 prop_pos = prop->GetTranslate();

            controller->MoveTo(prop_pos, 0.5f);

            float3 diff = prop_pos - my_pos;
            if(dot(diff, diff).x <= pickup_range_ * pickup_range_) {
                current_state_ = AIState::PickUpProp;
            }
            break;
        }

    case AIState::PickUpProp:
        {
            auto prop = target_prop_.lock();
            if(prop && controller->PickUpProp(prop)) {
                target_player_ = FindNearestPlayer();
                current_state_ = AIState::AimAndThrow;
            }
            else {
                current_state_ = AIState::Idle;
            }
            break;
        }

    case AIState::AimAndThrow:
        {
            if(!controller->IsCarryingProp()) {
                current_state_ = AIState::Idle;
                break;
            }

            auto target = target_player_.lock();
            if(!target) {
                target         = FindNearestPlayer();
                target_player_ = target;
            }

            if(target) {
                float3 target_pos = target->GetTranslate();
                controller->RotateTo(target_pos, 8.0f);
                controller->ThrowCarriedProp(target_pos, 15.0f);
                current_state_ = AIState::Idle;
            }
            break;
        }

    case AIState::Evade:
        {
            break;
        }
    }
}

void ComponentEnemyAI::GUI()
{
    __super::GUI();

    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode(u8"敵AI Brain (Enemy AI)")) {
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効 (Enable)", &enable))
                SetStatus(StatusBit::Enable, enable);

            const char* state_names[] = {"Idle", "SeekProp", "PickUpProp", "AimAndThrow", "Evade"};
            ImGui::Text(u8"現在のState: %s", state_names[static_cast<int>(current_state_)]);

            ImGui::DragFloat(u8"索敵距離 (Search Range)", &search_range_, 10.0f, 10.0f, 1000.0f);
            ImGui::DragFloat(u8"拾い距離 (Pickup Range)", &pickup_range_, 1.0f, 1.0f, 50.0f);

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentEnemyAI)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentEnemyAI)
