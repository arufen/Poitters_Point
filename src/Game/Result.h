//---------------------------------------------------------------------------
//!	@file	Result.h
//! @brief	Result scene: who won, the ranking, and Retry / Title
//!         (リザルトシーン：誰が勝ったか、ランキング、リトライ/タイトル)
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>

namespace PoittersPoint {

class Result : public Scene::Base
{
public:
    BP_CLASS_DECL(Result, u8"Result");

    //! @brief 初期化
    //! @return 初期化済み
    bool Init() override;

    void Update() override;

    void LateDraw() override;

    void Exit() override;

private:
    enum class State
    {
        FadeIn,     //Coming out of black(暗転から明るくなる)
        Menu,       //Choosing Retry / Title(リトライ/タイトルを選ぶ)
        FadeOut,    //Going to black, then change scene(暗転して、シーンを切り替える)
        Done,
    };

    enum class Choice
    {
        Retry,
        Title,
        Count
    };

    //Go to the chosen scene. Scenes are cached, so release the old one first to get a fresh start
    //(選んだシーンへ行く。シーンはキャッシュされるので、古いものを先に解放して最初からやり直す)
    void GoToChoice();

    void DrawCentered(const char* text, int y, int font, unsigned int color);

    State  state_      = State::FadeIn;
    Choice choice_     = Choice::Retry;
    float  stateTimer_ = 0.0f;
    float  blinkTimer_ = 0.0f;

    int fontTitle_ = -1;    //YOU WIN! etc.(YOU WIN!など)
    int fontText_  = -1;    //Ranking and menu(ランキングとメニュー)
    int fontSmall_ = -1;    //Hints(操作説明)
};

}    // namespace PoittersPoint
