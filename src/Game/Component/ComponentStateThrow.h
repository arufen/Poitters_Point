#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>
#include <Game/TutorialX_Bullet.h>

USING_PTR(ComponentStateThrow);

class ComponentStateThrow : public Component
{
public:
    BP_COMPONENT_DECL(ComponentStateThrow, u8"投げる");

    void Init() override;

    void Update() override;

    void GUI() override;

private:
    TutorialX::BulletWeakPtr bullet_;
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentStateThrow, 4);
