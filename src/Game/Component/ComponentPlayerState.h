#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentPlayerState);

class ComponentPlayerState : public Component
{
public:
    BP_COMPONENT_DECL(ComponentPlayerState, u8"投げる");

    void Init() override;

    void Update() override;

    void GUI() override;

private:
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentPlayerState, 4);
