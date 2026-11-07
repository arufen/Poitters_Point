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
    //持ち上げ→保持→持ち歩き→投げのアニメーション進行状態
    enum class CarryAnimState
    {
        None,      //!< 持っていない
        Start,     //!< Carry_start 再生中
        Start2,    //!< Carry_start2 で保持
        Walk,      //!< Carry_walk で持ち歩き
        Throw,     //!< Carry_throw 再生中
    };

    CarryAnimState carryState_ = CarryAnimState::None;

    //投げアニメ(Carry_throw)の腕伸展フレームでアイテムを離すための設定
    bool  throwReleased_    = false;    //!< アイテムをリリース済みかどうか
    float throwReleaseTime_ = 0.9f;     //!< 離すタイミング(秒)。Snatch の腕伸展フレーム 28f/30fps 付近

    float speed_ = 2.0f;

    //How high above the feet the marker floats(足元からどれだけ上に目印を浮かべるか)
    float markerHeight_ = 22.0f;

    //UI
    int UI_key_e;
};
}    // namespace PoittersPoint
