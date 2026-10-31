//---------------------------------------------------------------------------
//!	@file	Player.cpp
//! @brief	Player
//---------------------------------------------------------------------------

#include "Player.h"
#include "MainStage.h"
#include "Component/ComponentCameraController.h"
#include "Component/ComponentCharacterStats.h"
#include "Component/ComponentPlayerState.h"
#include "Component/ComponentPlayerPickUpTrigger.h"
#include "Component/ComponentStateIdleWalk.h"
#include <cstring>

namespace PoittersPoint {
//! @brief 初期化 / Initialize
//! @return 初期化終了 / Initialization finished
bool Player::Init()
{
    Super::Init();

    SetName("Player");

    AddComponent<ComponentModel>("data/Game/Models/Player/Salaryman.mv1")
        ->SetAnimation({
            { "idle",          "data/Game/Models/Player/Anims/Idle.mv1", 0, 1.0f}, // Idle
            { "walk",       "data/Game/Models/Player/Anims/Walking.mv1", 0, 3.3f}, // Walk
            {"throw", "data/Game/Models/Player/Anims/Carry_Walking.mv1", 1, 1.0f}  // Throw/Carry related
    });

    SetTranslate({0, 5, 0});

    //Random stats: HP 10-20, strength 0.8-1.3(ランダムなステータス：HP10〜20、筋力0.8〜1.3)
    AddComponent<ComponentCharacterStats>()->SetMaxHP(10 + GetRand(10))->SetStrength(0.8f + (GetRand(5) * 0.1f))->SetSpeed(2.0f);

    AddComponent<ComponentPlayerState>();
    AddComponent<ComponentCameraController>();

    auto col = AddComponent<ComponentCollisionCapsule>()    //
                   ->SetRadius(3.0f)
                   ->SetHeight(13.0f);

    col->UseGravity();
    col->SetCollisionGroup(ComponentCollision::CollisionGroup::PLAYER);

    AddComponent<ComponentPlayerPickUpTrigger>();

    //UI
    UI_key_e = LoadGraph("data/Game/UI/E_Key.png");

    return true;
}

void Player::Update()
{
    Super::Update();

    auto pickUpTrigger = GetComponent<ComponentPlayerPickUpTrigger>();
    if(!pickUpTrigger)
        return;

    //Can not move while lifting an item. Turn the walk component off and on(アイテムを持ち上げている間は動けない。歩くコンポーネントをオフ/オンする)
    bool lifting = pickUpTrigger->IsLifting();
    if(auto walk = GetComponent<ComponentStateIdleWalk>())
        walk->SetStatus(Component::StatusBit::Enable, !lifting);

    if(lifting) {
        //Stand still until the lift is done(持ち上げが終わるまで立ち止まる)
        if(auto mdl = GetComponent<ComponentModel>())
            mdl->PlayAnimationNoSame("idle", true);

        return;
    }

    //E key: throw if holding an item, otherwise pick up(Eキー：持っていれば投げる、持っていなければ拾う)
    if(!Input::IsKeyOn(KEY_INPUT_E))
        return;

    if(pickUpTrigger->HasItem())
        pickUpTrigger->ThrowItem();
    else
        pickUpTrigger->PickUpItem();
}

void Player::TakeDamage(int dmg)
{
    // Damage handling for Poitters Point style HP / ダメージ処理
    if(auto stats = GetComponent<ComponentCharacterStats>())
        stats->TakeDamage(dmg);
}

void Player::PostDraw()
{
    //Always show who the player is(いつもプレイヤーが誰か分かるように表示する)
    DrawPlayerMarker();
    DrawPickUpIcon();
    DrawHpBar();
}

void Player::DrawPickUpIcon()
{
    auto pickUpTrigger = GetComponent<ComponentPlayerPickUpTrigger>();
    if(!pickUpTrigger)
        return;

    auto item = pickUpTrigger->GetNearbyItem();
    if(!item)
        return;

    //Item world position -> screen position, so the icon floats right above the item(アイテムのワールド座標を画面座標に変換して、アイテムの真上にアイコンを出す)
    float3        pos        = item->GetTranslate();
    DxLib::VECTOR world_pos  = {pos.x, pos.y + 5.0f, pos.z};
    DxLib::VECTOR screen_pos = ConvWorldPosToScreenPos(world_pos);

    DrawRotaGraphF(screen_pos.x, screen_pos.y, 0.05f, 0.0f, UI_key_e, 1);
}

void Player::DrawHpBar()
{
    auto stats = GetComponent<ComponentCharacterStats>();
    if(!stats)
        return;

    //Bar position and size in pixels(バーの位置と大きさ(ピクセル))
    int x = 20;
    int y = 20;
    int w = 200;
    int h = 20;

    DrawBox(x, y, x + w, y + h, GetColor(50, 50, 50), TRUE);                                //Background(背景)
    DrawBox(x, y, x + (int)(w * stats->GetHPRatio()), y + h, GetColor(255, 0, 0), TRUE);    //HP(HP)
    DrawBox(x, y, x + w, y + h, GetColor(255, 255, 255), FALSE);                            //Border(枠)

    //DrawFormatString works like printf, so no sprintf buffer is needed(DrawFormatStringはprintfと同じ書き方なので、sprintfのバッファは不要)
    DrawFormatString(x + w + 10, y + 3, GetColor(255, 255, 255), "HP: %d/%d", stats->GetHP(), stats->GetMaxHP());
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

    int x = (int)screen_pos.x;
    int y = (int)screen_pos.y;

    unsigned int markColor    = GetColor(255, 220, 0);    //Yellow(黄色)
    unsigned int outlineColor = GetColor(0, 0, 0);        //Black outline so it is easy to read(読みやすいように黒い縁取り)

    //Triangle that points down at the head. The tip is at (x, y)(頭を指す下向きの三角形。先端が(x, y))
    DrawTriangle(x, y + 1, x - 10, y - 15, x + 10, y - 15, outlineColor, TRUE);
    DrawTriangle(x, y - 2, x - 7, y - 14, x + 7, y - 14, markColor, TRUE);

    //Text above the triangle, centered(三角形の上に文字を中央ぞろえで描く)
    const char* text = "PLAYER";
    int         tx   = x - GetDrawStringWidth(text, (int)strlen(text)) / 2;
    int         ty   = y - 34;

    //Draw 4 black copies around the text to make an outline(まわりに黒い文字を4つ描いて縁取りにする)
    DrawString(tx - 1, ty, text, outlineColor);
    DrawString(tx + 1, ty, text, outlineColor);
    DrawString(tx, ty - 1, text, outlineColor);
    DrawString(tx, ty + 1, text, outlineColor);
    DrawString(tx, ty, text, markColor);
}

}    // namespace PoittersPoint
