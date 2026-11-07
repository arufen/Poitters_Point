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

    //Stats (HP, strength, speed) live in ComponentCharacterStats now(ステータスはComponentCharacterStatsに入っている)
    //This just passes the damage to it(ダメージをそこへ渡すだけ)
    void TakeDamage(int dmg);

private:
    //Draw the "PLAYER" marker above the head so we can tell the player from the NPCs(頭の上に「PLAYER」の目印を描いて、プレイヤーとNPCを見分けられるようにする)
    void DrawPlayerMarker();

    //<<<<<<< HEAD
    //
    //private:
    //    //持ち上げ→保持→持ち歩き→投げのアニメーション進行状態
    //    enum class CarryAnimState
    //    {
    //        None,      //!< 持っていない
    //        Start,     //!< Carry_start 再生中
    //        Start2,    //!< Carry_start2 で保持
    //        Walk,      //!< Carry_walk で持ち歩き
    //        Throw,     //!< Carry_throw 再生中
    //    };
    //
    //    CarryAnimState carryState_ = CarryAnimState::None;
    //
    //    //投げアニメ(Carry_throw)の腕伸展フレームでアイテムを離すための設定
    //    bool  throwReleased_    = false;    //!< アイテムをリリース済みかどうか
    //    float throwReleaseTime_ = 0.9f;     //!< 離すタイミング(秒)。Snatch の腕伸展フレーム 28f/30fps 付近
    //
    //    float speed_ = 2.0f;
    //=======
    //Draw the E key icon above the item that can be picked up(拾えるアイテムの上にEキーのアイコンを描く)
    void DrawPickUpIcon();

    //Draw the HP bar at the top-left of the screen(画面の左上にHPバーを描く)
    void DrawHpBar();

private:
    //How high above the feet the marker floats(足元からどれだけ上に目印を浮かべるか)
    float markerHeight_ = 22.0f;

    //UI image handle. -1 = not loaded(UI画像のハンドル。-1は未読み込み)
    int UI_key_e = -1;
};
}    // namespace PoittersPoint
