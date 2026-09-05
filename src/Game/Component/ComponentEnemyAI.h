#pragma once

#include <System/Component/Component.h>
#include <System/Scene.h>

USING_PTR(ComponentEnemyAI);

class ComponentEnemyAI : public Component
{
public:
    enum class AIState
    {
        Idle,
        SeekProp,
        PickUpProp,
        AimAndThrow,
        Evade
    };

    BP_COMPONENT_DECL(ComponentEnemyAI, u8"敵AIコンポーネント");

    void Init() override;
    void Update() override;
    void GUI() override;

private:
    ObjectPtr FindNearestItem();
    ObjectPtr FindNearestPlayer();

    AIState current_state_ = AIState::Idle;

    float search_range_ = 300.0f;
    float pickup_range_ = 15.0f;

    ObjectWeakPtr target_prop_;
    ObjectWeakPtr target_player_;

    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(current_state_), CEREAL_NVP(search_range_), CEREAL_NVP(pickup_range_), CEREAL_NVP(target_prop_), CEREAL_NVP(target_player_));

        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentEnemyAI, 1);
