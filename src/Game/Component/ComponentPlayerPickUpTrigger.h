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

    //Draws the "lifting" text and gauge above the head(頭の上に「持ち上げ中」の文字とゲージを描く)
    void PostDraw() override;

    void GUI() override;

    // true if this player is currently holding an item
    bool HasItem() const { return !heldItem_.expired(); }

    //True while the owner is lifting an item. Can not move or throw during this time(持ち上げている間true。この間は動けないし投げられない)
    bool IsLifting() const { return liftTimer_ > 0.0f; }

    // call this from ComponentStateThrow (or input handling) when player throws
    //Does nothing while lifting(持ち上げ中は何もしない)
    void ThrowItem();

    //アイテムを持ち上げる
    void PickUpItem();

    // returns the nearby pickup-able item, or nullptr if none in range
    PoittersPoint::ItemPtr GetNearbyItem();

private:
    // how close player needs to be to auto-pick-up an item
    float pickupRange_ = 20.0f;

    // the item this player is currently holding, if any
    PoittersPoint::ItemWeakPtr heldItem_;

    //Lift time left and total, in seconds(持ち上げの残り時間と全体の時間(秒))
    float liftTimer_ = 0.0f;
    float liftTotal_ = 0.0f;

    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentPlayerPickUpTrigger, 4);
