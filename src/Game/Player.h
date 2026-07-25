//---------------------------------------------------------------------------
//!	@file	TutorialX_Player.h
//! @brief	TutorialX_Player
//---------------------------------------------------------------------------
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

private:
    float speed_ = 2.0f;
};
}    // namespace PoittersPoint
