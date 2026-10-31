//---------------------------------------------------------------------------//
//!	@file	Main_menu.cpp
//! @brief	メインメニュー（仮）
//---------------------------------------------------------------------------//
#include "Main_menu.h"
#include "MainStage.h"
#include <System/Scene.h>
#include <WinMain.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace PoittersPoint {

//! @brief 初期化
//! @return 初期化済み
bool MainMenu::Init()
{
    // 背景はタイトルと同じ画像を使う
    handleSky_      = LoadGraph(u8"data/Game/Title/Sky.png");
    handleBuilding_ = LoadGraph(u8"data/Game/Title/Building.png");

    timer_     = 0.0f;
    fadeAlpha_ = 255.0f;    // 真っ暗な状態から始める
    state_     = State::FadeIn;

    return true;
}

void MainMenu::Update()
{
    timer_ += 0.05f;

    switch(state_) {
    case State::FadeIn:
        // 少しずつ明るくする。この間は入力を受け付けない
        fadeAlpha_ -= kFadeInSpeed;
        if(fadeAlpha_ <= 0.0f) {
            fadeAlpha_ = 0.0f;
            state_     = State::Idle;
        }
        break;

    case State::Idle:
        // 1キーでゲームシーンへ
        if(Input::IsKeyDown(KEY_INPUT_1)) {
            Scene::Change(Scene::GetScene<PoittersPoint_MainStage>());
        }
        break;
    }
}

void MainMenu::LateDraw()
{
    const int w = WINDOW_W;
    const int h = WINDOW_H;

    // --- 1. 背景（空とビルを全画面に描き、暗くして文字を読みやすくする） ---
    if(handleSky_ != -1)
        DrawExtendGraph(0, 0, w, h, handleSky_, TRUE);
    if(handleBuilding_ != -1)
        DrawExtendGraph(0, 0, w, h, handleBuilding_, TRUE);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
    DrawBox(0, 0, w, h, GetColor(10, 15, 40), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // --- 2. 見出し「MAIN MENU」 ---
    {
        SetFontSize(48);
        const char* text = "MAIN MENU";
        const int   tw   = GetDrawStringWidth(text, static_cast<int>(std::strlen(text)));
        const int   tx   = (w - tw) / 2;
        const int   ty   = h / 5;

        DrawString(tx + 2, ty + 3, text, GetColor(0, 0, 0));    // 影
        DrawString(tx, ty, text, GetColor(255, 215, 0));        // 本体（金色）

        // 見出しの下の線
        DrawLine(tx - 20, ty + 64, tx + tw + 20, ty + 64, GetColor(255, 215, 0));
    }

    // --- 3. 「1 : GAME START」ボタン ---
    {
        SetFontSize(32);
        const char* text = "[ 1 ]  GAME START";
        const int   tw   = GetDrawStringWidth(text, static_cast<int>(std::strlen(text)));

        const int btnW = tw + 80;
        const int btnH = 70;
        const int btnX = (w - btnW) / 2;
        const int btnY = h / 2 - btnH / 2;

        // 操作できるようになったら、ゆっくり点滅させる
        int alpha = 255;
        if(state_ == State::Idle)
            alpha = static_cast<int>((std::sin(timer_ * 1.5f) + 1.0f) * 0.5f * 100.0f) + 155;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawRoundRect(btnX, btnY, btnX + btnW, btnY + btnH, 10, 10, GetColor(12, 18, 45), TRUE);
        DrawRoundRect(btnX, btnY, btnX + btnW, btnY + btnH, 10, 10, GetColor(255, 215, 0), FALSE);
        DrawRoundRect(btnX + 1, btnY + 1, btnX + btnW - 1, btnY + btnH - 1, 10, 10, GetColor(255, 215, 0), FALSE);

        const int tx = btnX + (btnW - tw) / 2;
        const int ty = btnY + (btnH - 32) / 2;
        DrawString(tx + 1, ty + 2, text, GetColor(0, 0, 0));
        DrawString(tx, ty, text, GetColor(255, 255, 255));
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // --- 4. 下のヒント文字 ---
    {
        SetFontSize(20);
        const char* text = "Press 1 to start the game";
        const int   tw   = GetDrawStringWidth(text, static_cast<int>(std::strlen(text)));
        DrawString((w - tw) / 2, h - 90, text, GetColor(220, 220, 220));
    }

    // --- 5. フェードイン用の黒マスク（暗転 → 明転） ---
    if(fadeAlpha_ > 0.0f) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(fadeAlpha_));
        DrawBox(0, 0, w, h, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

}    // namespace PoittersPoint
