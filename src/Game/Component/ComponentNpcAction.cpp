//---------------------------------------------------------------------------
//!	@file	ComponentNpcAction.cpp
//! @brief	[ADD] The NPC's "body". See the header for the rules
//!         ([追加] NPCの「体」。ルールはヘッダーを参照)
//---------------------------------------------------------------------------
#include "Game/Component/ComponentNpcAction.h"
#include "Game/Component/ComponentPlayerPickUpTrigger.h"
#include "Game/Component/ComponentCharacterStats.h"
#include "Game/Component/ComponentKnockdown.h"

void ComponentNpcAction::Init()
{
    Super::Init();
    throwTimer_      = 0.0f;
    throwReleased_   = false;
    liftAnimStarted_ = false;
}

void ComponentNpcAction::Update()
{
    Super::Update();

    //Throw in progress: let go of the item at the release moment, then wait for the animation to end
    //Same as Player::Update(投げの途中：離す瞬間に物を離し、アニメが終わるまで待つ。Player::Updateと同じ)
    if(throwTimer_ <= 0.0f)
        return;

    throwTimer_ += GetDeltaTime();

    if(!throwReleased_ && throwTimer_ >= throwReleaseTime_) {
        throwReleased_ = true;
        if(auto trigger = GetOwner()->GetComponent<ComponentPlayerPickUpTrigger>())
            trigger->ThrowItem();
    }

    if(throwTimer_ >= throwAnimTime_) {
        throwTimer_    = 0.0f;    //Done. The next Move()/Stop() picks idle or run again(終わり。次のMove()/Stop()がまたidleかrunを選ぶ)
        throwReleased_ = false;
    }
}

void ComponentNpcAction::Move(const float3& dir)
{
    //Can not walk while lifting or throwing, like the Player(プレイヤーと同じく、持ち上げ中・投げ中は歩けない)
    if(IsBusy()) {
        PlayStateAnimation(false);
        return;
    }

    //Speed comes from the stats (the speed potion buff is included)(速さはステータスから(スピードポーションのバフ込み))
    auto  stats = GetOwner()->GetComponent<ComponentCharacterStats>();
    float speed = stats ? stats->GetSpeed() : 1.0f;    //1.0 = fallback without stats(ステータスがない時の予備)

    //Same call as ComponentStateIdleWalk / the old Npc::MoveTowards(ComponentStateIdleWalk・前のNpc::MoveTowardsと同じ呼び方)
    GetOwner()->AddTranslate(dir * speed, true);
    Face(dir);

    PlayStateAnimation(true);
}

void ComponentNpcAction::Stop()
{
    PlayStateAnimation(false);
}

void ComponentNpcAction::Face(const float3& dir)
{
    if(auto mdl = GetOwner()->GetComponent<ComponentModel>())
        mdl->SetRotationToVectorWithLimit(dir, rotateSpeed_);
}

bool ComponentNpcAction::IsFacing(const float3& dir) const
{
    auto mdl = GetOwner()->GetComponent<ComponentModel>();
    if(!mdl)
        return true;

    //The models look toward -Z, same as ComponentItemPickUp::OnThrow(モデルは-Zを向いている。ComponentItemPickUp::OnThrowと同じ)
    float3 facing = normalize(-mdl->GetWorldVectorAxisZ());
    return (float)dot(facing, dir) > 0.98f;
}

bool ComponentNpcAction::StartThrow()
{
    auto trigger = GetOwner()->GetComponent<ComponentPlayerPickUpTrigger>();
    if(!trigger || !trigger->HasItem() || trigger->IsLifting() || IsThrowing())
        return false;

    auto mdl = GetOwner()->GetComponent<ComponentModel>();

    //Real length of the throw animation, from its frame count and speed. Same idea as Player::Update
    //(投げアニメの本当の長さを、フレーム数と速さから出す。Player::Updateと同じ考え方)
    float native = mdl ? mdl->GetAnimationTotalTime("throw") : 0.0f;
    if(native > 0.0f && throwAnimSpeed_ > 0.01f) {
        throwAnimTime_    = native / (ANIM_FPS * throwAnimSpeed_);
        throwReleaseTime_ = throwAnimTime_ * throwReleaseRatio_;
    }
    else {
        //No throw animation: let go right away(投げアニメがない：すぐに離す)
        throwAnimTime_    = 0.05f;
        throwReleaseTime_ = 0.0f;
    }

    if(mdl) {
        //Keep the real playback speed the same as the timing above (also after changing it in ImGui)
        //(実際の再生速度を上のタイミングと同じにする(ImGuiで変えた後も))
        mdl->SetAnimationSpeed("throw", throwAnimSpeed_);
        mdl->PlayAnimation("throw", false, 0.15f, 0.0f);
    }

    throwTimer_    = 0.0001f;    //Started(始まった)
    throwReleased_ = false;
    return true;
}

