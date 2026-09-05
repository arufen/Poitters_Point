#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>
#include "Game/Item.h"
#include "Game/Component/ComponentItemPickUp.h"

USING_PTR(ComponentPlayerPickUpTrigger);

class ComponentPlayerPickUpTrigger : public Component
{
public:
    BP_COMPONENT_DECL(ComponentPlayerPickUpTrigger, u8"アイテム取得トリガー");

    void Init() override;

    void Update() override;

    void GUI() override;

    // true if this player is currently holding an item
    bool HasItem() const { return !heldItem_.expired(); }

    // call this from ComponentStateThrow (or input handling) when player throws
    void ThrowItem();

    //アイテムを持ち上げる
    void PickUpItem();

    // returns the nearby pickup-able item, or nullptr if none in range
    PoittersPoint::ItemPtr GetNearbyItem();

private:
    // how close player needs to be to auto-pick-up an item
    float pickupRange_ = 15.0f;

    // the item this player is currently holding, if any
    PoittersPoint::ItemWeakPtr heldItem_;

    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentPlayerPickUpTrigger, 4);
