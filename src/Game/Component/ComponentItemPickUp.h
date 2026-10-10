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

    void ResetRotation();

    // Called when player throws this item away
    void OnThrow();

    // Item (the Object) calls this from its OnHit() when it collides with "Ground"
    void OnLanded();

    bool IsHeld() const { return isHeld_; }

private:
    bool    isHeld_   = false;
    bool    isThrown_ = false;                 //!< true while the item is flying through the air
    bool    isGround_ = false;                 //!< true once the item's collision has actually touched the ground
    float3  velocity_ = {0.0f, 0.0f, 0.0f};    //!< current speed/direction while thrown
    Object* holder_;
    float   throwSpeed_ = 700.0f;
    float   upBoost_    = 20.0f;

    // stops the item once isGround_ has been set by OnLanded()
    void CheckGround();

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentItemPickUp, 4);
