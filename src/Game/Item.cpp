//---------------------------------------------------------------------------
//!	@file	PoittersPoint_Player.cpp
//! @brief	PoittersPoint_Player
//---------------------------------------------------------------------------

#include "Item.h"
#include "MainStage.h"
#include "Component/ComponentItemPickUp.h"

namespace PoittersPoint {
//! @brief 初期化 / Initialize
//! @return 初期化終了 / Initialization finished
bool Item::Init()
{
    Super::Init();

    SetName("Item");

    //auto Item = Scene::Object::Create<Object>("Item");
    auto model = AddComponent<ComponentModel>("data/Game/Models/Test/Box2.mv1");

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

// 当たり判定が行われたときに呼ばれる関数
void Item::OnHit(const ComponentCollision::HitInfo& hit_info)
{
    auto name = hit_info.hit_collision_->GetOwner()->GetNameDefault();
    if(name == "Ground") {
        // tell the pickup component it landed, so it can stop moving
        if(auto pickup = GetComponent<ComponentItemPickUp>())
            pickup->OnLanded();
    }

    // 最後にこれを入れてください。ここでめり込みの解消を行っています。
    Super::OnHit(hit_info);
}
}    // namespace PoittersPoint
