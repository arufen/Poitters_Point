#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>
#include "Game/StageConfig.h"

USING_PTR(ComponentSpawnCarrier);

//The floating thing that brings a new item (like Poy Poy). Flies in from outside the arena, spins over the spot,
//drops the item, then flies away and deletes itself. A marker on the ground shows where the item will land.
//(新しいアイテムを運んでくる浮いている物(ポイポイ風)。アリーナの外から飛んできて、場所の上で回って、
// アイテムを落としてから飛び去って自分を消す。地面のマーカーでアイテムが落ちる場所が分かる)
class ComponentSpawnCarrier : public Component
{
public:
    BP_COMPONENT_DECL(ComponentSpawnCarrier, u8"アイテム運び役");

    //Make a carrier object that will bring this item type to the target spot(この種類のアイテムを目標の場所に運ぶ運び役を作る)
    static ComponentSpawnCarrierPtr Spawn(PoittersPoint::StageConfig::ItemType type, const float3& target);

    void Init() override;

    void Update() override;

    void Draw() override;

    void GUI() override;

    //The item it dropped. Empty until the drop happens(落としたアイテム。落とすまでは空)
    PoittersPoint::ItemWeakPtr GetSpawnedItem() const { return spawnedItem_; }

    //Spot on the ground where the item will land(アイテムが落ちる地面の場所)
    const float3& GetTarget() const { return target_; }

    //True after the item was dropped(アイテムを落としたらtrue)
    bool HasDropped() const { return state_ == State::Leave || state_ == State::Done; }

private:
    enum class State
    {
        FlyIn,        //Flying to the spot(場所へ飛んでいる)
        SpawnAnim,    //Spinning over the spot(場所の上で回っている)
        Leave,        //Item dropped, flying away(アイテムを落として飛び去っている)
        Done,         //Deleted(消えた)
    };

    //Drop the item under the carrier(運び役の下にアイテムを落とす)
    void DropItem();

    PoittersPoint::StageConfig::ItemType type_{};
    PoittersPoint::ItemWeakPtr           spawnedItem_;

    State  state_  = State::FlyIn;
    float  timer_  = 0.0f;
    float  spin_   = 0.0f;                  //Current spin angle in degrees(今の回転角度(度))
    float3 start_  = {0.0f, 0.0f, 0.0f};    //Where it comes from(どこから来るか)
    float3 hover_  = {0.0f, 0.0f, 0.0f};    //Point over the target(目標の上の点)
    float3 leave_  = {0.0f, 0.0f, 0.0f};    //Where it flies away to(どこへ飛び去るか)
    float3 target_ = {0.0f, 0.0f, 0.0f};    //Spot on the ground(地面の場所)

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentSpawnCarrier, 4);
