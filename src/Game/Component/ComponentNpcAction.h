//---------------------------------------------------------------------------
//!	@file	ComponentNpcAction.h
//! @brief	[ADD] The NPC's "body": moves and plays animations the same way the Player does
//!         ([追加] NPCの「体」：プレイヤーと同じやり方で動いてアニメを再生する)
//!
//! The AI (Npc.cpp) only decides WHAT to do: "run there", "stop", "throw now".
//! This component decides HOW it looks: which animation, how fast, when the item leaves the hand.
//! It copies the Player's rules (Player.cpp), so NPCs lift, carry and throw exactly like the Player:
//!   - lifting   : "carryStart", speed matched to the lift time (big items = slow)
//!   - carrying  : "carryWalk" while moving, "carryHold" while standing
//!   - throwing  : "throw", the item leaves the hand at the arm-swing moment, then the animation finishes
//!   - free hands: "run" while moving, "idle" while standing
//!   - knocked down: ComponentKnockdown plays "down"/"getup", and this component does not play over it
//! (AI(Npc.cpp)は「何をするか」だけ決める：「あそこへ走る」「止まる」「今投げる」。
//!  このコンポーネントは「どう見えるか」を決める：どのアニメを、どの速さで、いつ物を手から離すか。
//!  プレイヤー(Player.cpp)と同じルールなので、NPCもプレイヤーと同じように持ち上げ・運び・投げる：
//!   - 持ち上げ中：「carryStart」。速さは持ち上げ時間に合わせる(大きい物ほどゆっくり)
//!   - 持っている：動いている時は「carryWalk」、立っている時は「carryHold」
//!   - 投げ      ：「throw」。腕を振った瞬間に物が手から離れ、アニメを最後まで再生する
//!   - 手ぶら    ：動いている時は「run」、立っている時は「idle」
//!   - ダウン中  ：ComponentKnockdownが「down」/「getup」を再生するので、このコンポーネントは上から再生しない)
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentNpcAction);

class ComponentNpcAction : public Component
{
public:
    BP_COMPONENT_DECL(ComponentNpcAction, u8"NPCの動き(アニメ付き)");

    void Init() override;

    //Throw timing: counts time and lets go of the item at the right moment(投げのタイミング：時間を数えて、ちょうどいい瞬間に物を離す)
    void Update() override;

    void GUI() override;

    //Run (or carry-walk) one step toward dir this frame. Does nothing while lifting or throwing
    //(このフレームでdirの方へ1歩走る(持っていれば運び歩き)。持ち上げ中・投げ中は何もしない)
    void Move(const float3& dir);

    //Stand still this frame (idle / carryHold / lifting animation)(このフレームは立ち止まる(idle/carryHold/持ち上げアニメ))
    void Stop();

    //Turn toward dir, a little every frame(dirの方へ、毎フレーム少しずつ向く)
    void Face(const float3& dir);

    //True if the model already looks toward dir(モデルがもうdirの方を向いていればtrue)
    bool IsFacing(const float3& dir) const;

    //Start the throw animation. The item leaves the hand later, at the release moment.
    //Returns false if we can not throw now (no item, still lifting, already throwing)
    //(投げアニメを始める。物はあとの「離す瞬間」に手から離れる。
    // 今投げられない時(物がない・持ち上げ中・もう投げている)はfalse)
    bool StartThrow();

    //True from StartThrow() until the throw animation is finished(StartThrow()から投げアニメが終わるまでtrue)
    bool IsThrowing() const { return throwTimer_ > 0.0f; }

    //Lifting or throwing: the body is busy and can not walk(持ち上げ中か投げ中：体がふさがっていて歩けない)
    bool IsBusy() const;

    //Can not act (countdown, knocked down, K.O., match over): cancel the throw and stand still.
    //While the down/getup animation is playing, the model is left alone
    //(動けない(カウントダウン・ダウン・K.O.・試合終了)：投げを取り消して立ち止まる。
    // down/getupのアニメ中は、モデルに何もしない)
    void Freeze();

    //Throw animation speed (Player = 1.0) and when the item leaves the hand (0.0-1.0). From NpcCharacterDef
    //(投げアニメの速さ(プレイヤー＝1.0)と、物が手から離れる瞬間(0.0〜1.0)。NpcCharacterDefから)
    ComponentNpcActionPtr SetThrowTiming(float animSpeed, float releaseRatio);

private:
    //Pick and play the right animation for now. moving = we stepped this frame
    //(今に合ったアニメを選んで再生する。moving＝このフレームで歩いた)
    void PlayStateAnimation(bool moving);

    //Animation time runs at 30 per second (same as Player.cpp)(アニメの時間は1秒に30進む(Player.cppと同じ))
    static constexpr float ANIM_FPS = 30.0f;

    float rotateSpeed_ = 20.0f;    //Turn speed, same as the Player(回転の速さ。プレイヤーと同じ)

    //Throw(投げ)
    float throwAnimSpeed_    = 1.0f;             //Throw animation speed(投げアニメの速さ)
    float throwReleaseRatio_ = 28.0f / 82.0f;    //Item leaves the hand at this part of the animation(アニメのこの割合の所で物が手から離れる)
    float throwTimer_        = 0.0f;             //Seconds since the throw started. 0 = not throwing(投げ始めてからの秒数。0＝投げていない)
    float throwReleaseTime_  = 0.4f;             //Seconds until the item leaves the hand(物が手から離れるまでの秒数)
    float throwAnimTime_     = 1.1f;             //Seconds of the whole throw animation(投げアニメ全体の秒数)
    bool  throwReleased_     = false;            //The item already left the hand in this throw(この投げで物はもう手から離れた)

    //Lifting animation speed limits, same as the Player(持ち上げアニメの速さの上限下限。プレイヤーと同じ)
    float minLiftAnimSpeed_ = 0.30f;
    float maxLiftAnimSpeed_ = 4.00f;
    bool  liftAnimStarted_  = false;    //carryStart speed already set for this lift(この持ち上げでcarryStartの速さはもう決めた)

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentNpcAction, 1);
