//---------------------------------------------------------------------------
//!	@file	NpcCharacterDef.h
//! @brief	[ADD] Data of each NPC character (model, animations, stats)
//!         ([追加] NPCキャラクターごとのデータ(モデル・アニメ・ステータス))
//!
//! One entry = one character from the task list "Character" (cha0002 CVS, cha0003 POLICE, cha0004 JGSDF; cha0005 BANK later).
//! To add a new NPC character: add an id to NpcCharacterId and one more "case" in GetNpcCharacterDef().
//! The Npc class reads everything from here, so the AI code does not change.
//! (1つのデータ＝タスクリスト「Character」の1キャラ(cha0002 CVS、cha0003 POLICE、cha0004 JGSDF。cha0005 BANKは今後)。
//!  新しいNPCキャラを足す時は、NpcCharacterIdにIDを足して、GetNpcCharacterDef()に「case」を1つ足す。
//!  Npcクラスはすべてここから読むので、AIのコードは変えなくてよい)
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>
#include <vector>

namespace PoittersPoint {

//Which NPC character(どのNPCキャラか)
enum class NpcCharacterId
{
    CVS,       //cha0002 バイト(CVS)
    POLICE,    //cha0003 警察官(POLICE)   [ADD]([追加])
    JGSDF,     //cha0004 軍隊(JGSDF)      [ADD]([追加])

    Count
};

//Everything that makes one NPC character different(1人のNPCキャラを他と違うものにする全部のデータ)
struct NpcCharacterDef
{
    const char* id;           //Task list ID, e.g. "cha0002"(タスクリストのID。例「cha0002」)
    const char* name;         //Short English name, e.g. "CVS"(短い英語名。例「CVS」)
    const char* modelPath;    //Character model (.mv1)(キャラのモデル(.mv1))

    //Animations. Names the code uses: idle, run, carryStart, carryWalk, carryHold, throw, down, getup
    //(アニメ。コードが使う名前：idle, run, carryStart, carryWalk, carryHold, throw, down, getup)
    std::vector<Animation::Desc> animations;

    //Stats. Each NPC gets a small random spread so they do not all play the same
    //(ステータス。NPCごとに少しランダムに散らして、全員が同じ動きにならないようにする)
    int   hpMin;    //HP = hpMin + random(0..hpRandom)
    int   hpRandom;
    float strengthMin;    //Strength = strengthMin + random(0..strengthSteps) x 0.1 (lift speed)(筋力。持ち上げの速さ)
    int   strengthSteps;
    float speedMin;    //Move per frame = speedMin + random(0..speedSteps) x 0.05 (Player = 1.0)(1フレームの移動量。プレイヤー＝1.0)
    int   speedSteps;

    //Throw(投げ)
    float throwAnimSpeed;       //Throw animation speed. Player = 1.0, bigger = faster throw(投げアニメの速さ。プレイヤー＝1.0、大きいほど速く投げる)
    float throwReleaseRatio;    //When the item leaves the hand, 0.0-1.0 of the throw animation(投げアニメの何割の所で手から離れるか)

    //Getting hit(当たった時)
    float knockbackScale;    //Knockback distance x this. 1.0 = normal, bigger = blown further(ふっとぶ距離×これ。1.0＝普通、大きいほど遠くへ飛ぶ)

