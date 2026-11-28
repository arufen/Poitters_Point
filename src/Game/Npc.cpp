//---------------------------------------------------------------------------
//!	@file	Npc.cpp
//! @brief	Npc (AI controlled character)
//---------------------------------------------------------------------------

#include "Npc.h"
#include "Player.h"
#include "MainStage.h"
#include "Component/ComponentCharacterStats.h"
#include "Component/ComponentPlayerPickUpTrigger.h"
#include "Component/ComponentArenaBounds.h"
#include "Component/ComponentGroundShadow.h"
#include "Component/ComponentMatch.h"
#include "Component/ComponentKnockdown.h"
#include "Component/ComponentNpcAction.h"
#include "Component/ComponentCatchGuard.h"
#include "StageConfig.h"
#include <cmath>
#include <string>

namespace {
//Vector from "from" to "to" on the flat ground, height is ignored(fromからtoへのベクトル。高さは無視して平面だけ見る)
float3 FlatVector(const float3& from, const float3& to)
{
    float3 v = to - from;
    v.y      = 0.0f;
    return v;
}
}    // namespace

namespace PoittersPoint {
bool Npc::Init()
{
    Super::Init();

    //Every NPC gets its own name (Npc0, Npc1...), easy to find in the ImGui windows(NPCごとに別の名前(Npc0, Npc1…)。ImGuiのウィンドウで見分けやすい)
    static int npcCount = 0;
    SetName("Npc" + std::to_string(npcCount++));

    //[CHANGE] Model, animations and stats come from NpcCharacterDef.h. Default = cha0002 バイト(CVS)
    //(Before: the police model with only idle/walk/throw, and the throw animation was never played)
    //([変更] モデル・アニメ・ステータスはNpcCharacterDef.hから。最初はcha0002 バイト(CVS)
    // (前：警察のモデルでidle/walk/throwだけ。投げアニメは一度も再生されていなかった))
    ApplyCharacter(NpcCharacterId::CVS);

    SetTranslate({40 + (GetRand(60) - 30), 5, 40 + (GetRand(60) - 30)});

    auto col = AddComponent<ComponentCollisionCapsule>()    //
                   ->SetRadius(3.0f)
                   ->SetHeight(23.4f);

    col->UseGravity();
    col->SetCollisionGroup(ComponentCollision::CollisionGroup::PLAYER);

    //Same pickup component as the player, it works for any Object(プレイヤーと同じ拾うコンポーネント。どのObjectでも使える)
    AddComponent<ComponentPlayerPickUpTrigger>();

    //Invisible wall, so the NPC can not fall off the stage(見えない壁。NPCがステージから落ちないように)
    AddComponent<ComponentArenaBounds>();

    //Round shadow under the feet, cast onto the floor(床に落とす足元の丸い影)
    AddComponent<ComponentGroundShadow>()->SetRadius(StageConfig::NPC_SHADOW_RADIUS);

    //(ComponentKnockdown and ComponentNpcAction were added by ApplyCharacter() above)
    //(ComponentKnockdownとComponentNpcActionは上のApplyCharacter()で追加済み)

    return true;
}

//[ADD] Become a character from NpcCharacterDef.h([追加] NpcCharacterDef.hのキャラになる)
void Npc::ApplyCharacter(NpcCharacterId id)
{
    characterId_    = id;
    const auto& def = GetNpcCharacterDef(id);

    //Model + animations. Remove the old model first, so this can also switch characters later
    //(モデル＋アニメ。先に古いモデルを外すので、あとからキャラを変えることもできる)
    RemoveComponent<ComponentModel>();
    AddComponent<ComponentModel>(def.modelPath)->SetAnimation(def.animations);

    //Stats with a small random spread(少しランダムに散らしたステータス)
    auto stats = GetComponent<ComponentCharacterStats>();
    if(!stats)
        stats = AddComponent<ComponentCharacterStats>();
    stats->SetMaxHP(def.hpMin + GetRand(def.hpRandom))
        ->SetStrength(def.strengthMin + GetRand(def.strengthSteps) * 0.1f)
        ->SetSpeed(def.speedMin + GetRand(def.speedSteps) * 0.05f);

    //Getting hit: slide back, drop the item, play down/getup. Light characters are blown further
    //(当たった時：後ろに滑って、物を落とし、down/getupを再生。軽いキャラは遠くへ飛ぶ)
    auto knock = GetComponent<ComponentKnockdown>();
    if(!knock)
        knock = AddComponent<ComponentKnockdown>();
    knock->SetKnockbackScale(def.knockbackScale);

    //The "body": Player-like animations for run / lift / carry / throw(「体」：走る・持ち上げ・運ぶ・投げるのプレイヤーと同じアニメ)
    auto action = GetComponent<ComponentNpcAction>();
    if(!action)
        action = AddComponent<ComponentNpcAction>();
    action->SetThrowTiming(def.throwAnimSpeed, def.throwReleaseRatio);

    //[ADD] cha0003 POLICE: catch/guard items thrown from the front. Other characters do not have it
    //([追加] cha0003 POLICE：前から投げられた物をキャッチ・ガードする。ほかのキャラは持たない)
    if(def.catchGuard) {
        if(!GetComponent<ComponentCatchGuard>())
            AddComponent<ComponentCatchGuard>();
    }
    else {
        RemoveComponent<ComponentCatchGuard>();
    }
}

void Npc::Update()
{
    Super::Update();

    //Frozen during the countdown, when knocked down, after K.O. and after the match ended
    //[CHANGE] Freeze() also leaves the down/getup animation alone (StopMove() used to play idle over it)
    //(カウントダウン中・ダウン中・K.O.後・試合終了後は動かない。
    // [変更] Freeze()はdown/getupのアニメにも手を出さない(前のStopMove()は上からidleを再生していた))
    if(!ComponentMatch::CanAct(this)) {
        if(auto action = GetComponent<ComponentNpcAction>())
            action->Freeze();
        return;
    }

    //[ADD] cha0003 POLICE caught something: throw it right back at whoever threw it
    //([追加] cha0003 POLICEが何かをキャッチした：投げた相手にすぐ投げ返す)
    if(auto guard = GetComponent<ComponentCatchGuard>()) {
        const Object* thrower = nullptr;
        if(guard->TakeCatch(thrower))
            StartCounterThrow(thrower);

        //[ADD] Something is flying at us: stop and turn to face it, so it comes from the front.
        //Free hands -> it will be caught, busy hands -> guarded. Not during the throw animation (do not cancel our own throw)
        //([追加] 何かがこちらへ飛んできている：止まってその方を向く。そうすれば物は「前」から来る。
        // 手が空いていればキャッチ、ふさがっていればガードになる。投げアニメ中はやらない(自分の投げを止めない))
        auto   action = GetComponent<ComponentNpcAction>();
        float3 toItem;
        if(!(action && action->IsThrowing()) && guard->FindIncomingItem(toItem)) {
            StopMove();
            FaceTowards(toItem);
            return;
        }
    }

    switch(state_) {
    case State::SearchItem:
        UpdateSearchItem();
        break;

    case State::AimThrow:
        UpdateAimThrow();
        break;

    case State::Throwing:
        UpdateThrowing();
        break;

    case State::Cooldown:
        UpdateCooldown();
        break;
    }
}

void Npc::UpdateSearchItem()
{
    auto trigger = GetComponent<ComponentPlayerPickUpTrigger>();
    if(!trigger)
        return;

    //Already holding an item, go aim(もうアイテムを持っているので、ねらいへ)
    if(trigger->HasItem()) {
        StartAim();
        return;
    }

    //An item is in pickup range, pick it up(アイテムが拾える範囲にあるので拾う)
    if(trigger->GetNearbyItem()) {
        StopMove();
        trigger->PickUpItem();

        if(trigger->HasItem())
            StartAim();

        return;
    }

    //Find the closest item that nobody is holding(誰も持っていない一番近いアイテムを探す)
    float3 myPos     = GetTranslate();
    float3 bestDir   = {0.0f, 0.0f, 0.0f};
    float  bestDist  = 1.0e30f;
    bool   foundItem = false;

    for(auto& item : Scene::GetObjectsPtr<PoittersPoint::Item>()) {
        auto pickup = item->GetComponent<ComponentItemPickUp>();
        if(!pickup || pickup->IsHeld() || pickup->IsThrown())
            continue;    //No pickup component, someone is holding it, or it is still flying/bouncing(拾うコンポーネントがない、誰かが持っている、または飛んでいる・バウンド中)

        float3 toItem = FlatVector(myPos, item->GetTranslate());
        float  dist   = (float)length(toItem);
        if(dist < bestDist) {
            bestDist  = dist;
            bestDir   = toItem;
            foundItem = true;
        }
    }

    if(foundItem && bestDist > 0.01f)
        MoveTowards(normalize(bestDir));
    else
        StopMove();
}

void Npc::StartAim()
{
    // Decide target: throw at the player, or at other NPCs, or in a random direction
    // ターゲット決定：プレイヤーかNPCか、またはランダム方向
    float roll = GetRand(99) / 99.0f;
    aimTarget_.reset();

    if(roll < throwAtPlayerChance_) {
        aimMode_ = AimMode::Player;
    }
    else if(roll < throwAtPlayerChance_ + throwAtNpcChance_) {
        aimMode_ = AimMode::Npc;

        // Find closest other NPC / 一番近い他のNPCを狙う
        float3 myPos    = GetTranslate();
        float  bestDist = 1.0e30f;
        for(auto& npc : Scene::GetObjectsPtr<PoittersPoint::Npc>()) {
            if(npc.get() == this)
                continue;

            float dist = (float)length(FlatVector(myPos, npc->GetTranslate()));
            if(dist < bestDist) {
                bestDist   = dist;
                aimTarget_ = npc;
            }
        }
    }
    else {
        aimMode_ = AimMode::Random;
    }

    float angle = GetRand(359) * DegToRad;
    randomDir_  = float3(sinf(angle), 0.0f, cosf(angle));

    aimTimer_ = 0;

    //Pause for a moment after picking up, like a real player(本物のプレイヤーのように、拾ったあと少し止まる)
    holdTimer_ = holdMinFrames_ + GetRand(holdRandomFrames_);

    state_ = State::AimThrow;
}

//[ADD] Aim at the one who threw the item we just caught, with only a short pause
//If that one is gone (or unknown), keep the normal target StartAim() picked
//([追加] 今キャッチした物を投げた相手をねらう。待ち時間は短い。
// その相手がもういない(またはわからない)なら、StartAim()が選んだ普通のターゲットのまま)
void Npc::StartCounterThrow(const Object* thrower)
{
    StartAim();
    holdTimer_ = counterHoldFrames_;

    if(!thrower)
        return;

    for(auto& player : Scene::GetObjectsPtr<PoittersPoint::Player>()) {
        if(player.get() == thrower) {
            aimMode_ = AimMode::Player;
            aimTarget_.reset();
            return;
        }
    }

    for(auto& npc : Scene::GetObjectsPtr<PoittersPoint::Npc>()) {
        if(npc.get() == thrower && npc.get() != this) {
            aimMode_   = AimMode::Npc;
            aimTarget_ = npc;
            return;
        }
    }
}

float3 Npc::GetAimDirection()
{
    if(aimMode_ == AimMode::Player) {
        for(auto& player : Scene::GetObjectsPtr<PoittersPoint::Player>()) {
            float3 toPlayer = FlatVector(GetTranslate(), player->GetTranslate());
            if((float)length(toPlayer) > 0.01f)
                return normalize(toPlayer);
        }
    }
    else if(aimMode_ == AimMode::Npc) {
        //lock() gives nullptr if that NPC is already gone(そのNPCがもういなければlock()はnullptrを返す)
        if(auto target = aimTarget_.lock()) {
            float3 toNpc = FlatVector(GetTranslate(), target->GetTranslate());
            if((float)length(toNpc) > 0.01f)
                return normalize(toNpc);
        }
    }

    //No target found, or random mode(ターゲットがいない、またはランダムモード)
    return randomDir_;
}

void Npc::UpdateAimThrow()
{
    auto trigger = GetComponent<ComponentPlayerPickUpTrigger>();

    //We lost the item, go back to searching(アイテムを失ったので探す状態に戻る)
    if(!trigger || !trigger->HasItem()) {
        state_ = State::SearchItem;
        return;
    }

    //Still lifting a heavy item, wait here and do not move or throw(まだ重いアイテムを持ち上げ中なので、ここで待つ。動かないし投げない)
    if(trigger->IsLifting()) {
        StopMove();
        return;
    }

    //Lift is done. Stand still for a moment before aiming (counts down only after the lift)(持ち上げが終わった。ねらう前に少し立ち止まる。持ち上げが終わってから数える)
    if(holdTimer_ > 0) {
        holdTimer_--;
        StopMove();
        return;
    }

    float3 aimDir = GetAimDirection();

    StopMove();
    FaceTowards(aimDir);
    aimTimer_++;

    //Throw when we face the target, or when we waited too long(ターゲットを向いたら投げる。待ちすぎても投げる)
    //[CHANGE] Start the throw ANIMATION. The item leaves the hand at the arm-swing moment (ComponentNpcAction)
    //Before: the item flew away at once with no animation
    //([変更] 投げ「アニメ」を始める。物は腕を振った瞬間に手から離れる(ComponentNpcAction)。前：アニメなしですぐ飛んでいた)
    if(IsFacing(aimDir) || aimTimer_ >= aimTimeoutFrames_) {
        auto action = GetComponent<ComponentNpcAction>();
        if(action && action->StartThrow())
            state_ = State::Throwing;
    }
}

//[ADD] Wait until the throw animation is over, then cool down([追加] 投げアニメが終わるまで待ち、それから休む)
void Npc::UpdateThrowing()
{
    auto action = GetComponent<ComponentNpcAction>();
    if(action && action->IsThrowing()) {
        StopMove();    //Stand still. The throw animation keeps playing(立ち止まる。投げアニメはそのまま流れる)
        return;
    }

    cooldownTimer_ = cooldownFrames_;
    state_         = State::Cooldown;
}

void Npc::UpdateCooldown()
{
    StopMove();

    cooldownTimer_--;
    if(cooldownTimer_ <= 0)
        state_ = State::SearchItem;
}

//[CHANGE] The 4 functions below now only pass the order to ComponentNpcAction (the "body"),
//which moves and picks the Player-like animation
//([変更] 下の4つの関数は、命令をComponentNpcAction(「体」)に渡すだけになった。
// 動かすのと、プレイヤーと同じアニメを選ぶのは「体」がやる)
void Npc::MoveTowards(const float3& dir)
{
    if(auto action = GetComponent<ComponentNpcAction>())
        action->Move(dir);
}

void Npc::StopMove()
{
    if(auto action = GetComponent<ComponentNpcAction>())
        action->Stop();
}

void Npc::FaceTowards(const float3& dir)
{
    if(auto action = GetComponent<ComponentNpcAction>())
        action->Face(dir);
}

bool Npc::IsFacing(const float3& dir)
{
    auto action = GetComponent<ComponentNpcAction>();
    return action ? action->IsFacing(dir) : true;
}

void Npc::TakeDamage(int dmg)
{
    if(auto stats = GetComponent<ComponentCharacterStats>())
        stats->TakeDamage(dmg);
}

}    // namespace PoittersPoint
