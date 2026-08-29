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
};

}    // namespace PoittersPoint
