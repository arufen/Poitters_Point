#include "SelectScene.h"
#include "MainStage.h"
#include "Title.h"

#include <System/Scene.h>
#include <WinMain.h>

namespace PoittersPoint {

bool PoittersPoint_SelectScene::Init()
{
    m_selected = 0;
    return true;
}

void PoittersPoint_SelectScene::Update()
{
    if(Input::IsKeyDown(KEY_INPUT_LEFT) || Input::IsKeyDown(KEY_INPUT_A)) {
        m_selected = (m_selected + 3) % 4;
    }
    if(Input::IsKeyDown(KEY_INPUT_RIGHT) || Input::IsKeyDown(KEY_INPUT_D)) {
        m_selected = (m_selected + 1) % 4;
    }

    if(Input::IsKeyDown(KEY_INPUT_RETURN) || Input::IsKeyDown(KEY_INPUT_SPACE)) {
        Scene::Change(Scene::GetScene<PoittersPoint_MainStage>());
    }

    if(Input::IsKeyDown(KEY_INPUT_ESCAPE)) {
        Scene::Change(Scene::GetScene<Title>());
    }
}

void PoittersPoint_SelectScene::LateDraw()
{
    int w = WINDOW_W;
    int h = WINDOW_H;

    SetFontSize(64);
    int tw = GetDrawFormatStringWidth("Character Select");
    DrawFormatString((w - tw) / 2, 60, GetColor(255, 255, 255), "Character Select");

    const char* names[4] = {"Char1", "Char2", "Char3", "Char4"};
    int         boxW     = 200;
    int         boxH     = 280;
    int         spacing  = 40;
    int         startX   = (w - (boxW * 4 + spacing * 3)) / 2;
    int         startY   = h / 2 - boxH / 2;

    for(int i = 0; i < 4; ++i) {
        int x = startX + i * (boxW + spacing);
        int y = startY;
        if(i == m_selected) {
            DrawBox(x - 4, y - 4, x + boxW + 4, y + boxH + 4, GetColor(255, 255, 0), FALSE);
        }
        DrawBox(x, y, x + boxW, y + boxH, GetColor(80, 80, 80), FALSE);
        DrawBox(x + 2, y + 2, x + boxW - 2, y + boxH - 2, GetColor(40, 40, 40), TRUE);
        SetFontSize(28);
        int ctw = GetDrawFormatStringWidth(names[i]);
        DrawFormatString(x + (boxW - ctw) / 2, y + boxH / 2 - 14, GetColor(255, 255, 255), names[i]);
    }

    SetFontSize(24);
    const char* msg = "<- -> / A D : Select   Enter / Space : Start   Esc : Back";
    int         mw  = GetDrawFormatStringWidth(msg);
    DrawFormatString((w - mw) / 2, h - 80, GetColor(200, 200, 200), msg);
}

}    // namespace PoittersPoint
