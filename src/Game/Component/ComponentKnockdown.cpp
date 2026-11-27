#include "Game/Component/ComponentKnockdown.h"
#include "Game/Component/ComponentPlayerPickUpTrigger.h"
#include "Game/Component/ComponentCharacterStats.h"
#include "Game/StageConfig.h"
#include <cmath>
#include <cstring>

namespace SC = PoittersPoint::StageConfig;

namespace {
const char* DOWN_TEXT = "DOWN!";

//[ADD] Own name, because DxLib already has "PI"([追加]DxLibにもう「PI」があるので自分専用の名前)
const float KNOCK_PI = 3.14159265f;

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

    bool wasDown = IsDown();

    duration_ = SC::KNOCKDOWN_TIME;
    timer_    = duration_;    //Restarts if already down(すでにダウン中なら最初から)

    //Let go of whatever we carry, and cancel lifting(持っている物を離して、持ち上げもやめる)
    if(auto trigger = GetOwner()->GetComponent<ComponentPlayerPickUpTrigger>())
        trigger->DropItem();

    //[ADD] Fall-down animation setup([追加]倒れるアニメの準備)
    auto mdl = GetOwner()->GetComponent<ComponentModel>();
    if(!mdl || !fallEnabled_)
        return;

    //[ADD] Animation-file mode: the model has "down" and "getup" (the Player)
    //([追加] アニメファイルのモード：モデルに「down」と「getup」がある(プレイヤー))
    if(HasKnockAnimations()) {
        getUpStarted_ = false;

        if(wasDown || animActive_) {
            //Hit again while down: stay lying (last frame of "down"), and the wait starts again
            //(ダウン中にまた当たった：倒れたまま(「down」の最後のフレーム)で、待ち時間が最初から)
            timer_ = duration_ - downAnimTime_;
            mdl->PlayAnimation(ANIM_DOWN, false, 0.1f, mdl->GetAnimationTotalTime(ANIM_DOWN));
            animActive_ = true;
            return;
        }

        //Turn to face the hit, so "down" (which always falls BACKWARD) falls the way we are pushed.
        //SetRotationToVector(v) makes the model look toward v, the same as walking does
        //(当たった方を向く。「down」はいつも「後ろ」に倒れるので、これで押された方向へ倒れる。
        // SetRotationToVector(v)はモデルをvの方へ向ける。歩く時と同じ)
        if((float)length(dir) > 0.001f)
            mdl->SetRotationToVector(-dir);

        //Speed so the whole animation takes downAnimTime_ / getUpAnimTime_ seconds. Animation time runs at 30 per second
        //(アニメ全体がdownAnimTime_/getUpAnimTime_秒になる速さ。アニメの時間は1秒に30進む)
        float downTotal  = mdl->GetAnimationTotalTime(ANIM_DOWN);
        float getUpTotal = mdl->GetAnimationTotalTime(ANIM_GETUP);
        if(downAnimTime_ > 0.01f)
            mdl->SetAnimationSpeed(ANIM_DOWN, downTotal / (30.0f * downAnimTime_));
        if(getUpAnimTime_ > 0.01f)
            mdl->SetAnimationSpeed(ANIM_GETUP, getUpTotal / (30.0f * getUpAnimTime_));

        //Short blend: being hit is sudden(短いブレンド：当たるのは一瞬なので)
        mdl->PlayAnimation(ANIM_DOWN, false, 0.05f, 0.0f);
        animActive_ = true;
        return;
    }

    //Fallback below: no animation files, tilt the whole model(以下は予備：アニメのファイルがないので、モデル全体を傾ける)
    if(wasDown || poseSaved_) {
        //Already lying: skip the fall part, so the body does not jump up and fall again(もう倒れている：倒れる部分を飛ばす。体が起きてまた倒れないように)
        timer_ = duration_ - fallTime_;
        return;
    }

    //Remember the standing pose, to put it back when getting up(立っている姿勢を覚えて、起き上がる時に戻す)
    basePose_  = mdl->GetMatrix();
    poseSaved_ = true;

