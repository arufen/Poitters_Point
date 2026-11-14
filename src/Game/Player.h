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

    //Draw the E key icon above the item that can be picked up(拾えるアイテムの上にEキーのアイコンを描く)
    void DrawPickUpIcon();

    //Draw the HP bar at the top-left of the screen(画面の左上にHPバーを描く)
    void DrawHpBar();

private:
    //How high above the feet the marker floats(足元からどれだけ上に目印を浮かべるか)
    float markerHeight_ = 22.0f;

    //UI image handle. -1 = not loaded(UI画像のハンドル。-1は未読み込み)
    int UI_key_e = -1;

    //アニメキーフレーム基準のfps。play_time_はframe単位(dt*30で進む)なので実秒=frame/この値
    static constexpr float kAnimFps_ = 30.0f;

    //投げアニメの再生経過秒。0の時は投げていない
    float throwTimer_ = 0.0f;

    //投げアニメ中にアイテムをリリース済みかどうか
    bool throwReleased_ = false;

    //腕伸展フレームを投げる位置とする割合(設計値 28f/82f)。実際のアニメ長に比例適用する
    float throwReleaseRatio_ = 28.0f / 82.0f;

    //throw開始時に実アニメ長から算出する秒。throwが読めない場合のフォールバック初期値
    float throwReleaseTime_ = 0.9f;     //アイテムを離すまでの経過秒
    float throwAnimTime_    = 2.75f;    //投げアニメ全体の再生秒

    //持ち上げアニメ(carryStart)の速さを物の大きさに合わせるための設定
    //速い(=小さい物)ほど大きな値、遅い(=大きい物)ほど小さい値
    float minLiftAnimSpeed_ = 0.30f;    //一番大きい物の再生速度(遅い)
    float maxLiftAnimSpeed_ = 4.00f;    //一番小さい物の再生速度(速い)

    //今回の持ち上げで既にcarryStartの速さを設定したかどうか
    bool liftAnimStarted_ = false;
};
}    // namespace PoittersPoint
