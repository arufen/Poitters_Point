//---------------------------------------------------------------------------//
//!	@file	Title.h
//! @brief	Title
//---------------------------------------------------------------------------//
#pragma once
#include <System/Scene.h>

namespace PoittersPoint {

class Title : public Scene::Base
{
public:
    BP_CLASS_DECL(Title, u8"Title");

    //! @brief 初期化
    //! @return 初期化済み
    bool Init() override;

    void Update() override;

    void LateDraw() override;

private:
    enum class State
    {
        Intro,        //!< カメラが引くオープニング演出
        WaitInput,    //!< 入力待ち状態
        Blinking,     //!< Enter押下後のボタン高速点滅状態
        FadeOut       //!< 画面暗転フェードアウト状態
    };

    State state_ = State::Intro;

    int   handleSky_       = -1;      //!< 空
    int   handleBuilding_  = -1;      //!< ビル
    int   handlePlayer_    = -1;      //!< プレイヤー（後ろ姿）
    int   handleLogo_      = -1;      //!< ロゴ（職業バトラー）
    int   handleLogoShine_ = -1;      //!< ロゴの白いシルエット（光の演出用）
    float timer_           = 0.0f;    //!< 点滅制御用のタイマー
    float stateTimer_      = 0.0f;    //!< 状態遷移用のタイマー
    float fadeAlpha_       = 0.0f;    //!< フェードアウト用アルファ値（0:透明 ～ 255:真っ黒）

    float                  introTimer_  = 0.0f;     //!< オープニング演出のタイマー（フレーム）
    static constexpr float kIntroFrames = 90.0f;    //!< 演出の長さ（60fpsで1.5秒）

    // --- フラッシュ演出（ゲーム風） ---
    float                  flashTimer_ = -1.0f;    //!< フラッシュの経過フレーム（-1:停止中）
    static constexpr float kFlashRise  = 8.0f;     //!< 光が立ち上がる時間
    static constexpr float kFlashHold  = 2.0f;     //!< ピークで止まる時間
    static constexpr float kFlashFall  = 80.0f;    //!< ゆっくり消える時間
    static constexpr float kFlashTotal = kFlashRise + kFlashHold + kFlashFall;

    // --- ロゴの光（演出終了後に1回だけ） ---
    float                  shineTimer_    = 0.0f;     //!< 光の演出タイマー（フレーム）
    static constexpr float kShineFrames   = 36.0f;    //!< 光が左から右へ流れる時間
    static constexpr float kSparkleStart  = 30.0f;    //!< 「キラン」を始めるフレーム
    static constexpr float kSparkleFrames = 30.0f;    //!< 「キラン」の長さ
    static constexpr float kShineTotal    = kSparkleStart + kSparkleFrames;

    // --- 「キラン」の繰り返し ---
    float                  sparkleTimer_    = -1.0f;     //!< 今光っているキランの経過フレーム（-1:光っていない）
    float                  sparkleWait_     = 0.0f;      //!< 次のキランまでの待ちフレーム
    static constexpr float kSparkleInterval = 150.0f;    //!< キランとキランの間隔（60fpsで約2.5秒）

    // --- 左右の白フェード（画面端の白いぼかし） ---
    static constexpr int   kEdgeFadeMaxAlpha = 200;     //!< 画面端のアルファ値（中央側は0）
    static constexpr float kEdgeFadeRatio    = 1.0f;    //!< フェードの幅（1.0=中央まで、0.5=半分まで）
    static constexpr int   kEdgeFadeStep     = 4;       //!< 帯の幅(px)。小さいほど滑らか

    //! @brief 左右の白フェードを描画する
    void DrawEdgeFade(int w, int h);

    //! @brief ビルの間から差し込む光の筋を描画する
    //! @param amount 0〜1。演出の進み具合に合わせて薄く出す
    void DrawLightRays(int w, int h, float amount);

    //! @brief 下から舞い上がる金色の粒を描画する
    //! @param amount 0〜1。演出の進み具合に合わせて薄く出す
    void DrawParticles(int w, int h, float amount);
};

}    // namespace PoittersPoint
