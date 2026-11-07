//---------------------------------------------------------------------------
//!	@file	Player.h
//! @brief	Player
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>

namespace PoittersPoint {
USING_PTR(Player);
class Player : public Object
{
public:
    BP_OBJECT_DECL(Player, "PoittersPoint::Player");

    //! @brief 初期化 / Initialize
    //! @return 初期化終了 / Initialization finished
    bool Init() override;

    void Update() override;

    void PostDraw() override;

private:
    //Draw the "PLAYER" marker above the head so we can tell the player from the NPCs(頭の上に「PLAYER」の目印を描いて、プレイヤーとNPCを見分けられるようにする)
    void DrawPlayerMarker();

    //Draw the E key icon above the item that can be picked up(拾えるアイテムの上にEキーのアイコンを描く)
    void DrawPickUpIcon();

    //Draw the HP bars of the player and all NPCs in a row at the bottom of the screen(画面の下にプレイヤーと全NPCのHPバーを横一列に描く)
    void DrawHpHud();

private:
    //How high above the feet the marker floats(足元からどれだけ上に目印を浮かべるか)
    float markerHeight_ = 22.0f;

    //UI image handle. -1 = not loaded(UI画像のハンドル。-1は未読み込み)
    int UI_key_e = -1;
};
}    // namespace PoittersPoint
