#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>
#include <string>
#include <vector>

USING_PTR(ComponentMatch);

//The match: 3-2-1-GO! countdown -> timer -> FINISH! / TIME UP! -> fade -> Result scene.
//Also remembers the K.O. order for the ranking and makes K.O.'d characters drop their item.
//Draws its UI at the very last draw step (ProcTiming::UI), so it is on top of the HP bars and the fade covers everything.
//(試合：3-2-1-GO!のカウントダウン→タイマー→FINISH!/TIME UP!→暗転→Resultシーン。
// ランキングのためにK.O.の順番も覚えて、K.O.されたキャラにアイテムを落とさせる。
// UIは一番最後の描画(ProcTiming::UI)で描くので、HPバーより上に出て、暗転は全部をおおう)
class ComponentMatch : public Component
{
public:
    BP_COMPONENT_DECL(ComponentMatch, u8"試合の進行");

    void Init() override;

    void Update() override;

    void Exit() override;

    void GUI() override;

    //Can this character move and throw right now? False during the countdown, after K.O., and after the match ended.
    //True when there is no match at all (test scenes), so other scenes are not affected.
    //(このキャラは今、動いたり投げたりできるか？カウントダウン中・K.O.後・試合終了後はfalse。
    // 試合がない時(テストシーン)はtrue。ほかのシーンに影響しないように)
    static bool CanAct(::Object* character);

private:
    enum class State
    {
        Countdown,    //3, 2, 1(3、2、1)
        Playing,      //Timer running(タイマーが動いている)
        Finished,     //FINISH! / TIME UP! banner(FINISH!/TIME UP!の表示)
        FadeOut,      //Going to black(暗転中)
        Done,         //Scene change requested(シーン切り替えを頼んだ)
    };

    //One character in the match(試合に出ている1人)
    struct Fighter
    {
        std::weak_ptr<::Object> object;
        std::string             label;    //"PLAYER", "NPC 1"... same as the HP bar(HPバーと同じ名前)
        bool                    isPlayer   = false;
        bool                    knockedOut = false;
        int                     koOrder    = 0;    //1 = first one knocked out(1＝最初にK.O.された人)
    };

    //Collect the player and the NPCs, in the same order as the HP bar(HPバーと同じ順番でプレイヤーとNPCを集める)
    void CollectFighters();

    //Find new K.O.s: remember the order and drop their item(新しいK.O.を見つける：順番を覚えて、アイテムを落とさせる)
    void CheckKnockouts();

    int CountAlive() const;

    //End the match and write the ranking into LastMatchResult()(試合を終わらせて、ランキングをLastMatchResult()に書く)
    void Finish(bool timeUp);

    //UI drawing, registered at ProcTiming::UI in Init()(UIの描画。Init()でProcTiming::UIに登録する)
    void DrawUI();
    void DrawCountdown();
    void DrawTimer();
    void DrawBanner();
    void DrawFade();

    //Text centered on the screen at height y, with scale(画面の真ん中、高さyにスケールつきで文字を描く)
    void DrawCentered(const char* text, int y, int font, unsigned int color, float scale = 1.0f);

    State                state_      = State::Countdown;
    float                stateTimer_ = 0.0f;      //Time spent in the current state(今の状態の経過時間)
    float                timeLeft_   = 120.0f;    //Match time left(試合の残り時間)
    float                goTimer_    = 0.0f;      //"GO!" still showing while > 0(0より大きい間「GO!」を表示)
    bool                 timeUp_     = false;
    bool                 collected_  = false;
    int                  koCount_    = 0;
    std::vector<Fighter> fighters_;

    //Font handles, made once in Init and deleted in Exit(フォントハンドル。Initで1回作り、Exitで消す)
    int fontHuge_  = -1;    //Countdown numbers(カウントダウンの数字)
    int fontBig_   = -1;    //FINISH! / TIME UP!
    int fontTimer_ = -1;    //Match timer(試合タイマー)
    int fontSmall_ = -1;    //"TIME" label(「TIME」の文字)

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentMatch, 4);
