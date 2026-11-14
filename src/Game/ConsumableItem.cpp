//---------------------------------------------------------------------------
//!	@file	ConsumableItem.cpp
//! @brief	ConsumableItem (apple, golden apple, speed potion)
//---------------------------------------------------------------------------

#include "ConsumableItem.h"
#include "Component/ComponentConsumableItem.h"
#include <System/Component/ComponentModel.h>
#include <System/Component/ComponentCollisionCapsule.h>

namespace PoittersPoint {
bool ConsumableItem::Init()
{
    Super::Init();

    //Type order for spawning. Apple x3, Speed Potion x2, Golden Apple x1 per 6 items, so all 3 types show up and Golden is rare.
    //Replace with GetRand() later if you want it fully random.
    //(スポーンする種類の順番。6個ごとにリンゴ3、スピード2、金のリンゴ1。3種類とも必ず出て、金のリンゴはレア。完全ランダムにしたい時はあとでGetRand()にする)
    using ItemType                      = ComponentConsumableItem::ItemType;
    static const ItemType SPAWN_CYCLE[] = {
        ItemType::Apple,
        ItemType::Apple,
        ItemType::SpeedPotion,
        ItemType::GoldenApple,
        ItemType::Apple,
        ItemType::SpeedPotion,
    };
    static int spawnCount = 0;
    ItemType   type       = SPAWN_CYCLE[spawnCount % 6];
    spawnCount++;

    //Name like "Apple", unique number is added by the engine(「Apple」のような名前。番号はエンジンが自動でつける)
    SetName(ComponentConsumableItem::GetItemName(type));

    //Model file and model size both depend on the type, so each model can be sized on its own
    //(モデルファイルと大きさは両方とも種類で決まる。だからモデルごとに大きさを別々に変えられる)
    auto   model = AddComponent<ComponentModel>(ComponentConsumableItem::GetModelPath(type));
    float  scale = ComponentConsumableItem::GetModelScale(type);
    float3 size  = {scale, scale, scale};
    model->SetScaleAxisXYZ(size);

    //Small capsule with gravity, so it falls and lands on the ground. Overlap with characters and thrown items, so nobody gets pushed
    //(重力つきの小さいカプセル。落ちて地面に乗る。キャラや投げたアイテムとはオーバーラップさせて、押し合わないようにする)
    u32 overlapGroups = (u32)ComponentCollision::CollisionGroup::PLAYER     //
                        | (u32)ComponentCollision::CollisionGroup::ENEMY    //
                        | (u32)ComponentCollision::CollisionGroup::ITEM;

    auto col = AddComponent<ComponentCollisionCapsule>()    //
                   ->SetTranslate(float3(0.0f, -7.0f, 0.0f))
                   ->SetRadius(8.2f)
                   ->SetHeight(10.0f)
                   ->SetCollisionGroup(ComponentCollision::CollisionGroup::ITEM)
                   ->SetOverlapCollisionGroup(overlapGroups);
    col->UseGravity(true);

    //Position - MainStage places it. It starts high and falls(位置はMainStageが決める。高い所から落ちてくる)
    SetTranslate({0.0f, 50.0f, 0.0f});

    //The behavior: collect when a character comes close(動き：キャラが近づいたら取られる)
    AddComponent<ComponentConsumableItem>()->SetItemType(type);

    return true;
}
}    // namespace PoittersPoint
