//---------------------------------------------------------------------------
//!	@file	MainStage.h
//! @brief	MainStage
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>
#include "ItemSpawner.h"

namespace PoittersPoint {

class PoittersPoint_MainStage : public Scene::Base
{
public:
    static constexpr int MAX_ENEMIES = 5;

    BP_CLASS_DECL(PoittersPoint_MainStage, u8"MainStage");

    //! @brief 初期化
    //! @return 初期化済み
    bool Init() override;

    void Update() override;

    void GUI() override;

private:
    //Keeps the throwable item pool full (StageConfig.h)(投げるアイテムのプールを満タンに保つ(StageConfig.h))
    ItemSpawner itemSpawner_;
};

}    // namespace PoittersPoint
