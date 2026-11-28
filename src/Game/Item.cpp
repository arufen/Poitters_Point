//---------------------------------------------------------------------------
//!	@file	PoittersPoint_Item.cpp
//! @brief	PoittersPoint_Item
//---------------------------------------------------------------------------

#include "Item.h"
#include "StageConfig.h"
#include "MainStage.h"
#include "Component/ComponentItemPickUp.h"
#include "Component/ComponentCharacterStats.h"
#include "Component/ComponentArenaBounds.h"
#include "Component/ComponentKnockdown.h"
#include "Component/ComponentCatchGuard.h"
#include <System/Component/ComponentModel.h>
#include <System/Component/ComponentCollisionCapsule.h>
#include <string>

namespace PoittersPoint {

//! @brief 初期化 / Initialize
//! @return 初期化終了 / Initialization finished
bool Item::Init()
{
    Super::Init();

    //Collision (use capsule due to bug of using CollissionBox). Size is set by SetItemType
    //(コリジョン（CollisionBoxのバグのためカプセルを使用）。大きさはSetItemTypeで設定する)
    auto collision = AddComponent<ComponentCollisionCapsule>();
    collision->UseGravity(true);
    collision->SetCollisionGroup(ComponentCollision::CollisionGroup::ENEMY);

    //Position - placed by the spawner / 配置はスポナー側で行う
    float3 pos = {0.0f, 50.0f, 0.0f};
    SetTranslate(pos);

    //Component
    AddComponent<ComponentItemPickUp>();

    //Invisible wall, so the item can not fall off the stage(見えない壁。ステージから落ちないように)
    AddComponent<ComponentArenaBounds>();

    //Default type. The spawner calls SetItemType again with the real type(デフォルトの種類。スポナーが本当の種類でもう一度SetItemTypeを呼ぶ)
    SetItemType(StageConfig::ItemType::Cobblestone);

    return true;
}

ItemPtr Item::SetItemType(StageConfig::ItemType type)
{
    const auto& def = StageConfig::GetItemDef(type);

    type_       = type;
    weightType_ = def.weightType;
    weight_     = def.weight;
    size_       = def.objectScale;

    //Unique name like "Obsidian_3". The hand attach finds objects by NAME, so it must not be the same as others
    //(「Obsidian_3」のような固有の名前。手にくっつける処理は名前で探すので、他と同じ名前にしない)
    static int itemCount = 0;
    itemCount++;
    itemName_ = std::string(def.name) + "_" + std::to_string(itemCount);
    SetName(itemName_);

    //Swap the model(モデルを入れ替える)
    RemoveComponent<ComponentModel>();
    auto model = AddComponent<ComponentModel>(def.model);
    //modelScale 1.0 = engine default 0.1(modelScaleの1.0＝エンジンのデフォルト0.1)
    float  ms         = def.modelScale * StageConfig::MODEL_BASE_SCALE;
    float3 modelScale = {ms, ms, ms};
    model->SetScaleAxisXYZ(modelScale);

    //Object size(オブジェクトの大きさ)
    float3 size = {def.objectScale, def.objectScale, def.objectScale};
    SetScaleAxisXYZ(size);

    //Bounce and tumble after a throw, by weight. Chest and TNT get 0 bounces so they explode on the first landing
    //(投げた後のバウンドと回転。重さで決まる。チェストとTNTはバウンド0なので最初の着地で爆発する)
    if(auto pickup = GetComponent<ComponentItemPickUp>()) {
        const auto& bounce = StageConfig::GetBounceDef(def.weightType);
        int         count  = StageConfig::CanBounce(type) ? bounce.count : 0;
        pickup->SetBounce(count, bounce.keepUp, bounce.keepForward, StageConfig::BOUNCE_MIN_SPEED)->SetSpin(bounce.spinSpeed);

        //Model scale + facing fix from StageConfig. Set here (after the new model was added) so it applies to the new model
        //(StageConfigのモデルのスケールと向きの補正。新しいモデルを追加した後にここで設定するので、新しいモデルにかかる)
        pickup->SetModelTransform(ms, float3(def.rotX, def.rotY, def.rotZ));
    }

    //Collision size for this model(このモデルに合った当たり判定の大きさ)
    if(auto col = GetComponent<ComponentCollisionCapsule>()) {
        col->SetRadius(def.colRadius);
        col->SetHeight(def.colHeight);
        col->SetTranslate(float3(0.0f, def.colOffsetY, 0.0f));
    }

    return std::static_pointer_cast<Item>(shared_from_this());
}

int Item::GetDamage() const
{
    //Light about 1, Medium about 2, Heavy about 3-5 (at least 1)(軽い物は約1、普通は約2、重い物は約3〜5。最低1)
    int dmg = (int)(weight_ * 1.5f + 0.5f);
    if(dmg < 1)
        dmg = 1;
    return dmg;
}

// 当たり判定が行われたときに呼ばれる関数 / Called when collision occurs
void Item::OnHit(const ComponentCollision::HitInfo& hit_info)
{
    Object* other  = hit_info.hit_collision_->GetOwner();
    auto    name   = other->GetNameDefault();
    auto    pickup = GetComponent<ComponentItemPickUp>();

    if(name == "Ground") {
        //Tell the pickup component it landed, so it can stop moving(着地したことを拾うコンポーネントに伝えて、動きを止める)
        if(pickup)
            pickup->OnLanded();
    }
    else if(pickup && pickup->CanDealDamage()) {
        //Never hurt the one who threw it(投げた本人にはダメージを与えない)
        if(other != pickup->GetThrower()) {
            //Anyone with ComponentCharacterStats can be hurt. No name check, so Player, Npc0, Npc1... all work
            //(ComponentCharacterStatsを持っていれば誰でもダメージを受ける。名前は見ないので、Player・Npc0・Npc1...全部OK)
            if(auto stats = other->GetComponent<ComponentCharacterStats>()) {
                //[ADD] cha0003 POLICE: catch or guard from the front, asked BEFORE the damage
                //([追加] cha0003 POLICE：前からならキャッチかガード。ダメージの「前」に聞く)
                int  damage    = GetDamage();
                bool knockDown = true;
                if(auto guard = other->GetComponent<ComponentCatchGuard>()) {
                    auto result = guard->TryBlock(std::static_pointer_cast<Item>(shared_from_this()), *pickup);

                    //Caught: the item is in their hands now. No damage, no knockback(キャッチ：物はもう相手の手の中。ダメージもふっとびもなし)
                    if(result == ComponentCatchGuard::Result::Catch) {
                        Super::OnHit(hit_info);
                        return;
                    }

                    //Guarded: less damage (can be 0) and no fall-down. The item still bounces off
                    //(ガード：ダメージが減る(0もある)、倒れない。物は普通にはね返る)
                    if(result == ComponentCatchGuard::Result::Guard) {
                        damage    = guard->GetGuardDamage(damage);
                        knockDown = false;
                        if(damage <= 0)
                            pickup->OnHitCharacter();
                    }
                }

                //TakeDamage returns false if the target is already dead, so the item passes through(すでに死んでいたらfalseなので、アイテムはそのまま通り抜ける)
                if(stats->TakeDamage(damage)) {
                    //Knockback in the direction the item was flying (hit from the left = pushed right). Read it BEFORE
                    //OnHitCharacter(), which slows the item down. No sideways speed (dropped from above) = push away from the item
                    //(アイテムが飛んでいた方向にふっとぶ(左から当たる＝右へ飛ぶ)。アイテムを遅くするOnHitCharacter()の「前」に読む。
                    // 横の速さがない(上から落ちた)ならアイテムから離れる方向へ)
                    auto knock = knockDown ? other->GetComponent<ComponentKnockdown>() : nullptr;
                    if(knock) {
                        float3 dir = pickup->GetVelocity();
                        dir.y      = 0.0f;
                        if((float)length(dir) < 1.0f) {
                            dir   = other->GetTranslate() - GetTranslate();
                            dir.y = 0.0f;
                        }
                        knock->Knock(dir, weight_ * StageConfig::KNOCKBACK_DISTANCE_PER_WEIGHT);
                    }

                    pickup->OnHitCharacter();
                }
            }
        }
    }

    // 最後にこれを入れてください。ここでめり込みの解消を行っています。
    Super::OnHit(hit_info);
}
}    // namespace PoittersPoint
