//---------------------------------------------------------------------------
//!	@file	TutorialX_Camera.cpp
//! @brief	TutorialX_Camera
//---------------------------------------------------------------------------

#include "MainCamera.h"
#include "MainStage.h"

namespace PoittersPoint {
//! @brief 初期化
//! @return 初期化終了
bool Camera::Init()
{
    Super::Init();

    SetName("Camera");

    //auto camera = Scene::Object::Create<Object>("Camera");
    AddComponent<ComponentCamera>()    //
        ->SetPositionAndTarget({0, 190, -310}, {0, 0, 0});

    return true;
}
}    // namespace PoittersPoint
