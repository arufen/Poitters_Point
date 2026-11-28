//---------------------------------------------------------------------------
//!	@file	Npc.h
//! @brief	Npc (AI controlled character)
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>
#include "NpcCharacterDef.h"

namespace PoittersPoint {
USING_PTR(Npc);

//[CHANGE] The NPC now moves like the Player. This class is the "brain" (AI: what to do).
//The "body" (animations, lifting, throwing) is ComponentNpcAction, and the character data
//(model, animations, stats) comes from NpcCharacterDef.h (cha0002 バイト(CVS) for now).
//([変更] NPCがプレイヤーと同じように動くようになった。このクラスは「頭」(AI：何をするか)。
// 「体」(アニメ・持ち上げ・投げ)はComponentNpcActionで、キャラのデータ(モデル・アニメ・ステータス)は
// NpcCharacterDef.hから来る(今はcha0002 バイト(CVS)))
class Npc : public Object
{
public:
    BP_OBJECT_DECL(Npc, "PoittersPoint::Npc");

    //Initialize(初期化)
    bool Init() override;

    //[ADD] Become this character: model, animations and stats. Init() calls it with cha0002 CVS.
    //Can be called again after Create<Npc>() to switch to another character (e.g. from MainStage)
    //([追加] このキャラになる：モデル・アニメ・ステータス。Init()がcha0002 CVSで呼ぶ。
    // Create<Npc>()の後にもう一度呼べば、別のキャラに変えられる(例：MainStageから))
    void ApplyCharacter(NpcCharacterId id);

    //[ADD] The character data in use([追加] 使っているキャラのデータ)
    const NpcCharacterDef& GetCharacterDef() const { return GetNpcCharacterDef(characterId_); }

    //Update the AI every frame(毎フレームAIを更新する)
    void Update() override;

    //Stats (HP, strength, speed) live in ComponentCharacterStats now(ステータスはComponentCharacterStatsに入っている)
    //This just passes the damage to it(ダメージをそこへ渡すだけ)
    void TakeDamage(int dmg);

    //Chance to throw at other NPCs, 0.0 - 1.0 (他のNPCに投げる確率 0.0〜1.0)
    void SetThrowAtNpcChance(float c) { throwAtNpcChance_ = c; }

private:
    //AI states(AIの状態)
    enum class State
    {
        SearchItem,    //Walk to the closest item and pick it up(一番近いアイテムへ歩いて拾う)
        AimThrow,      //Turn to the target and throw(ターゲットの方を向いて投げる)
        Throwing,      //[ADD] The throw animation is playing, wait for it to end([追加] 投げアニメ中。終わるまで待つ)
        Cooldown,      //Wait a little after throwing(投げたあと少し待つ)
    };

    //Who we throw at(誰に投げるか)
    enum class AimMode
    {
        Player,    //Throw at the player(プレイヤーに投げる)
        Npc,       //Throw at the closest other NPC(一番近い他のNPCに投げる)
        Random,    //Throw in a random direction(ランダムな方向に投げる)
    };

    //State functions(状態ごとの処理)
    void UpdateSearchItem();
    void UpdateAimThrow();
    void UpdateThrowing();    //[ADD]([追加])
    void UpdateCooldown();

    //Decide the target when we start aiming(ねらい始める時にターゲットを決める)
    void StartAim();

    //Direction to throw. Target direction or the random direction(投げる方向。ターゲットの方向かランダムな方向)
    float3 GetAimDirection();

    //Move one step to the direction. [CHANGE] ComponentNpcAction picks run/carryWalk(その方向に1歩進む。[変更] run/carryWalkはComponentNpcActionが選ぶ)
    void MoveTowards(const float3& dir);

    //Stand still. [CHANGE] ComponentNpcAction picks idle/carryHold/lifting(止まる。[変更] idle/carryHold/持ち上げはComponentNpcActionが選ぶ)
    void StopMove();

    //Turn the model to the direction(モデルをその方向に向ける)
    void FaceTowards(const float3& dir);

    //True if the model is already looking at the direction(モデルがすでにその方向を向いていればtrue)
    bool IsFacing(const float3& dir);

private:
    State state_ = State::SearchItem;

    NpcCharacterId characterId_ = NpcCharacterId::CVS;    //[ADD] Which character this NPC is([追加] このNPCがどのキャラか)

    //Chance to throw at the player or at other NPCs. The rest is a random direction(プレイヤーかNPCに投げる確率。残りはランダムな方向)
    float throwAtPlayerChance_ = 0.7f;
    float throwAtNpcChance_    = 0.3f;

    AimMode    aimMode_ = AimMode::Player;
    NpcWeakPtr aimTarget_;    //Weak, so it is safe if that NPC is gone(弱参照なので、そのNPCが消えても安全)
    float3     randomDir_ = {0.0f, 0.0f, 1.0f};

    //Timers are counted in frames(タイマーはフレーム数で数える)
    int aimTimer_         = 0;
    int aimTimeoutFrames_ = 120;    //Throw anyway after this many frames(このフレーム数を過ぎたら向いていなくても投げる)

    //Pause after picking up, before aiming. Random so NPCs do not all act the same (60 frames = about 1 second)
    //(拾ったあと、ねらう前の一時停止。NPCごとに違う動きになるようランダム。60フレーム＝約1秒)
    int holdTimer_        = 0;
    int holdMinFrames_    = 45;    //Shortest pause(一番短い待ち時間)
    int holdRandomFrames_ = 60;    //Random extra pause, 0 to this value(ランダムで足される待ち時間 0〜この値)

    int cooldownTimer_  = 0;
    int cooldownFrames_ = 90;    //Wait after the throw(投げたあとの待ち時間)
};
}    // namespace PoittersPoint
