#include "Game/Component/ComponentPlayerPickUpTrigger.h"
#include "Game/Component/ComponentCharacterStats.h"
#include "Game/Component/ComponentStateIdleWalk.h"
#include "Game/MainStage.h"
#include <cstring>

namespace {
//Text shown while lifting. If it looks broken, check that DxLib UTF-8 mode and the file encoding match(持ち上げ中に出す文字。文字化けする時はDxLibのUTF-8設定とファイルの文字コードを合わせる)
const char* LIFT_TEXT = "持ち上げ中...";

//Draw centered text with a black outline(黒い縁取りつきで中央ぞろえの文字を描く)
void DrawOutlinedText(int centerX, int y, const char* text)
{
    unsigned int white   = GetColor(255, 255, 255);
    unsigned int outline = GetColor(0, 0, 0);

    int x = centerX - GetDrawStringWidth(text, (int)strlen(text)) / 2;

    DrawString(x - 1, y, text, outline);
    DrawString(x + 1, y, text, outline);
    DrawString(x, y - 1, text, outline);
    DrawString(x, y + 1, text, outline);
    DrawString(x, y, text, white);
}
}    // namespace

void ComponentPlayerPickUpTrigger::Init()
{
    Super::Init();
    // nothing to spawn here - this component just watches for nearby items
}

void ComponentPlayerPickUpTrigger::Update()
{
    Super::Update();

    //We lost the item, so stop lifting(アイテムを失ったので持ち上げを終わる)
    if(!HasItem()) {
        liftTimer_ = 0.0f;
        return;
    }

    //Count down the lift time(持ち上げ時間を減らす)
    if(liftTimer_ > 0.0f) {
        liftTimer_ -= GetDeltaTime();
        if(liftTimer_ < 0.0f)
            liftTimer_ = 0.0f;
    }
}

void ComponentPlayerPickUpTrigger::PostDraw()
{
    Super::PostDraw();

    if(!IsLifting())
        return;

    //Head position in the world -> screen position(頭の位置のワールド座標 -> 画面座標)
    float3        pos        = GetOwner()->GetTranslate();
    DxLib::VECTOR world_pos  = {pos.x, pos.y + 22.0f, pos.z};
    DxLib::VECTOR screen_pos = ConvWorldPosToScreenPos(world_pos);

    //Do not draw when behind the camera(カメラの後ろなら描かない)
    if(screen_pos.z <= 0.0f || screen_pos.z >= 1.0f)
        return;

    int x = (int)screen_pos.x;
    int y = (int)screen_pos.y;

    DrawOutlinedText(x, y + 6, LIFT_TEXT);

    //Gauge: fills up from left to right while lifting(ゲージ：持ち上げ中に左から右へ増える)
    float progress = 1.0f - (liftTotal_ > 0.0f ? liftTimer_ / liftTotal_ : 1.0f);
    int   gaugeW   = 50;
    int   gaugeH   = 6;
    int   gx       = x - gaugeW / 2;
    int   gy       = y + 26;

    DrawBox(gx - 1, gy - 1, gx + gaugeW + 1, gy + gaugeH + 1, GetColor(0, 0, 0), TRUE);
    DrawBox(gx, gy, gx + gaugeW, gy + gaugeH, GetColor(70, 70, 70), TRUE);
    DrawBox(gx, gy, gx + (int)(gaugeW * progress), gy + gaugeH, GetColor(255, 200, 0), TRUE);
}

void ComponentPlayerPickUpTrigger::ThrowItem()
{
    //Can not throw while lifting(持ち上げ中は投げられない)
    if(IsLifting())
        return;

    if(auto item = heldItem_.lock()) {
        if(auto pickup = item->GetComponent<ComponentItemPickUp>())
            pickup->OnThrow();
    }

    heldItem_.reset();
}

void ComponentPlayerPickUpTrigger::PickUpItem()
{
    //Already holding something(もう持っている)
    if(HasItem())
        return;

    // check every item in the scene that has ComponentItemPickUp
    auto items = Scene::GetObjectsPtr<PoittersPoint::Item>();
    for(auto& item : items) {
        auto pickup = item->GetComponent<ComponentItemPickUp>();
        if(!pickup || pickup->IsHeld())
            continue;    // no pickup component, or already held by someone else

        float dist = length(item->GetTranslate() - GetOwner()->GetTranslate());
        if(dist <= pickupRange_) {
            pickup->OnPickedUp(GetOwner());
            heldItem_ = item;

            //Lift time depends on item weight and our strength. No stats = no waiting(持ち上げ時間はアイテムの重さと自分の筋力で決まる。ステータスがなければ待たない)
            auto stats = GetOwner()->GetComponent<ComponentCharacterStats>();
            liftTotal_ = stats ? stats->GetLiftTime(item->GetWeight()) : 0.0f;
            liftTimer_ = liftTotal_;

            break;    // only grab one item per frame
        }
    }
}

PoittersPoint::ItemPtr ComponentPlayerPickUpTrigger::GetNearbyItem()
{
    // check every item in the scene that has ComponentItemPickUp
    auto items = Scene::GetObjectsPtr<PoittersPoint::Item>();
    for(auto& item : items) {
        auto pickup = item->GetComponent<ComponentItemPickUp>();
        if(!pickup || pickup->IsHeld())
            continue;    // no pickup component, or already held by someone else

        float dist = length(item->GetTranslate() - GetOwner()->GetTranslate());
        if(dist <= pickupRange_)
            return item;    // found the closest-checked one in range
    }
    return nullptr;    // nothing nearby
}

void ComponentPlayerPickUpTrigger::GUI()
{
    __super::GUI();

    // GUI内に出現させる
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("PickUpTrigger")) {
            // 有効/無効
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text(HasItem() ? "Holding item" : "Empty handed");
            ImGui::Text("Lift time : %.2f / %.2f", liftTimer_, liftTotal_);
            ImGui::DragFloat("Pickup Range", &pickupRange_, 0.5f, 0.0f, 100.0f);

            // GUI上でオーナーから自分(SampleObjectController)を削除します
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentPlayerPickUpTrigger)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentPlayerPickUpTrigger)
