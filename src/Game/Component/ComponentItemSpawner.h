#pragma once

#include <System/Component/Component.h>
#include <System/Scene.h>
#include <vector>

namespace PoittersPoint {

USING_PTR(ComponentItemSpawner);

class ComponentItemSpawner : public Component
{
public:
    BP_COMPONENT_DECL(ComponentItemSpawner, u8"Item Spawnerコンポーネント");

    ComponentItemSpawner()          = default;
    virtual ~ComponentItemSpawner() = default;

    virtual void GUI() override;

    //! @brief Spawns an item at a random position within the defined radius/area
    void SpawnItemAtRandomPosition();

    CEREAL_SAVELOAD(arc, ver)
    {
        arc(cereal::make_nvp("spawn_chance", spawn_chance_),
            cereal::make_nvp("spawn_area_min", spawn_area_min_),
            cereal::make_nvp("spawn_area_max", spawn_area_max_),
            cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }

private:
    float  spawn_chance_   = 0.75f;                       //!< Overall chance to spawn (0.0 to 1.0 = 75%)
    float3 spawn_area_min_ = {-100.0f, 0.0f, -100.0f};    //!< Minimum X, Y, Z boundary
    float3 spawn_area_max_ = {100.0f, 0.0f, 100.0f};      //!< Maximum X, Y, Z boundary
};

}    // namespace PoittersPoint

CEREAL_REGISTER_TYPE(PoittersPoint::ComponentItemSpawner)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, PoittersPoint::ComponentItemSpawner)
