//---------------------------------------------------------------------------
//!	@file	PoittersPoint_Player.cpp
//! @brief	PoittersPoint_Player
//---------------------------------------------------------------------------

#include "Item.h"
#include "MainStage.h"
#include <System/Component/ComponentCollisionBox.h>

namespace PoittersPoint {
//! @brief 初期化
//! @return 初期化終了
bool Item::Init()
{
    Super::Init();

    SetName("Item");

    //auto Item = Scene::Object::Create<Object>("Item");
    auto model = AddComponent<ComponentModel>("data/Game/Models/Test/Box2.mv1");

    //Collision
    auto   collision = AddComponent<ComponentCollisionBox>();
    float3 colSize   = {10.0f, 10.0f, 10.0f};
    collision->SetSize(colSize);
    collision->UseGravity(true);
    collision->SetCollisionGroup(ComponentCollision::CollisionGroup::ENEMY);

    //Position
    float3 pos = {0.0f, 50.0f, 0.0f};
    SetTranslate(pos);

    return true;
}
}    // namespace PoittersPoint
