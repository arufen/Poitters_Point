#pragma once

#include <System/Component/Component.h>
#include <System/Scene.h>

USING_PTR(ComponentEnemyController);

class ComponentEnemyController : public Component
{
public:
    BP_COMPONENT_DECL(ComponentEnemyController, u8"敵アクションコンポーネント");

    void Init() override;
    void Update() override;
    void GUI() override;

    // Movement & Actions called by AI or Player Input
    void MoveTo(const float3& target_pos, float speed);
    void RotateTo(const float3& target_pos, float turn_speed);
    bool PickUpProp(ObjectPtr prop);
    void ThrowCarriedProp(const float3& target_pos, float throw_power);

    // State Queries
    bool      IsCarryingProp() const { return !carried_prop_.expired(); }
    ObjectPtr GetCarriedProp() const { return carried_prop_.lock(); }

private:
    ObjectWeakPtr carried_prop_;
    float         move_speed_ = 0.5f;

    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(move_speed_), CEREAL_NVP(carried_prop_));
        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentEnemyController, 1);
