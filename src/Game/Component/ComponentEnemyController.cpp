#include <Game/Component/ComponentEnemyController.h>

void ComponentEnemyController::Init()
{
    __super::Init();
}

void ComponentEnemyController::Update()
{
    __super::Update();

    // Attach held prop to character's position/head offset
    if(auto prop = carried_prop_.lock()) {
        float3 hand_offset = GetOwner()->GetTranslate() + float3{0.0f, 10.0f, 0.0f};
        prop->SetTranslate(hand_offset);
    }
}

void ComponentEnemyController::MoveTo(const float3& target_pos, float speed)
{
    auto   owner       = GetOwner();
    float3 current_pos = owner->GetTranslate();
    float3 dir         = normalize(target_pos - current_pos);

    // Turn & Move forward
    owner->SetRotationToPositionWithLimit(target_pos, 5.0f);
    owner->SetTranslate(current_pos + dir * speed * GetDeltaTime60());
}

void ComponentEnemyController::RotateTo(const float3& target_pos, float turn_speed)
{
    GetOwner()->SetRotationToPositionWithLimit(target_pos, turn_speed);
}

bool ComponentEnemyController::PickUpProp(ObjectPtr prop)
{
    if(!prop || IsCarryingProp())
        return false;

    carried_prop_ = prop;
    return true;
}

void ComponentEnemyController::ThrowCarriedProp(const float3& target_pos, float throw_power)
{
    auto prop = carried_prop_.lock();
    if(!prop)
        return;

    // Release prop and send it flying (e.g., set velocity or trigger projectile logic)
    float3 my_pos    = GetOwner()->GetTranslate();
    float3 throw_dir = normalize(target_pos - my_pos);

    // Unparent/detach prop
    carried_prop_.reset();

    // Trigger prop throw impulse logic here
    // prop->GetComponent<ComponentRigidBody>()->AddImpulse(throw_dir * throw_power);
}

void ComponentEnemyController::GUI()
{
    __super::GUI();

    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode(u8"Character Controller")) {
            ImGui::DragFloat(u8"移動速度 (Move Speed)", &move_speed_, 0.1f, 0.0f, 5.0f);

            if(IsCarryingProp()) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), u8"アイテム所持中: %s", GetCarriedProp()->GetName().data());
            }
            else {
                ImGui::Text(u8"アイテム未所持");
            }

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentEnemyController)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentEnemyController)
