#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentItemPickUp);

class ComponentItemPickUp : public Component
{
public:
    BP_COMPONENT_DECL(ComponentItemPickUp, u8"アイテム拾い");

    void Init() override;

    void Update() override;

    void GUI() override;

    // Player calls this when it touches the item
    void OnPickedUp(Object* holder);

    // Called when player throws this item away
    void OnThrown();

    bool IsHeld() const { return isHeld_; }

private:
    bool    isHeld_ = false;
    Object* holder_;

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentItemPickUp, 4);
