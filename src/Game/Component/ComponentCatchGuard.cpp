//---------------------------------------------------------------------------
//!	@file	ComponentCatchGuard.cpp
//! @brief	[ADD] cha0003 警察官(POLICE): catch or guard items thrown at this character
//!         ([追加] cha0003 警察官(POLICE)：投げられた物をキャッチ・ガードする)
//---------------------------------------------------------------------------
#include "Game/Component/ComponentCatchGuard.h"
#include "Game/Component/ComponentItemPickUp.h"
#include "Game/Component/ComponentPlayerPickUpTrigger.h"
#include "Game/Component/ComponentNpcAction.h"
#include "Game/Component/ComponentMatch.h"
#include "Game/Item.h"
#include "Game/StageConfig.h"
#include <cmath>
#include <cstring>

namespace SC = PoittersPoint::StageConfig;

namespace {
//Own name, because DxLib already has "PI"(DxLibにもう「PI」があるので自分専用の名前)
const float GUARD_PI = 3.14159265f;

//How long the hands need to settle a caught item (a short "lift")(キャッチした物を手に収める時間(短い「持ち上げ」))
const float CATCH_HOLD_TIME = 0.2f;

//Text with a black outline, same look as "DOWN!"(黒いふちどりの文字。「DOWN!」と同じ見た目)
void DrawOutlined(int cx, int y, const char* text, unsigned int color)
{
    int w = GetDrawStringWidth(text, (int)strlen(text));
    int x = cx - w / 2;
    for(int dx = -1; dx <= 1; ++dx)
        for(int dy = -1; dy <= 1; ++dy)
            if(dx != 0 || dy != 0)
                DrawString(x + dx, y + dy, text, GetColor(0, 0, 0));
    DrawString(x, y, text, color);
}
}    // namespace

void ComponentCatchGuard::Init()
{
    Super::Init();
    cooldownTimer_ = 0.0f;
    pushVel_       = {0.0f, 0.0f, 0.0f};
    lastResult_    = Result::None;
    textTimer_     = 0.0f;
    catchThrower_  = nullptr;
    caughtNew_     = false;
}

void ComponentCatchGuard::Update()
{
    Super::Update();

    float delta = GetDeltaTime();

    if(cooldownTimer_ > 0.0f)
        cooldownTimer_ -= delta;
    if(textTimer_ > 0.0f)
        textTimer_ -= delta;

    //Small push back after a guard. x/z only, so gravity keeps working (same as ComponentKnockdown)
    //(ガード後の小さな押し戻し。x/zだけなので重力はそのまま動く(ComponentKnockdownと同じ))
    if((float)length(pushVel_) > 0.1f) {
        float3 pos  = GetOwner()->GetTranslate();
        pos.x      += pushVel_.x * delta;
        pos.z      += pushVel_.z * delta;
        GetOwner()->SetTranslate(pos);

        pushVel_ *= expf(-SC::KNOCKBACK_FRICTION * delta);
    }
}

bool ComponentCatchGuard::IsFromFront(const float3& itemVelocity) const
{
    //Where the item comes FROM = the opposite of where it flies(物が「来る」方向＝飛んでいる方向の反対)
    float3 from = {-(float)itemVelocity.x, 0.0f, -(float)itemVelocity.z};
    if((float)length(from) < 1.0f)
        return false;    //Dropped from straight above, no front/back(真上から落ちてきた。前も後ろもない)
    from = normalize(from);

    //The models look toward -Z (same as the throw direction in ComponentItemPickUp::OnThrow)
    //(モデルは-Zを向いている(ComponentItemPickUp::OnThrowの投げる方向と同じ))
    auto model = GetOwner()->GetComponent<ComponentModel>();
    if(!model)
        return false;
    float3 front = -model->GetWorldVectorAxisZ();
    front.y      = 0.0f;
    if((float)length(front) < 0.0001f)
        return false;
    front = normalize(front);

    return (float)dot(front, from) >= cosf(frontAngleDeg_ * (GUARD_PI / 180.0f));
}

bool ComponentCatchGuard::HandsFree() const
{
    auto trigger = GetOwner()->GetComponent<ComponentPlayerPickUpTrigger>();
    if(!trigger || trigger->HasItem() || trigger->IsLifting())
        return false;

    //In a throw animation the arms are busy, even after the item left the hand(投げアニメ中は、物が手から離れた後でも腕がふさがっている)
    if(auto action = GetOwner()->GetComponent<ComponentNpcAction>()) {
        if(action->IsThrowing())
            return false;
    }
    return true;
}

