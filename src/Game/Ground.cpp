//---------------------------------------------------------------------------
//!	@file	PoittersPoint_Player.cpp
//! @brief	PoittersPoint_Player
//---------------------------------------------------------------------------

#include "Ground.h"
#include "MainStage.h"

namespace PoittersPoint {
//! @brief 初期化
//! @return 初期化終了
bool Ground::Init()
{
    Super::Init();

    SetName("Ground");

    //auto ground = Scene::Object::Create<Object>("Ground");
    auto   model = AddComponent<ComponentModel>("data/Game/Models/Stage/Stage2.mv1");
    float3 size  = {2.0f, 2.0f, 2.0f};
    SetScaleAxisXYZ(size);
    AddComponent<ComponentCollisionModel>()->AttachToModel();

    return true;
}
}    // namespace PoittersPoint
