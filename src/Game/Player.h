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

    //! @brief 初期化 / Initialize
    //! @return 初期化終了 / Initialization finished
    bool Init() override;

    void Update() override;

    void PostDraw() override;

    // Stats for Poitters Point style game (ポイッターズポイント風ステータス)
    // Speed / 速度
    float GetSpeed() const { return speed_; }
    void  SetSpeed(float s) { speed_ = s; }
    // HP (integer approx 10-20 as requested / HPは整数で10〜20程度)
    int  GetHP() const { return hp_; }
    void SetHP(int hp) { hp_ = hp; }
    void TakeDamage(int dmg);
    // Strength power / 筋力
    float GetStrength() const { return strength_; }
    void  SetStrength(float str) { strength_ = str; }
    // Whether player can lift heavy item easier based on strength / 筋力に応じて重いアイテムを持てるか
    bool CanLiftHeavy(float itemWeight) const;

private:
    //Draw the "PLAYER" marker above the head so we can tell the player from the NPCs(頭の上に「PLAYER」の目印を描いて、プレイヤーとNPCを見分けられるようにする)
    void DrawPlayerMarker();

private:
    // Movement speed / 移動速度
    float speed_ = 2.0f;

    // Poitters Point style stats / ステータス
    int   hp_       = 15;      // HP (10-20 range as per game style / ポイッターズポイント風に10〜20程度)
    float strength_ = 1.0f;    // Strength power / 筋力（強いほど重い物を持ちやすく、アニメ短縮にも使用予定）

    //How high above the feet the marker floats(足元からどれだけ上に目印を浮かべるか)
    float markerHeight_ = 22.0f;

    //UI
    int UI_key_e;
};
}    // namespace PoittersPoint
