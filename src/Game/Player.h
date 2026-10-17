//---------------------------------------------------------------------------
//!	@file	TutorialX_Player.h
//! @brief	TutorialX_Player
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>

namespace PoittersPoint {
USING_PTR(Player);
class Player : public Object
{
public:
    BP_OBJECT_DECL(Player, "PoittersPoint::Player");

    //! @brief 初期化
    //! @return 初期化終了
    bool Init() override;

    void Update() override;

    void PostDraw() override;

private:
    //Draw the "PLAYER" marker above the head so we can tell the player from the NPCs(頭の上に「PLAYER」の目印を描いて、プレイヤーとNPCを見分けられるようにする)
    void DrawPlayerMarker();

private:
    float speed_ = 2.0f;

    //How high above the feet the marker floats(足元からどれだけ上に目印を浮かべるか)
    float markerHeight_ = 22.0f;

    //UI
    int UI_key_e;
};
}    // namespace PoittersPoint
