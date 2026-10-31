//---------------------------------------------------------------------------//
//!	@file	Main_menu.h
//! @brief	メインメニュー（仮）
//---------------------------------------------------------------------------//
#pragma once
#include <System/Scene.h>

namespace PoittersPoint {

class MainMenu : public Scene::Base
{
public:
    BP_CLASS_DECL(MainMenu, u8"MainMenu");

    //! @brief 初期化
    //! @return 初期化済み
    bool Init() override;

    void Update() override;

    void LateDraw() override;

private:
    enum class State
    {
        FadeIn,    //!< 暗転から明転するフェードイン（操作不可）
        Idle       //!< 操作できる状態
    };

    State state_ = State::FadeIn;

    int   handleSky_      = -1;        //!< 背景の空
    int   handleBuilding_ = -1;        //!< 背景のビル
    float timer_          = 0.0f;      //!< 点滅制御用のタイマー
    float fadeAlpha_      = 255.0f;    //!< フェードイン用アルファ値（255:真っ黒 ～ 0:透明）

    static constexpr float kFadeInSpeed = 5.0f;    //!< 1フレームに明るくなる量（タイトルのフェードアウトと同じ速さ）
};

}    // namespace PoittersPoint
