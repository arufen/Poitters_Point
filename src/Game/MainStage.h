//---------------------------------------------------------------------------
//!	@file	MainStage.h
//! @brief	MainStage
//---------------------------------------------------------------------------
#include <System/Scene.h>

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

    void AddDeadEnemy();

private:
    void createEnemy();

    int enemy_dead_count_ = 0;
};

}    // namespace PoittersPoint