    //[ADD] Can catch/guard items thrown from the front (ComponentCatchGuard). cha0003 POLICE only
    //([追加] 前から投げられた物をキャッチ・ガードできる(ComponentCatchGuard)。cha0003 POLICEだけ)
    bool catchGuard = false;
};

//The data of one character(1キャラのデータ)
inline const NpcCharacterDef& GetNpcCharacterDef(NpcCharacterId id)
{
    //cha0002 バイト(CVS): fast legs, the fastest throw motion, but low HP and easily blown away
    //(cha0002 バイト(CVS)：足が速く、投げるモーションも最速。ただしHPが低く吹き飛びやすい)
    //CVS.mv1 has the same bones as the Salaryman, so the Player's carry/throw animations fit it too
    //(CVS.mv1はサラリーマンと同じボーンなので、プレイヤーの持つ・投げるアニメもそのまま使える)
    static const float CVS_THROW_SPEED = 2.5f;    //2.5x the Player = the fastest throw(プレイヤーの2.5倍＝最速の投げ)

    static const NpcCharacterDef CVS = {
        "cha0002",
        "CVS",
        "data/Game/Models/Enemy/CVS/CVS.mv1",
        {
          // name          file                                                    index speed
            {"idle", "data/Game/Models/Enemy/CVS/Anims/Idle.mv1", 0, 1.0f},                 // 待機 / idle
            {"run", "data/Game/Models/Enemy/CVS/Anims/Run.mv1", 0, 1.3f},                   // [新規] 走る / run (tools/AnimMaker/make_cvs.py)
            {"carryStart", "data/Game/Models/Player/OR_Anims/Carry_start.mv1", 0, 1.0f},    // 持ち上げ / lift (Player)
            {"carryWalk", "data/Game/Models/Player/OR_Anims/Carry_walk.mv1", 3, 1.8f},      // 持って歩く / carry walk (Player)
            {"carryHold", "data/Game/Models/Player/OR_Anims/Carry_start.mv1", 0, 1.0f},     // 持って立つ / carry hold (Player)
            {"throw", "data/Game/Models/Player/OR_Anims/Throw.mv1", 0, CVS_THROW_SPEED},    // 投げる / throw (Player, faster)
            {"down", "data/Game/Models/Enemy/CVS/Anims/Down.mv1", 0, 1.0f},                 // [新規] 大きく吹き飛ぶ / big blow-away
            {"getup", "data/Game/Models/Enemy/CVS/Anims/GetUp.mv1", 0, 1.0f},               // [新規] 跳ね起きる / hop-up
        },
        6, //HP 6-9: low (Player 10-19)(HP6〜9：低い(プレイヤーは10〜19))
        3,
        0.8f, //Strength 0.8-1.2: average(筋力0.8〜1.2：ふつう)
        4,
        1.15f, //Speed 1.15-1.35: faster than the Player (1.0)(速さ1.15〜1.35：プレイヤー(1.0)より速い)
        4,
        CVS_THROW_SPEED,
        28.0f / 82.0f, //Same release moment as the Player's throw animation(プレイヤーの投げアニメと同じ離す瞬間)
        1.6f, //Blown 1.6x further(1.6倍遠くへ吹き飛ぶ)
    };

    //-----------------------------------------------------------------------
    //[ADD] cha0003 警察官(POLICE): high defense, good at catching/guarding thrown items
    //More HP, hard to blow away, and [ADD] catches/guards items thrown from the front (ComponentCatchGuard):
    //  free hands -> CATCH and throw it right back, busy hands or a heavy item -> GUARD (less damage, no fall-down)
    //Note: POLICE/Anims/Idle.mv1 has NO animation inside (0 animations), so the Salaryman's Idle is borrowed
    //([追加] cha0003 警察官(POLICE)：防御力が高く、投げられた物をキャッチ・ガードしやすい
    // HPが多く、吹き飛びにくい。[追加] 前から投げられた物をキャッチ・ガードする(ComponentCatchGuard)：
    //   手が空いている→キャッチしてすぐ投げ返す、手がふさがっている・重い物→ガード(ダメージが減る、倒れない)
    // 注意：POLICE/Anims/Idle.mv1の中にはアニメが入っていない(0個)ので、サラリーマンのIdleを借りている)
    //-----------------------------------------------------------------------
    static const NpcCharacterDef POLICE = {
        "cha0003",
        "POLICE",
        "data/Game/Models/Enemy/POLICE/Police.mv1",
        {
          // name          file                                                    index speed
            {"idle", "data/Game/Models/Player/Anims/Idle.mv1", 0, 1.0f},                    // 待機 (借り物/borrowed: Salaryman)
            {"run", "data/Game/Models/Enemy/POLICE/Anims/Running.mv1", 0, 1.0f},            // 走る / run (POLICE)
            {"carryStart", "data/Game/Models/Player/OR_Anims/Carry_start.mv1", 0, 1.0f},    // 持ち上げ / lift (Player)
            {"carryWalk", "data/Game/Models/Player/OR_Anims/Carry_walk.mv1", 3, 1.5f},      // 持って歩く / carry walk (Player)
            {"carryHold", "data/Game/Models/Player/OR_Anims/Carry_start.mv1", 0, 1.0f},     // 持って立つ / carry hold (Player)
            {"throw", "data/Game/Models/Player/OR_Anims/Throw.mv1", 0, 1.3f},               // 投げる / throw (Player)
            {"down", "data/Game/Models/Player/OR_Anims/Down.mv1", 0, 1.0f},                 // 倒れる / down (Player)
            {"getup", "data/Game/Models/Player/OR_Anims/GetUp.mv1", 0, 1.0f},               // 起き上がる / get up (Player)
        },
        14, //HP 14-19: tough(HP14〜19：タフ)
        5,
        0.9f, //Strength 0.9-1.2(筋力0.9〜1.2)
        3,
        0.95f, //Speed 0.95-1.1: about the Player(速さ0.95〜1.1：プレイヤーくらい)
        3,
        1.3f, //Throw a little faster than the Player(投げはプレイヤーより少し速い)
        28.0f / 82.0f,
        0.7f, //Hard to blow away(吹き飛びにくい)
        true, //[ADD] Catch/guard from the front([追加] 前からの物をキャッチ・ガード)
    };

    //-----------------------------------------------------------------------
    //[ADD] cha0004 軍隊(JGSDF): lifts big objects (big rocks) fast and throws very far, but moves slowly
    //For now: very strong (fast lifting) and slow. Throwing "very far" is NOT made yet (a later task for this character)
    //Only Idle exists for JGSDF, so it walks with the Salaryman's Walking
    //([追加] cha0004 軍隊(JGSDF)：大型オブジェクト(大岩など)を高速で持ち上げ、超遠くまで投げられる。移動速度は遅め
    // 今は：とても力持ち(持ち上げが速い)で遅い。「超遠くまで投げる」はまだ作っていない(このキャラの今後の作業)
    // JGSDFにはIdleしかないので、サラリーマンのWalkingで歩く)
    //-----------------------------------------------------------------------
    static const NpcCharacterDef JGSDF = {
        "cha0004",
        "JGSDF",
        "data/Game/Models/Enemy/JGSDF/Jgsdf.mv1",
        {
          // name          file                                                    index speed
            {"idle", "data/Game/Models/Enemy/JGSDF/Anims/Idle.mv1", 0, 1.0f},               // 待機 / idle (JGSDF)
            {"run", "data/Game/Models/Player/Anims/Walking.mv1", 0, 2.6f},                  // 歩く / walk (借り物/borrowed: Salaryman)
            {"carryStart", "data/Game/Models/Player/OR_Anims/Carry_start.mv1", 0, 1.0f},    // 持ち上げ / lift (Player)
            {"carryWalk", "data/Game/Models/Player/OR_Anims/Carry_walk.mv1", 3, 1.2f},      // 持って歩く / carry walk (Player)
            {"carryHold", "data/Game/Models/Player/OR_Anims/Carry_start.mv1", 0, 1.0f},     // 持って立つ / carry hold (Player)
            {"throw", "data/Game/Models/Player/OR_Anims/Throw.mv1", 0, 1.2f},               // 投げる / throw (Player)
            {"down", "data/Game/Models/Player/OR_Anims/Down.mv1", 0, 1.0f},                 // 倒れる / down (Player)
            {"getup", "data/Game/Models/Player/OR_Anims/GetUp.mv1", 0, 1.0f},               // 起き上がる / get up (Player)
        },
        12, //HP 12-17(HP12〜17)
        5,
        1.8f, //Strength 1.8-2.2: lifts big rocks fast(筋力1.8〜2.2：大岩も速く持ち上げる)
        4,
        0.7f, //Speed 0.7-0.85: slow(速さ0.7〜0.85：遅め)
        3,
        1.2f,
        28.0f / 82.0f,
        0.8f, //A bit hard to blow away(少し吹き飛びにくい)
    };

    switch(id) {
    case NpcCharacterId::POLICE:
        return POLICE;
    case NpcCharacterId::JGSDF:
        return JGSDF;
    case NpcCharacterId::CVS:
    default:
        return CVS;
    }
}

}    // namespace PoittersPoint
