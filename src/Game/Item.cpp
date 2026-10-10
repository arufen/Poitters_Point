//---------------------------------------------------------------------------
//!	@file	PoittersPoint_Player.cpp
//! @brief	PoittersPoint_Player
//---------------------------------------------------------------------------

#include "Item.h"
#include "MainStage.h"
#include <System/Component/ComponentCollisionBox.h>
#include "Component/ComponentItemPickUp.h"

namespace PoittersPoint {
//! @brief 初期化
//! @return 初期化終了
bool Item::Init()
{
    Super::Init();

    SetName("Item");

    //auto Item = Scene::Object::Create<Object>("Item");
    auto model = AddComponent<ComponentModel>("data/Game/Models/Test/Box2.mv1");

    //Collision (use capsule due to bug of using CollissionBox)
    // //コリジョン（CollisionBoxのバグのためカプセルを使用）
    auto collision = AddComponent<ComponentCollisionCapsule>();
    collision->SetHeight(26.0f);
    collision->SetRadius(10.0f);
    collision->SetTranslate(float3(0.0f, -10.0f, 0.0f));
    collision->UseGravity(true);
    collision->SetCollisionGroup(ComponentCollision::CollisionGroup::ENEMY);

    //Position
    float3 pos = {0.0f, 50.0f, 0.0f};
    SetTranslate(pos);
    float3 size = {0.5f, 0.5f, 0.5f};
    SetScaleAxisXYZ(size);

    //Component
    AddComponent<ComponentItemPickUp>();

    return true;
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
