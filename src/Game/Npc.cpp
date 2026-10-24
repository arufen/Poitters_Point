//---------------------------------------------------------------------------
//!	@file	Npc.cpp
//! @brief	Npc (AI controlled character)
//---------------------------------------------------------------------------

#include "Npc.h"
#include "Player.h"
#include "MainStage.h"
#include "Component/ComponentPlayerPickUpTrigger.h"
#include <cmath>

namespace PoittersPoint {
bool Npc::Init()
{
    Super::Init();

    //Every NPC needs a different name, because the item attaches to the hand by object name(アイテムは名前で手にくっつくので、NPCごとに別の名前が必要)
    static int npcCount = 0;
    npcName_            = "Npc" + std::to_string(npcCount);
    npcCount++;
    SetName(npcName_);

    AddComponent<ComponentModel>("data/Game/Models/Player/Salaryman.mv1")
        ->SetAnimation({
            { "idle",          "data/Game/Models/Player/Anims/Idle.mv1", 0, 1.0f},
            { "walk",       "data/Game/Models/Player/Anims/Walking.mv1", 0, 3.3f},
            {"throw", "data/Game/Models/Player/Anims/Carry_Walking.mv1", 1, 1.0f}
    });

    // Randomize NPC stats / NPCのステータスをランダム化
    hp_        = 8 + (GetRand(12));
    strength_  = 0.7f + (GetRand(6) * 0.1f);
    moveSpeed_ = 0.6f + (GetRand(4) * 0.1f);

    SetTranslate({40 + (GetRand(60) - 30), 5, 40 + (GetRand(60) - 30)});

    auto col = AddComponent<ComponentCollisionCapsule>()    //
                   ->SetRadius(3.0f)
                   ->SetHeight(13.0f);

    col->UseGravity();
    col->SetCollisionGroup(ComponentCollision::CollisionGroup::PLAYER);

    //Same pickup component as the player, it works for any Object(プレイヤーと同じ拾うコンポーネント。どのObjectでも使える)
    AddComponent<ComponentPlayerPickUpTrigger>();

    return true;
}

void Npc::Update()
{
    Super::Update();

    switch(state_) {
    case State::SearchItem:
        UpdateSearchItem();
        break;

    case State::AimThrow:
        UpdateAimThrow();
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
    auto   items     = Scene::GetObjectsPtr<PoittersPoint::Item>();
    float3 myPos     = GetTranslate();
    float3 bestDir   = {0.0f, 0.0f, 0.0f};
    float  bestDist  = 1.0e30f;
    bool   foundItem = false;

    for(auto& item : items) {
        auto pickup = item->GetComponent<ComponentItemPickUp>();
        if(!pickup || pickup->IsHeld())
            continue;    //No pickup component, or someone is holding it(拾うコンポーネントがない、または誰かが持っている)

        float3 toItem = item->GetTranslate() - myPos;
        toItem.y      = 0.0f;    //Only look at the flat distance(平面の距離だけを見る)

        float dist = (float)length(toItem);
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
    if(roll < throwAtPlayerChance_) {
        aimAtPlayer_ = true;
        aimAtNpc_    = false;
        aimTarget_   = nullptr;
    }
    else if(roll < throwAtPlayerChance_ + throwAtNpcChance_) {
        aimAtPlayer_ = false;
        aimAtNpc_    = true;
        aimTarget_   = nullptr;
        // Find closest other NPC / 一番近い他のNPCを狙う
        auto   npcs     = Scene::GetObjectsPtr<PoittersPoint::Npc>();
        float3 myPos    = GetTranslate();
        float  bestDist = 1.0e30f;
        for(auto& npc : npcs) {
            if(npc.get() == this)
                continue;
            float3 to  = npc->GetTranslate() - myPos;
            to.y       = 0.0f;
            float dist = (float)length(to);
            if(dist < bestDist) {
                bestDist   = dist;
                aimTarget_ = npc.get();
            }
        }
    }
    else {
        aimAtPlayer_ = false;
        aimAtNpc_    = false;
        aimTarget_   = nullptr;
    }

    float angle = GetRand(359) * DegToRad;
    randomDir_  = float3(sinf(angle), 0.0f, cosf(angle));

    aimTimer_ = 0;
    state_    = State::AimThrow;
}

float3 Npc::GetAimDirection()
{
    if(aimAtPlayer_) {
        auto players = Scene::GetObjectsPtr<PoittersPoint::Player>();
        for(auto& player : players) {
            float3 toPlayer = player->GetTranslate() - GetTranslate();
            toPlayer.y      = 0.0f;

            if((float)length(toPlayer) > 0.01f)
                return normalize(toPlayer);
        }
    }
    else if(aimAtNpc_ && aimTarget_) {
        float3 toNpc = aimTarget_->GetTranslate() - GetTranslate();
        toNpc.y      = 0.0f;
        if((float)length(toNpc) > 0.01f)
            return normalize(toNpc);
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

    float3 aimDir = GetAimDirection();

    StopMove();
    FaceTowards(aimDir);
    aimTimer_++;

    //Throw when we face the target, or when we waited too long(ターゲットを向いたら投げる。待ちすぎても投げる)
    if(IsFacing(aimDir) || aimTimer_ >= aimTimeoutFrames_) {
        trigger->ThrowItem();

        cooldownTimer_ = cooldownFrames_;
        state_         = State::Cooldown;
    }
}

void Npc::UpdateCooldown()
{
    StopMove();

    cooldownTimer_--;
    if(cooldownTimer_ <= 0)
        state_ = State::SearchItem;
}

void Npc::MoveTowards(const float3& dir)
{
    //Move in the world direction, same call as ComponentStateIdleWalk(ワールド方向に移動。ComponentStateIdleWalkと同じ呼び方)
    AddTranslate(dir * moveSpeed_, true);

    FaceTowards(dir);

    if(auto mdl = GetComponent<ComponentModel>())
        mdl->PlayAnimationNoSame("walk", true);
}

void Npc::StopMove()
{
    if(auto mdl = GetComponent<ComponentModel>())
        mdl->PlayAnimationNoSame("idle", true);
}

void Npc::FaceTowards(const float3& dir)
{
    if(auto mdl = GetComponent<ComponentModel>())
        mdl->SetRotationToVectorWithLimit(dir, rotateSpeed_);
}

bool Npc::IsFacing(const float3& dir)
{
    auto mdl = GetComponent<ComponentModel>();
    if(!mdl)
        return true;

    //Same way ComponentItemPickUp::OnThrow gets the throw direction(ComponentItemPickUp::OnThrowが投げる方向を取るのと同じ方法)
    float3 facing = normalize(-mdl->GetWorldVectorAxisZ());

    return (float)dot(facing, dir) > 0.98f;
}

}    // namespace PoittersPoint