    //Fall the way we are pushed. Hit from straight above (no direction) = fall backward
    //The models look toward -Z, so "backward" is the model's +Z axis
    //(押された方向へ倒れる。真上から当たった(方向がない)なら後ろへ倒れる。モデルは-Zを向いているので「後ろ」はモデルの+Z軸)
    float3 fall = dir;
    if((float)length(fall) < 0.001f) {
        fall   = mdl->GetWorldMatrix().axisZ();
        fall.y = 0.0f;
    }
    fallDir_ = ((float)length(fall) > 0.001f) ? normalize(fall) : float3(0.0f, 0.0f, 1.0f);

    //Which way rotateAxis turns depends on the coordinate system. Test it once: the head must move TOWARD fallDir_
    //(rotateAxisが回る向きは座標系で変わる。一度試して、頭がfallDir_の方へ動くほうを選ぶ)
    float3 axis = normalize(cross(float3(0.0f, 1.0f, 0.0f), fallDir_));
    float3 head = mul(float4(0.0f, 1.0f, 0.0f, 0.0f), matrix::rotateAxis(axis, 1.0f)).xyz;
    fallSign_   = ((float)dot(head, fallDir_) >= 0.0f) ? 1.0f : -1.0f;
}

void ComponentKnockdown::Update()
{
    Super::Update();

    if(timer_ > 0.0f) {
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

    //K.O. = stay on the ground(K.O.なら地面に倒れたまま)
    bool dead = false;
    if(auto stats = GetOwner()->GetComponent<ComponentCharacterStats>())
        dead = stats->IsDead();

    //[ADD] Animation-file mode: start "getup" near the end, then give the model back to walking/idle
    //([追加] アニメファイルのモード：最後のほうで「getup」を始め、そのあとモデルを歩き/アイドルに返す)
    if(animActive_) {
        auto mdl = GetOwner()->GetComponent<ComponentModel>();

        if(!dead && !getUpStarted_ && timer_ <= getUpAnimTime_) {
            getUpStarted_ = true;
            if(mdl)
                mdl->PlayAnimation(ANIM_GETUP, false, 0.08f, 0.0f);
        }

        //Done: standing again. A K.O. keeps animActive_ so nobody plays "idle" over the lying pose
        //(終わり：また立った。K.O.ならanimActive_のままにして、倒れた姿勢の上に誰も「idle」を再生しないようにする)
        if(!IsDown() && !dead)
            animActive_ = false;
        return;
    }

    //[ADD] Fallback tilt([追加]予備の傾け)
    if(!poseSaved_)
        return;    //Standing normally, nothing to do(普通に立っている。何もしない)

    if(!fallEnabled_) {
        RestorePose();
        return;
    }

    if(IsDown() || dead)
        ApplyFallPose(GetFallDegrees(dead), GetHopHeight());
    else
        RestorePose();    //Knockdown is over: stand up straight again(ダウン終わり：またまっすぐ立つ)
}

//[ADD] Both animations are registered on the model (GetAnimationTotalTime returns 0 when the name is unknown)
//([追加] モデルに両方のアニメが登録されているか(名前がなければGetAnimationTotalTimeは0を返す))
bool ComponentKnockdown::HasKnockAnimations() const
{
    auto mdl = GetOwner()->GetComponent<ComponentModel>();
    return mdl && mdl->GetAnimationTotalTime(ANIM_DOWN) > 0.0f && mdl->GetAnimationTotalTime(ANIM_GETUP) > 0.0f;
}

//[ADD] Tilt for the current moment. See the timeline in the header([追加]今の傾き。流れはヘッダーを参照)
float ComponentKnockdown::GetFallDegrees(bool dead) const
{
    float elapsed = duration_ - timer_;    //Seconds since the hit(当たってからの秒数)

    //1. Falling: fast at first, slow at the end (ease-out)(1. 倒れる：最初は速く、最後はゆっくり(イーズアウト))
    if(fallTime_ > 0.0f && elapsed < fallTime_) {
        float t = elapsed / fallTime_;
        return fallMaxDeg_ * (1.0f - (1.0f - t) * (1.0f - t));
    }

    //2. Getting up: only when still alive(2. 起き上がる：生きている時だけ)
    if(!dead && getUpTime_ > 0.0f && timer_ < getUpTime_) {
        float t = timer_ / getUpTime_;    //1 -> 0 while getting up(起き上がる間に1→0)
        return fallMaxDeg_ * (t * t * (3.0f - 2.0f * t));
    }

    //3. Lying on the ground(3. 地面に倒れている)
    return fallMaxDeg_;
}

//[ADD] Small hop while falling: up and back down once (sin curve)([追加]倒れる間の小さなジャンプ：1回上がって下りる(sinカーブ))
float ComponentKnockdown::GetHopHeight() const
{
    float elapsed = duration_ - timer_;
    if(fallTime_ <= 0.0f || elapsed >= fallTime_ || timer_ <= 0.0f)
        return 0.0f;

    return hopHeight_ * sinf(KNOCK_PI * (elapsed / fallTime_));
}

//[ADD] Tilt the model around the feet. Only the MODEL turns, the collision capsule stays standing
//([追加]足元を中心にモデルを傾ける。回るのはモデルだけで、当たり判定のカプセルは立ったまま)
void ComponentKnockdown::ApplyFallPose(float degrees, float hop)
{
    auto mdl = GetOwner()->GetComponent<ComponentModel>();
    if(!mdl)
        return;

    fallDegNow_ = degrees;

    //Turn around the axis that is flat and at a right angle to the fall direction(水平で、倒れる方向と直角な軸のまわりに回す)
    float3 axis = normalize(cross(float3(0.0f, 1.0f, 0.0f), fallDir_));
    matrix rot  = matrix::rotateAxis(axis, fallSign_ * degrees * (KNOCK_PI / 180.0f));

    //Standing pose -> tilt -> small hop. Matrices apply LEFT to RIGHT in this engine (use mul(), not "*")
    //(立っている姿勢→傾ける→小さく跳ねる。このエンジンの行列は左から右の順にかかる(「*」ではなくmul()を使う))
    mdl->Matrix() = mul(mul(basePose_, rot), matrix::translate(0.0f, hop, 0.0f));
}

//[ADD] Put the standing pose back([追加]立っている姿勢に戻す)
void ComponentKnockdown::RestorePose()
{
    if(!poseSaved_)
        return;

    if(auto mdl = GetOwner()->GetComponent<ComponentModel>())
        mdl->Matrix() = basePose_;

    poseSaved_  = false;
    fallDegNow_ = 0.0f;
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

            //[ADD] Fall-down animation tuning([追加]倒れるアニメの調整)
            ImGui::Separator();
            ImGui::Checkbox(u8"倒れるアニメ / Fall animation", &fallEnabled_);

            //[ADD] Which mode this character uses([追加] このキャラがどちらのモードを使っているか)
            ImGui::Text(HasKnockAnimations() ? u8"モード: アニメファイル(down/getup)" : u8"モード: コードで傾ける(アニメなし)");
            ImGui::DragFloat(u8"downの秒数(Down sec)", &downAnimTime_, 0.01f, 0.05f, 1.0f);
            ImGui::DragFloat(u8"getupの秒数(GetUp sec)", &getUpAnimTime_, 0.01f, 0.05f, 1.0f);

            ImGui::Text(u8"--- 予備(傾け) / Fallback tilt ---");
            ImGui::Text("Tilt now: %.1f deg", fallDegNow_);
            ImGui::DragFloat(u8"倒れる角度(Max deg)", &fallMaxDeg_, 1.0f, 0.0f, 90.0f);
            ImGui::DragFloat(u8"倒れる秒数(Fall sec)", &fallTime_, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat(u8"起き上がる秒数(Get up sec)", &getUpTime_, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat(u8"跳ねる高さ(Hop)", &hopHeight_, 0.1f, 0.0f, 20.0f);

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentKnockdown)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentKnockdown)
