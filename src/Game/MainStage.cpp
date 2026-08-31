//---------------------------------------------------------------------------
//!	@file	MainStage.cpp
//! @brief	MainStage
//---------------------------------------------------------------------------
#include "MainStage.h"
#include "TutorialX_GameOver.h"

#include "Player.h"
#include "Ground.h"
#include "Item.h"

#include "MainCamera.h"
#include "TutorialX_Enemy.h"

namespace PoittersPoint {

void PoittersPoint_MainStage::createEnemy()
{
    for(int i = 0; i < MAX_ENEMIES; i++) Scene::Object::Create<TutorialX::Enemy>();
}

//! @brief 初期化
//! @return 初期化済み
bool PoittersPoint_MainStage::Init()
{
    // 最初に1回動作する
    // ただし trueを返さなければ Initに何回も来る仕様。

    Scene::Object::Create<Ground>();

    Scene::Object::Create<Player>();

    Scene::Object::Create<Camera>();

    Scene::Object::Create<Item>();

    createEnemy();

    return true;
}

void PoittersPoint_MainStage::Update()
{
    printfDx("\nDEAD ENEMY: %d", enemy_dead_count_);

    if(enemy_dead_count_ >= MAX_ENEMIES) {
        //createEnemy();
        Scene::Change(Scene::GetScene<TutorialX::TutorialX_GameOver>());

        enemy_dead_count_ = 0;
    }
}

void PoittersPoint_MainStage::AddDeadEnemy()
{
    enemy_dead_count_++;
}

}    // namespace PoittersPoint
