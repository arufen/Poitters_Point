//---------------------------------------------------------------------------
//!	@file	PoittersPoint_Item.h
//! @brief	PoittersPoint_Item
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>
#include <string>

namespace PoittersPoint {

//Forward declaration. The full list is in StageConfig.h(前方宣言。中身はStageConfig.hにある)
namespace StageConfig {
enum class ItemType : int;
}

USING_PTR(Item);
class Item : public Object
{
public:
    BP_OBJECT_DECL(Item, "PoittersPoint::Item");

    //! @brief 初期化 / Initialize
    //! @return 初期化終了 / Initialization finished
    bool Init() override;

    // 当たり判定が行われたときに呼ばれる関数 / Called when collision occurs
    void OnHit(const ComponentCollision::HitInfo& hit_info) override;

    // Item weight type / アイテムの重さ種別
    enum class WeightType
    {
        Light,     // 軽い / Light
        Medium,    // 普通 / Medium
        Heavy      // 重い / Heavy
    };

    //Set what this item is (model, weight, size, collision all come from StageConfig.h). Call right after Create, it is chainable
    //Init() already sets Cobblestone, so an Item made without this is a cobblestone
    //(このアイテムが何かを決める(モデル・重さ・大きさ・当たり判定はStageConfig.hから)。Createの直後に呼ぶ。つなげて呼べる
    // Init()でもう丸石にしているので、これを呼ばずに作ったItemは丸石になる)
    ItemPtr SetItemType(StageConfig::ItemType type);

    StageConfig::ItemType GetItemType() const { return type_; }
    WeightType            GetWeightType() const { return weightType_; }
    float                 GetWeight() const { return weight_; }
    //Object size (object scale). Used to scale the lifting animation(オブジェクトの大きさ(オブジェクトのスケール)。持ち上げアニメーションの速さに使う)
    float              GetSize() const { return size_; }
    const std::string& GetItemName() const { return itemName_; }

    //Damage this item deals when thrown at a character. Heavier = more damage(投げて当たった時のダメージ。重いほど大きい)
    int GetDamage() const;

private:
    StageConfig::ItemType type_{};    //0 = Cobblestone(0は丸石)
    WeightType            weightType_ = WeightType::Medium;
    float                 weight_     = 1.0f;
    float                 size_       = 0.5f;    //Object scale set by SetItemType(SetItemTypeで設定したオブジェクトのスケール)
    std::string           itemName_;
};
}    // namespace PoittersPoint
