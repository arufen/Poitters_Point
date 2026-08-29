//---------------------------------------------------------------------------//
//!	@file	Title.cpp
//! @brief	Title
//---------------------------------------------------------------------------//
#include "Title.h"
#include "MainStage.h"

#include <System/Scene.h>
#include <WinMain.h>

namespace PoittersPoint {

//! @brief 初期化
//! @return 初期化済み
bool Title::Init()
{
    return true;
}

void Title::Update()
{
    if(Input::IsKeyDown(KEY_INPUT_RETURN)) {
        Scene::Change(Scene::GetScene<PoittersPoint_MainStage>());
    }
}

void Title::LateDraw()
{
    int w = WINDOW_W;
    int h = WINDOW_H;

    SetFontSize(80);
    int tw = GetDrawFormatStringWidth(u8"職業バトラー");
    DrawFormatString((w - tw) / 2, h / 2 - 40, GetColor(255, 255, 255), u8"職業バトラー");

    SetFontSize(24);
    const char* msg = "Press Enter";
    int         mw  = GetDrawFormatStringWidth(msg);
    DrawFormatString((w - mw) / 2, h / 2 + 60, GetColor(200, 200, 200), msg);
}

}    // namespace PoittersPoint
