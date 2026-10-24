//---------------------------------------------------------------------------
//!	@file	Npc.h
//! @brief	Npc (AI controlled character)
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>
#include <string>

namespace PoittersPoint {
USING_PTR(Npc);
class Npc : public Object
{
public:
    BP_OBJECT_DECL(Npc, "PoittersPoint::Npc");

    //Initialize(初期化)
    bool Init() override;

    //Update the AI every frame(毎フレームAIを更新する)
    void Update() override;

private:
    //AI states(AIの状態)
    enum class State
    {
        SearchItem,    //Walk to the closest item and pick it up(一番近いアイテムへ歩いて拾う)
        AimThrow,      //Turn to the target and throw(ターゲットの方を向いて投げる)
        Cooldown,      //Wait a little after throwing(投げたあと少し待つ)
    };

    //State functions(状態ごとの処理)
    void UpdateSearchItem();
    void UpdateAimThrow();
    void UpdateCooldown();

    //Decide the target when we start aiming(ねらい始める時にターゲットを決める)
    void StartAim();

    //Direction to throw. Player direction or the random direction(投げる方向。プレイヤーの方向かランダムな方向)
    float3 GetAimDirection();

    //Move one step to the direction and play walk animation(その方向に1歩進んで歩きアニメを流す)
    void MoveTowards(const float3& dir);

    //Stop and play idle animation(止まって待機アニメを流す)
    void StopMove();

    //Turn the model to the direction(モデルをその方向に向ける)
    void FaceTowards(const float3& dir);

    //True if the model is already looking at the direction(モデルがすでにその方向を向いていればtrue)
    bool IsFacing(const float3& dir);

    //Stats for NPCs (NPCステータス) - similar to player
    float GetSpeed() const { return moveSpeed_; }
    void  SetSpeed(float s) { moveSpeed_ = s; }
    int   GetHP() const { return hp_; }
    void  SetHP(int hp) { hp_ = hp; }
    float GetStrength() const { return strength_; }
    void  SetStrength(float str) { strength_ = str; }
    // Can throw at NPC or player (NPC同士にも投げられるようにする)
    void SetThrowAtNpcChance(float c) { throwAtNpcChance_ = c; }

private:
    //Unique name. ComponentAttachModel finds the hand by the object name(固有の名前。ComponentAttachModelはオブジェクト名で手を探す)
    std::string npcName_;

    State state_ = State::SearchItem;

    float moveSpeed_   = 0.8f;     //Move amount per frame, same style as the player(1フレームの移動量。プレイヤーと同じ方式)
    float rotateSpeed_ = 20.0f;    //Turn speed(回転速度)

    //Stats / ステータス
    int   hp_       = 12;
    float strength_ = 0.9f;

    //Chance to throw at the player. 0.0 - 1.0. Otherwise a random direction(プレイヤーに向けて投げる確率。0.0〜1.0。外れたらランダムな方向)
    float throwAtPlayerChance_ = 0.7f;
    // Chance to throw at other NPCs / NPCに投げる確率
    float throwAtNpcChance_ = 0.3f;

    bool    aimAtPlayer_ = true;
    bool    aimAtNpc_    = false;
    Object* aimTarget_   = nullptr;
    float3  randomDir_   = {0.0f, 0.0f, 1.0f};

    //Timers are counted in frames(タイマーはフレーム数で数える)
    int aimTimer_         = 0;
    int aimTimeoutFrames_ = 120;    //Throw anyway after this many frames(このフレーム数を過ぎたら向いていなくても投げる)

    int cooldownTimer_  = 0;
    int cooldownFrames_ = 90;    //Wait after the throw(投げたあとの待ち時間)
};
}    // namespace PoittersPoint
