#include "Game/Component/ComponentKnockdown.h"
#include "Game/Component/ComponentPlayerPickUpTrigger.h"
#include "Game/StageConfig.h"
#include <cmath>
#include <cstring>

namespace SC = PoittersPoint::StageConfig;

namespace {
const char* DOWN_TEXT = "DOWN!";

//Text with a black outline, same look as the lifting text(黒いふちどりの文字。持ち上げ中の文字と同じ見た目)
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

void ComponentKnockdown::Init()
{
    Super::Init();
    timer_    = 0.0f;
    duration_ = SC::KNOCKDOWN_TIME;
    slideVel_ = {0.0f, 0.0f, 0.0f};
}

void ComponentKnockdown::Knock(const float3& awayDir, float distance)
{
    //Flat direction only. No direction (hit from straight above) = no slide(平面の方向だけ。方向がない(真上から当たった)なら滑らない)
    float3 dir = {(float)awayDir.x, 0.0f, (float)awayDir.z};
    float  len = (float)length(dir);
    dir        = (len > 0.001f) ? dir / len : float3(0.0f, 0.0f, 0.0f);

    //With speed v and friction f, the slide covers v / f, so v = distance x f(速さv・摩擦fだと滑る距離はv/f。だからv＝距離×f)
    slideVel_ = dir * (distance * SC::KNOCKBACK_FRICTION);

    duration_ = SC::KNOCKDOWN_TIME;
    timer_    = duration_;    //Restarts if already down(すでにダウン中なら最初から)

    //Let go of whatever we carry, and cancel lifting(持っている物を離して、持ち上げもやめる)
    if(auto trigger = GetOwner()->GetComponent<ComponentPlayerPickUpTrigger>())
        trigger->DropItem();
}

void ComponentKnockdown::Update()
{
    Super::Update();

    if(timer_ <= 0.0f)
        return;

    float delta  = GetDeltaTime();
    timer_      -= delta;
    if(timer_ < 0.0f)
        timer_ = 0.0f;

    //Slide on x/z only, never touch y, so the collision gravity keeps working(x/zだけ滑る。yは触らないので、当たり判定の重力はそのまま動く)
    if((float)length(slideVel_) > 0.1f) {
        float3 pos  = GetOwner()->GetTranslate();
        pos.x      += slideVel_.x * delta;
        pos.z      += slideVel_.z * delta;
        GetOwner()->SetTranslate(pos);

        //Slow down smoothly, the same amount every second whatever the frame rate(フレームレートに関係なく、毎秒同じだけなめらかに遅くなる)
        slideVel_ *= expf(-SC::KNOCKBACK_FRICTION * delta);
    }
}

void ComponentKnockdown::PostDraw()
{
    Super::PostDraw();

    if(!IsDown())
        return;

    //Head position in the world -> screen position, same as the lifting bar(頭の位置のワールド座標 -> 画面座標。持ち上げバーと同じ)
    float3        pos        = GetOwner()->GetTranslate();
    DxLib::VECTOR world_pos  = {pos.x, pos.y + 22.0f, pos.z};
    DxLib::VECTOR screen_pos = ConvWorldPosToScreenPos(world_pos);

    //Do not draw when behind the camera(カメラの後ろなら描かない)
    if(screen_pos.z <= 0.0f || screen_pos.z >= 1.0f)
        return;

    int x = (int)screen_pos.x;
    int y = (int)screen_pos.y;

    DrawOutlined(x, y + 6, DOWN_TEXT, GetColor(255, 80, 80));

    //Red gauge: empties from right to left until they get up(赤いゲージ：起き上がるまで右から左へ減っていく)
    float left   = duration_ > 0.0f ? timer_ / duration_ : 0.0f;
    int   gaugeW = 50;
    int   gaugeH = 6;
    int   gx     = x - gaugeW / 2;
    int   gy     = y + 26;

    DrawBox(gx - 1, gy - 1, gx + gaugeW + 1, gy + gaugeH + 1, GetColor(0, 0, 0), TRUE);
    DrawBox(gx, gy, gx + gaugeW, gy + gaugeH, GetColor(70, 70, 70), TRUE);
    DrawBox(gx, gy, gx + (int)(gaugeW * left), gy + gaugeH, GetColor(230, 40, 40), TRUE);
}

void ComponentKnockdown::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("Knockdown")) {
            ImGui::Text(IsDown() ? "DOWN  %.2f s" : "Standing", timer_);

            //Test button: knock back toward +X(テスト用：+X方向にふっとばす)
            if(ImGui::Button(u8"テスト：ダウン"))
                Knock(float3(1.0f, 0.0f, 0.0f), 20.0f);

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentKnockdown)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentKnockdown)
