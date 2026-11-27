//---------------------------------------------------------------------------
//!	@file	TutorialX_Camera.cpp
//! @brief	TutorialX_Camera
//---------------------------------------------------------------------------

#include "MainCamera.h"
#include "MainStage.h"
#include "Component/ComponentGroupCamera.h"

namespace PoittersPoint {
//! @brief 初期化
//! @return 初期化終了
bool Camera::Init()
{
    Super::Init();

    SetName("Camera");

    AddComponent<ComponentCamera>()    //
        ->SetPositionAndTarget({0, 190, -310}, {0, 0, 0});

    //Poitter's Point style camera: follows the middle of every character still alive and zooms to fit them
    //(ポイッターズポイント風カメラ：生きているキャラ全員の真ん中を追いかけて、全員が入るようにズームする)
    AddComponent<ComponentGroupCamera>();

    return true;
}
}    // namespace PoittersPoint
