//---------------------------------------------------------------------------
//!	@file	TutorialX_Player.cpp
//! @brief	TutorialX_Player
//---------------------------------------------------------------------------

#include "Player.h"
#include "MainStage.h"
#include "Component/ComponentCameraController.h"
#include "Component/ComponentPlayerState.h"
#include "Component/ComponentPlayerPickUpTrigger.h"
#include <cstring>

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
            { "idle",    "data/Game/Models/Player/Anims/Idle.mv1", 0, 1.0f}, // Idle
            { "walk", "data/Game/Models/Player/Anims/Walking.mv1", 0, 1.0f}, // Jump
            {"throw",  "data/Game/Models/Player/Anims/Snatch.mv1", 0, 1.0f}  // Throw
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
    //Always show who the player is(いつもプレイヤーが誰か分かるように表示する)
    DrawPlayerMarker();

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

void Player::DrawPlayerMarker()
{
    //Head position in the world, then convert to the screen position(頭の位置をワールド座標で決めて、画面の位置に変換する)
    float3        pos        = GetTranslate();
    DxLib::VECTOR world_pos  = {pos.x, pos.y + markerHeight_, pos.z};
    DxLib::VECTOR screen_pos = ConvWorldPosToScreenPos(world_pos);

    //Do not draw when the player is behind the camera(カメラの後ろにいる時は描かない)
    if(screen_pos.z <= 0.0f || screen_pos.z >= 1.0f)
        return;

    const int x = (int)screen_pos.x;
    const int y = (int)screen_pos.y;

    const unsigned int markColor    = GetColor(255, 220, 0);    //Yellow(黄色)
    const unsigned int outlineColor = GetColor(0, 0, 0);        //Black outline so it is easy to read(読みやすいように黒い縁取り)

    //Triangle that points down at the head. The tip is at (x, y)(頭を指す下向きの三角形。先端が(x, y))
    DrawTriangle(x, y + 1, x - 10, y - 15, x + 10, y - 15, outlineColor, TRUE);
    DrawTriangle(x, y - 2, x - 7, y - 14, x + 7, y - 14, markColor, TRUE);

    //Text above the triangle, centered(三角形の上に文字を中央ぞろえで描く)
    const char* text  = "PLAYER";
    int         width = GetDrawStringWidth(text, (int)strlen(text));
    int         tx    = x - width / 2;
    int         ty    = y - 34;

    DrawString(tx - 1, ty, text, outlineColor);
    DrawString(tx + 1, ty, text, outlineColor);
    DrawString(tx, ty - 1, text, outlineColor);
    DrawString(tx, ty + 1, text, outlineColor);
    DrawString(tx, ty, text, markColor);
}

}    // namespace PoittersPoint