ComponentCatchGuard::Result ComponentCatchGuard::TryBlock(const PoittersPoint::ItemPtr& item, ComponentItemPickUp& pickup)
{
    if(!item || !GetStatus(StatusBit::Enable))
        return Result::None;

    //Down, K.O., countdown, match over = can not react(ダウン中・K.O.・カウントダウン・試合終了＝反応できない)
    if(!ComponentMatch::CanAct(GetOwner()))
        return Result::None;

    //Still recovering from the last catch/guard(前のキャッチ・ガードからまだ立ち直っていない)
    if(cooldownTimer_ > 0.0f)
        return Result::None;

    const float3 vel = pickup.GetVelocity();
    if(!IsFromFront(vel))
        return Result::None;

    const Object* thrower = pickup.GetThrower();

    //1. CATCH: free hands and not too heavy(1. キャッチ：手が空いていて、重すぎない)
    if(HandsFree() && item->GetWeight() <= catchMaxWeight_) {
        auto trigger = GetOwner()->GetComponent<ComponentPlayerPickUpTrigger>();
        if(trigger && trigger->CatchItem(item, CATCH_HOLD_TIME)) {
            catchThrower_  = thrower;
            caughtNew_     = true;
            cooldownTimer_ = cooldown_;
            lastResult_    = Result::Catch;
            textTimer_     = textTime_;
            catchCount_++;
            return Result::Catch;
        }
    }

    //2. GUARD: anything else from the front. A small push the way the item flew, no fall-down
    //(2. ガード：前から来たそれ以外の物。物が飛んでいた方向へ少し押されるだけで、倒れない)
    float3 dir = {(float)vel.x, 0.0f, (float)vel.z};
    if((float)length(dir) > 0.001f)
        dir = normalize(dir);
    pushVel_ = dir * (guardPushDist_ * item->GetWeight() * SC::KNOCKBACK_FRICTION);

    cooldownTimer_ = cooldown_;
    lastResult_    = Result::Guard;
    textTimer_     = textTime_;
    guardCount_++;
    return Result::Guard;
}

int ComponentCatchGuard::GetGuardDamage(int damage) const
{
    int dmg = (int)((float)damage * guardDamageScale_);
    return dmg < 0 ? 0 : dmg;
}

bool ComponentCatchGuard::TakeCatch(const Object*& outThrower)
{
    outThrower = catchThrower_;
    if(!caughtNew_)
        return false;

    caughtNew_    = false;
    catchThrower_ = nullptr;
    return true;
}

void ComponentCatchGuard::PostDraw()
{
    Super::PostDraw();

    if(textTimer_ <= 0.0f || lastResult_ == Result::None)
        return;

    //Head position in the world -> screen position, same as "DOWN!"(頭の位置のワールド座標 -> 画面座標。「DOWN!」と同じ)
    float3        pos        = GetOwner()->GetTranslate();
    DxLib::VECTOR world_pos  = {pos.x, pos.y + 22.0f, pos.z};
    DxLib::VECTOR screen_pos = ConvWorldPosToScreenPos(world_pos);

    //Do not draw when behind the camera(カメラの後ろなら描かない)
    if(screen_pos.z <= 0.0f || screen_pos.z >= 1.0f)
        return;

    //Float up a little while it shows(表示中に少し上へ浮かぶ)
    float t    = 1.0f - textTimer_ / textTime_;
    int   rise = (int)(t * 12.0f);

    if(lastResult_ == Result::Catch)
        DrawOutlined((int)screen_pos.x, (int)screen_pos.y - 10 - rise, "CATCH!", GetColor(255, 220, 40));
    else
        DrawOutlined((int)screen_pos.x, (int)screen_pos.y - 10 - rise, "GUARD!", GetColor(90, 170, 255));
}

void ComponentCatchGuard::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("CatchGuard")) {
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text("Catch: %d  Guard: %d", catchCount_, guardCount_);
            ImGui::Text(cooldownTimer_ > 0.0f ? "Cooldown %.2f s" : "Ready", cooldownTimer_);
            ImGui::DragFloat(u8"前の角度(Front deg)", &frontAngleDeg_, 1.0f, 0.0f, 180.0f);
            ImGui::DragFloat(u8"キャッチできる重さ(Catch max weight)", &catchMaxWeight_, 0.05f, 0.0f, 10.0f);
            ImGui::DragFloat(u8"ガード時のダメージ倍率(Guard dmg x)", &guardDamageScale_, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat(u8"ガード時の押される距離(Guard push)", &guardPushDist_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat(u8"次までの秒数(Cooldown)", &cooldown_, 0.01f, 0.0f, 5.0f);

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentCatchGuard)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentCatchGuard)
