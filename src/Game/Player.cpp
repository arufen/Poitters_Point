//---------------------------------------------------------------------------
//!	@file	TutorialX_Player.cpp
//! @brief	TutorialX_Player
//---------------------------------------------------------------------------

#include "Player.h"
#include "MainStage.h"
#include "Component/ComponentCameraController.h"
#include "Component/ComponentPlayerState.h"
#include "Component/ComponentPlayerPickUpTrigger.h"
#include "Game/Component/ComponentStateIdleWalk.h"
#include <cstring>

namespace PoittersPoint {
//! @brief 初期化
//! @return 初期化終了
bool Player::Init()
{
    Super::Init();

    SetName("Player");

    auto player = Scene::Object::Create<Object>("Player");
    AddComponent<ComponentModel>("data/Game/Models/Player/Salaryman.mv1")
        ->SetAnimation({
            {      "idle",          "data/Game/Models/Player/Anims/Idle.mv1", 0, 1.0f}, // Idle(mixamo)
            {      "walk",       "data/Game/Models/Player/Anims/Walking.mv1", 0, 1.0f}, // Walk(mixamo)
            //{"Carry_start", "data/Game/Models/Player/OR_Anims/ORCS2.mv1", 0, 1.0f}, // 持とうとする(Carry_start.mv1 idx1/idx2にmixamo実モーションあり)
            //{"Carry_start2","data/Game/Models/Player/OR_Anims/ORCS2.mv1", 1,1.0f}, // 持ち上げて保持状態にうつるまで(idx3はキーフレーム空・idx1のみ実モーションあり)
            {"Carry_walk", "data/Game/Models/Player/OR_Anims/Carry_walk.mv1", 3, 1.0f}, // 持ちながら歩く(実ファイル名は Carry_walk.mv1)保持状態を保
            //{"Carry_throw", "data/Game/Models/Player/OR_Anims/Carry_Throw.mv1", 1, 1.0f}, // (登録のみ・未使用。実投げモーションは"throw"/Snatch.mv1を使用)
            {     "throw",        "data/Game/Models/Player/Anims/Snatch.mv1", 0, 1.0f}, // 投げる(mixamo Snatch:持ち上げながら投げる)
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

    auto pickUpTrigger = GetComponent<ComponentPlayerPickUpTrigger>();
    auto model         = GetComponent<ComponentModel>();
    auto idleWalk      = GetComponent<ComponentStateIdleWalk>();

    if(pickUpTrigger && model) {
        //Eキー: 拾う / 投げる
        if(Input::IsKeyOn(KEY_INPUT_E)) {
            if(pickUpTrigger->HasItem()) {
                //投げアニメ再生中は再入力を無視(終了後に拾い直しできる)
                if(carryState_ != CarryAnimState::Throw) {
                    carryState_    = CarryAnimState::Throw;
                    throwReleased_ = false;
                    //Carry_Throw.mv1 の idx1 は start/lift と同じ運びモーションで投げモーションではないため、
                    //実際に腕を伸ばして投げる mixamo の Snatch ("throw") を再生する
                    model->PlayAnimation("throw");
                }
            }
            else if(carryState_ != CarryAnimState::Throw) {
                pickUpTrigger->PickUpItem();
                //実際に拾えた場合だけ持ち上げアニメーションを開始
                if(pickUpTrigger->HasItem()) {
                    carryState_ = CarryAnimState::Start;
                    model->PlayAnimation("Carry_start");
                }
            }
        }

        bool isMoving = (Input::IsKey(KEY_INPUT_W) || Input::IsKey(KEY_INPUT_A) || Input::IsKey(KEY_INPUT_S) || Input::IsKey(KEY_INPUT_D));

        if(pickUpTrigger->HasItem()) {
            //持ち中のアニメーション進行
            //非ループアニメは終了すると IsPlaying() が false になり、最後のポーズを保持する
            switch(carryState_) {
            case CarryAnimState::Start:
                //Carry_start 終了 → Carry_start2 で持ち上げを続ける
                if(!model->IsPlaying()) {
                    carryState_ = CarryAnimState::Start2;
                    model->PlayAnimation("Carry_start2");
                }
                break;

            case CarryAnimState::Start2:
                //移動開始 → 持ちながら歩くアニメーション(歩き中はループ)
                if(isMoving) {
                    carryState_ = CarryAnimState::Walk;
                    model->PlayAnimation("Carry_walk", true);
                }
                break;

            case CarryAnimState::Walk:
                //移動停止 → 持ち上げて保持するポーズへ戻る
                if(!isMoving) {
                    carryState_ = CarryAnimState::Start2;
                    model->PlayAnimation("Carry_start2");
                }
                break;

            case CarryAnimState::Throw:
                //投げアニメの腕伸展フレーム(リリース点)でアイテムを手から離す
                if(!throwReleased_ && model->GetAnimationTime() >= throwReleaseTime_) {
                    pickUpTrigger->ThrowItem();
                    throwReleased_ = true;
                }
                break;

            default:
                break;
            }
        }
        else {
            //投げ終わり → idle へ戻る
            if(carryState_ == CarryAnimState::Throw && !model->IsPlaying()) {
                carryState_ = CarryAnimState::None;
                model->PlayAnimation("idle");
            }
        }

        //持ち中・投げ中は idle/walk アニメーションの再生を止めて Carry アニメを上書きさせない
        if(idleWalk) {
            bool carrying = pickUpTrigger->HasItem() || carryState_ == CarryAnimState::Throw;
            idleWalk->SetAnimationsEnabled(!carrying);
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
