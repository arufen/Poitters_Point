//---------------------------------------------------------------------------
//!	@file	MainStage.cpp
//! @brief	MainStage
//---------------------------------------------------------------------------
#include "MainStage.h"
//#include "TutorialX_GameOver.h"

#include "Player.h"
#include "Npc.h"
#include "Ground.h"
#include "Item.h"

#include "MainCamera.h"
//#include "TutorialX_Enemy.h"

namespace PoittersPoint {

//! @brief 初期化 / Initialize
//! @return 初期化済み / Initialized
bool PoittersPoint_MainStage::Init()
{
    // 最初に1回動作する
    // ただし trueを返さなければ Initに何回も来る仕様。

    Scene::Object::Create<Ground>();

    // Spawn player (already requested) / プレイヤーを1体生成（既に存在として扱う）
    Scene::Object::Create<Player>();
    // Spawn 3 NPCs and make them able to throw objects at NPC or player / NPCを3体生成し、NPC同士・プレイヤーに投げられるようにする
    for(int i = 0; i < 3; ++i) {
        auto npc = Scene::Object::Create<Npc>();
        // Give some randomness / ランダム性を追加
        float3 pos = {20.0f + (GetRand(80) - 40), 5.0f, 20.0f + (GetRand(80) - 40)};
        if(npc)
            npc->SetTranslate(pos);
    }

    Scene::Object::Create<Camera>();

    // Spawn more items in random places on the ground / 地面上のランダムな場所にアイテムを複数配置
    for(int i = 0; i < 8; ++i) {
        auto item = Scene::Object::Create<Item>();
        if(item) {
            float rx = (GetRand(200) - 100);
            float rz = (GetRand(200) - 100);
            item->SetTranslate({rx, 60.0f, rz});
        }
    }
    // Also spawn one more as before for compatibility / 互換性のため1つ追加
    Scene::Object::Create<Item>();

    //createEnemy();

    return true;
}

void PoittersPoint_MainStage::Update()
{
}

}    // namespace PoittersPoint
