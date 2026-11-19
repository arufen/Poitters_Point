//---------------------------------------------------------------------------
//!	@file	Result.cpp
//! @brief	Result scene: who won, the ranking, and Retry / Title
//!         (リザルトシーン：誰が勝ったか、ランキング、リトライ/タイトル)
//---------------------------------------------------------------------------
#include "Result.h"
#include "MatchResult.h"
#include "MainStage.h"
#include "Title.h"
#include "StageConfig.h"
#include <cmath>
#include <cstring>
#include <string>

namespace PoittersPoint {

namespace {
const unsigned int COLOR_WHITE  = 0xFFFFFF;
const unsigned int COLOR_GRAY   = 0x8C8C8C;
const unsigned int COLOR_YELLOW = 0xFFD200;
const unsigned int COLOR_RED    = 0xFF3C3C;
const unsigned int COLOR_BLUE   = 0x5AA0FF;
const unsigned int COLOR_EDGE   = 0x000000;

const char* RANK_NAMES[] = {"1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th"};
}    // namespace

bool Result::Init()
{
    state_      = State::FadeIn;
    choice_     = Choice::Retry;
    stateTimer_ = 0.0f;
    blinkTimer_ = 0.0f;

    //Own font handles, so the global font size (used by the Title scene) is never changed(自分のフォントハンドル。Titleシーンが使うグローバルのフォントサイズは変えない)
    fontTitle_ = CreateFontToHandle(nullptr, 96, 9, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 5);
    fontText_  = CreateFontToHandle(nullptr, 32, 6, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 2);
    fontSmall_ = CreateFontToHandle(nullptr, 20, 5, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 2);

    return true;
}

void Result::Exit()
{
    for(int* font : {&fontTitle_, &fontText_, &fontSmall_}) {
        if(*font != -1) {
            DeleteFontToHandle(*font);
            *font = -1;
        }
    }

    Super::Exit();
}

void Result::Update()
{
    float delta  = GetDeltaTime();
    stateTimer_ += delta;
    blinkTimer_ += delta;

    switch(state_) {
    case State::FadeIn:
        if(stateTimer_ >= StageConfig::SCENE_FADE_TIME) {
            state_      = State::Menu;
            stateTimer_ = 0.0f;
        }
        break;

    case State::Menu:
        {
            //Up/Down or W/S to choose, Enter to confirm (Title also uses Enter)(上下かW/Sで選んで、Enterで決定(Titleと同じEnter))
            int c     = (int)choice_;
            int count = (int)Choice::Count;
            if(Input::IsKeyDown(KEY_INPUT_UP) || Input::IsKeyDown(KEY_INPUT_W))
                c = (c + count - 1) % count;
            if(Input::IsKeyDown(KEY_INPUT_DOWN) || Input::IsKeyDown(KEY_INPUT_S))
                c = (c + 1) % count;
            choice_ = (Choice)c;

            if(Input::IsKeyDown(KEY_INPUT_RETURN)) {
                state_      = State::FadeOut;
                stateTimer_ = 0.0f;
            }
            break;
        }

    case State::FadeOut:
        if(stateTimer_ >= StageConfig::SCENE_FADE_TIME) {
            state_ = State::Done;
            GoToChoice();
        }
        break;

    case State::Done:
        break;
    }
}

void Result::GoToChoice()
{
    if(choice_ == Choice::Retry) {
        Scene::ReleaseScene(Scene::GetScene<PoittersPoint_MainStage>());
        Scene::Change(Scene::GetScene<PoittersPoint_MainStage>());
    }
    else {
        Scene::ReleaseScene(Scene::GetScene<Title>());
        Scene::Change(Scene::GetScene<Title>());
    }
}

void Result::DrawCentered(const char* text, int y, int font, unsigned int color)
{
    int w = GetDrawStringWidthToHandle(text, (int)strlen(text), font);
    DrawStringToHandle((WINDOW_W - w) / 2, y, text, color, font, COLOR_EDGE);
}

void Result::LateDraw()
{
    const MatchResult& result = LastMatchResult();

    //Background(背景)
    DrawBox(0, 0, WINDOW_W, WINDOW_H, GetColor(20, 24, 40), TRUE);

    //Big outcome text(大きい結果の文字)
    const char*  title = "DRAW";
    unsigned int color = COLOR_WHITE;
    if(result.outcome == MatchResult::Outcome::Win) {
        title = "YOU WIN!";
        color = COLOR_YELLOW;
    }
    else if(result.outcome == MatchResult::Outcome::Lose) {
        title = "YOU LOSE...";
        color = COLOR_BLUE;
    }
    int y = (int)(WINDOW_H * 0.10f);
    DrawCentered(title, y, fontTitle_, color);

    //Why it ended + who won(終わった理由と勝者)
    std::string sub = result.reason == MatchResult::Reason::TimeUp ? "TIME UP" : "K.O.";
    if(!result.winner.empty())
        sub += "  -  WINNER: " + result.winner;
    y += GetFontSizeToHandle(fontTitle_) + 16;
    DrawCentered(sub.c_str(), y, fontText_, COLOR_WHITE);

    //Ranking table(ランキング表)
    const int rowH    = 46;
    const int tableW  = 560;
    const int left    = (WINDOW_W - tableW) / 2;
    y                += 70;

    for(size_t i = 0; i < result.ranking.size(); ++i) {
        const auto& e    = result.ranking[i];
        int         rowY = y + (int)i * rowH;

        //Highlight the human player's row(人間のプレイヤーの行を目立たせる)
        if(e.isPlayer) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawBox(left - 12, rowY - 6, left + tableW + 12, rowY + rowH - 10, GetColor(255, 210, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        unsigned int nameColor = e.isPlayer ? COLOR_YELLOW : COLOR_WHITE;
        const char*  rank      = i < std::size(RANK_NAMES) ? RANK_NAMES[i] : "-";

        DrawStringToHandle(left, rowY, rank, nameColor, fontText_, COLOR_EDGE);
        DrawStringToHandle(left + 110, rowY, e.label.c_str(), nameColor, fontText_, COLOR_EDGE);

        //Right side: K.O. or HP left(右側：K.O.か残りHP)
        std::string  hp      = e.knockedOut ? "K.O." : "HP " + std::to_string(e.hp) + " / " + std::to_string(e.maxHp);
        unsigned int hpColor = e.knockedOut ? COLOR_RED : COLOR_WHITE;
        int          hpW     = GetDrawStringWidthToHandle(hp.c_str(), (int)hp.size(), fontText_);
        DrawStringToHandle(left + tableW - hpW, rowY, hp.c_str(), hpColor, fontText_, COLOR_EDGE);
    }

    //Menu(メニュー)
    y                   = (int)(WINDOW_H * 0.78f);
    const char* items[] = {"RETRY", "TITLE"};
    for(int i = 0; i < (int)Choice::Count; ++i) {
        bool        selected = (int)choice_ == i;
        std::string text     = selected ? std::string("> ") + items[i] + " <" : items[i];

        //The selected one blinks while waiting(選ばれている方は待っている間点滅する)
        unsigned int c = selected ? COLOR_YELLOW : COLOR_GRAY;
        if(selected && state_ == State::Menu && fmodf(blinkTimer_, 0.8f) > 0.6f)
            c = COLOR_WHITE;

        DrawCentered(text.c_str(), y + i * 44, fontText_, c);
    }
    DrawCentered("W/S or UP/DOWN : choose    ENTER : decide", WINDOW_H - 40, fontSmall_, COLOR_GRAY);

    //Fade in / out(明転・暗転)
    float a = 0.0f;
    if(state_ == State::FadeIn)
        a = 1.0f - stateTimer_ / StageConfig::SCENE_FADE_TIME;
    else if(state_ == State::FadeOut)
        a = stateTimer_ / StageConfig::SCENE_FADE_TIME;
    else if(state_ == State::Done)
        a = 1.0f;
    if(a < 0.0f)
        a = 0.0f;
    if(a > 1.0f)
        a = 1.0f;

    if(a > 0.0f) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(255 * a));
        DrawBox(0, 0, WINDOW_W, WINDOW_H, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

}    // namespace PoittersPoint
