//---------------------------------------------------------------------------
//!	@file	ItemBox.cpp
//! @brief	Make any throwable item: cobblestone, ice, obsidian, wood plank, chest, TNT
//!         (投げられるアイテムを作る：丸石・氷・黒曜石・木材の板・チェスト・TNT)
//---------------------------------------------------------------------------

#include "ItemBox.h"
#include "Component/ComponentItemBox.h"

namespace PoittersPoint {

ItemPtr SpawnThrowable(StageConfig::ItemType type, const float3& pos)
{
    //Make a normal Item (collision, gravity, ComponentItemPickUp...), then give it its type (model, weight, size)
    //(普通のItemを作って(当たり判定・重力・拾う機能...)、種類を設定する(モデル・重さ・大きさ))
    auto item = Scene::Object::Create<Item>();
    if(!item)
        return nullptr;

    item->SetItemType(type);
    item->SetTranslate(pos);

    switch(type) {
    case StageConfig::ItemType::Chest:
        //Chest: explodes on landing and drops consumables only(チェスト：着地で爆発して、消費アイテムだけ出す)
        item->AddComponent<ComponentItemBox>()->SetContentCount(StageConfig::CHEST_MIN_CONTENT, StageConfig::CHEST_MAX_CONTENT)->SetConsumableChance(100);
        break;

    case StageConfig::ItemType::TNT:
        //TNT: same explosion as the chest, no items, but blast damage around it(TNT：チェストと同じ爆発。アイテムは出ないが、まわりに爆風ダメージ)
        item->AddComponent<ComponentItemBox>()->SetContentCount(0, 0)->SetBlast(StageConfig::TNT_BLAST_RADIUS, StageConfig::TNT_BLAST_DAMAGE);
        break;

    default:
        break;
    }

    return item;
}

}    // namespace PoittersPoint
