//---------------------------------------------------------------------------
//!	@file	TutorialX_Player.cpp
//! @brief	TutorialX_Player
//---------------------------------------------------------------------------

#include "Player.h"
#include "MainStage.h"
#include "Component/ComponentStateIdleWalk.h"
#include "Component/ComponentCameraController.h"
#include "Component/ComponentPlayerState.h"
#include "Component/ComponentPlayerPickUpTrigger.h"

namespace PoittersPoint {
//! @brief 初期化
//! @return 初期化終了
bool Player::Init()
{
    Super::Init();

    SetName("Player");

    //auto player = Scene::Object::Create<Object>("Player");
    AddComponent<ComponentModel>("data/Game/Models/Player/Salaryman.mv1")
        ->SetAnimation({
            { "idle",          "data/Game/Models/Player/Anims/Idle.mv1", 0, 1.0f}, // Idle
            { "walk",       "data/Game/Models/Player/Anims/Walking.mv1", 0, 1.0f}, // Jump
            {"throw", "data/Game/Models/Player/Anims/Carry_Walking.mv1", 1, 1.0f}  // Throw
    });

    SetTranslate({0, 5, 0});

    AddComponent<ComponentPlayerState>();

    AddComponent<ComponentCameraController>();

    auto col = AddComponent<ComponentCollisionCapsule>()    //
                   ->SetRadius(3.0f)
                   ->SetHeight(13.0f);

    col->UseGravity();
    col->SetCollisionGroup(ComponentCollision::CollisionGroup::PLAYER);

    //Component
    AddComponent<ComponentPlayerPickUpTrigger>();

    //UI
    UI_key_e = LoadGraph("data/Game/UI/E_Key.png");

    return true;
}

void Player::Update()
{
    Super::Update();

    if(auto pickUpTrigger = GetComponent<ComponentPlayerPickUpTrigger>()) {
        if(Input::IsKeyOn(KEY_INPUT_E)) {
            if(pickUpTrigger->HasItem()) {
                pickUpTrigger->ThrowItem();
            }
            else {
                pickUpTrigger->PickUpItem();
            }
        }
    }
}

void Player::PostDraw()
{
    auto pickUpTrigger = GetComponent<ComponentPlayerPickUpTrigger>();
    if(auto item = pickUpTrigger->GetNearbyItem()) {
        // item's world position -> screen position, so the icon
        // floats right above the item on screen
        const float3  up_len     = {0.0f, 5.0f, 0.0f};
        DxLib::VECTOR world_pos  = {item->GetTranslate().x + up_len.x, item->GetTranslate().y + up_len.y, item->GetTranslate().z + up_len.z};
        DxLib::VECTOR screen_pos = ConvWorldPosToScreenPos(world_pos);

        DrawRotaGraphF(screen_pos.x, screen_pos.y, 0.05f, 0.0f, UI_key_e, 1);
    }
}

}    // namespace PoittersPoint
