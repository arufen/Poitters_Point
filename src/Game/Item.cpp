//---------------------------------------------------------------------------
//!	@file	PoittersPoint_Player.cpp
//! @brief	PoittersPoint_Player
//---------------------------------------------------------------------------

#include "Item.h"
#include "MainStage.h"
#include "Component/ComponentItemPickUp.h"
#include "Component/ComponentCharacterStats.h"
//! @brief 初期化 / Initialize
//! @return 初期化終了 / Initialization finished
namespace PoittersPoint {

bool Item::Init()
{
    Super::Init();

    SetName("Item");

    //auto Item = Scene::Object::Create<Object>("Item");
    auto model = AddComponent<ComponentModel>("data/Game/Models/Object/ThorwRock.mv1");

    // Randomize item stats / アイテムごとに固有のステータスをランダム化
    RandomizeStats();

    //Collision (use capsule due to bug of using CollissionBox)
    // //コリジョン（CollisionBoxのバグのためカプセルを使用）
    auto collision = AddComponent<ComponentCollisionCapsule>();
    collision->SetHeight(26.0f);
    collision->SetRadius(10.0f);
    collision->SetTranslate(float3(0.0f, -10.0f, 0.0f));
    collision->UseGravity(true);
    collision->SetCollisionGroup(ComponentCollision::CollisionGroup::ENEMY);

    //Position - will be placed by spawner / 配置はスポナー側で行う想定
    float3 pos = {0.0f, 50.0f, 0.0f};
    SetTranslate(pos);
    // Scale based on weight/status (smaller or bigger depending on status) / 重さに応じて大きさを調整（モデル未実装のためまずはスケールで表現）
    float scaleFactor = 0.4f;
    if(weightType_ == WeightType::Light)
        scaleFactor = 0.3f + (GetRand(10) * 0.01f);
    else if(weightType_ == WeightType::Medium)
        scaleFactor = 0.5f + (GetRand(10) * 0.01f);
    else if(weightType_ == WeightType::Heavy)
        scaleFactor = 0.7f + (GetRand(15) * 0.01f);
    float3 size = {scaleFactor, scaleFactor, scaleFactor};
    SetScaleAxisXYZ(size);

    //Component
    AddComponent<ComponentItemPickUp>();

    return true;
}

void Item::RandomizeStats()
{
    // Randomize weight and give unique name/identity / ランダムに重さを決定し固有の名前を付与
    int r = GetRand(99);
    if(r < 30) {
        // Light / 軽い
        weightType_ = WeightType::Light;
        weight_     = 0.5f + (GetRand(40) * 0.01f);
        itemName_   = "LightItem";
    }
    else if(r < 75) {
        // Medium / 普通
        weightType_ = WeightType::Medium;
        weight_     = 1.0f + (GetRand(60) * 0.01f);
        itemName_   = "MediumItem";
    }
    else {
        // Heavy / 重い
        weightType_ = WeightType::Heavy;
        weight_     = 2.0f + (GetRand(100) * 0.01f);
        itemName_   = "HeavyItem";
    }
    // Append unique number / 固有番号を追加
    static int itemCount = 0;
    itemCount++;
    itemName_ += "_" + std::to_string(itemCount);
    SetName(itemName_);
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
                //TakeDamage returns false if the target is already dead, so the item passes through(すでに死んでいたらfalseなので、アイテムはそのまま通り抜ける)
                if(stats->TakeDamage(GetDamage()))
                    pickup->OnHitCharacter();
            }
        }
    }

    // 最後にこれを入れてください。ここでめり込みの解消を行っています。
    Super::OnHit(hit_info);
}
}    // namespace PoittersPoint