bool ComponentNpcAction::IsBusy() const
{
    if(IsThrowing())
        return true;

    auto trigger = GetOwner()->GetComponent<ComponentPlayerPickUpTrigger>();
    return trigger && trigger->IsLifting();
}

void ComponentNpcAction::Freeze()
{
    //Getting hit cancels the throw (the item was already dropped)(当たったら投げを取り消す(物はもう落としている))
    throwTimer_      = 0.0f;
    throwReleased_   = false;
    liftAnimStarted_ = false;

    //The down/getup animation owns the model now. Do not play idle over it(down/getupのアニメがモデルを動かしている。上からidleを再生しない)
    if(auto knock = GetOwner()->GetComponent<ComponentKnockdown>(); knock && knock->IsPlayingKnockAnimation())
        return;

    if(auto mdl = GetOwner()->GetComponent<ComponentModel>())
        mdl->PlayAnimationNoSame("idle", true);
}

ComponentNpcActionPtr ComponentNpcAction::SetThrowTiming(float animSpeed, float releaseRatio)
{
    throwAnimSpeed_    = animSpeed;
    throwReleaseRatio_ = releaseRatio;
    return std::dynamic_pointer_cast<ComponentNpcAction>(shared_from_this());
}

void ComponentNpcAction::PlayStateAnimation(bool moving)
{
    auto mdl = GetOwner()->GetComponent<ComponentModel>();
    if(!mdl)
        return;

    //Throwing: the throw animation is already playing, keep it(投げ中：投げアニメがもう流れているので、そのまま)
    if(IsThrowing())
        return;

    auto trigger = GetOwner()->GetComponent<ComponentPlayerPickUpTrigger>();
    bool lifting = trigger && trigger->IsLifting();
    bool holding = trigger && trigger->HasItem();

    //Lifting: play "carryStart" once, at a speed that ends with the lift time. Same math as Player::Update
    //(持ち上げ中：「carryStart」を1回、持ち上げ時間ちょうどで終わる速さで再生する。Player::Updateと同じ計算)
    if(lifting) {
        if(!liftAnimStarted_) {
            liftAnimStarted_ = true;

            float native = mdl->GetAnimationTotalTime("carryStart");    //Frames at speed 1.0(速さ1.0でのフレーム数)
            float dur    = trigger->GetLiftDuration();                  //Seconds, from the item size and strength(秒。物の大きさと筋力から)

            float speed = 1.0f;
            if(native > 0.0f && dur > 0.001f)
                speed = native / (ANIM_FPS * dur);
            if(speed < minLiftAnimSpeed_)
                speed = minLiftAnimSpeed_;
            if(speed > maxLiftAnimSpeed_)
                speed = maxLiftAnimSpeed_;

            mdl->SetAnimationSpeed("carryStart", speed);
        }
        mdl->PlayAnimationNoSame("carryStart", false, 0.2f, 0.0f);
        return;
    }
    liftAnimStarted_ = false;    //Next lift calculates the speed again(次の持ち上げでまた速さを計算する)

    if(holding) {
        //Carrying: walk with the item, or hold the end pose of carryStart (frame 33), like the Player
        //(持っている：物を持って歩くか、carryStartの最後の姿勢(33フレーム)のまま立つ。プレイヤーと同じ)
        if(moving)
            mdl->PlayAnimationNoSame("carryWalk", true, 0.2f, 0.0f);
        else
            mdl->PlayAnimationNoSame("carryHold", false, 0.3f, 33.0f);
        return;
    }

    //Free hands(手ぶら)
    if(moving)
        mdl->PlayAnimationNoSame("run", true, 0.15f, 0.0f);
    else
        mdl->PlayAnimationNoSame("idle", true, 0.2f, 0.0f);
}

void ComponentNpcAction::GUI()
{
    __super::GUI();

    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode(u8"NpcAction")) {
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text(IsThrowing() ? "Throwing %.2f / %.2f s" : "Not throwing", throwTimer_, throwAnimTime_);
            ImGui::DragFloat(u8"投げアニメの速さ(Throw speed)", &throwAnimSpeed_, 0.05f, 0.1f, 5.0f);
            ImGui::DragFloat(u8"離す瞬間(Release 0-1)", &throwReleaseRatio_, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat(u8"回転の速さ(Turn speed)", &rotateSpeed_, 1.0f, 1.0f, 90.0f);

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentNpcAction)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentNpcAction)
