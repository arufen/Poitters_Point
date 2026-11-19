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
#include "ConsumableItem.h"

#include "MainCamera.h"
#include "Component/ComponentMatch.h"
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

    //Throwable items (rocks, chest, TNT...) all come from the spawner. Edit the pool in StageConfig.h
    //Called after Player/Npc, so items do not spawn on top of them
    //(投げるアイテム(石・チェスト・TNT...)はすべてスポナーから出る。プールはStageConfig.hで変える
    // Player/Npcの後に呼ぶので、キャラの真上にはアイテムが出ない)
    itemSpawner_.Init();

    // Spawn consumable items (apple, golden apple, speed potion) at random places in the arena
    // 消費アイテム(リンゴ・金のリンゴ・スピードポーション)をアリーナのランダムな場所に配置
    for(int i = 0; i < 6; ++i) {
        auto consumable = Scene::Object::Create<ConsumableItem>();
        if(consumable) {
            float rx = (GetRand(200) - 100);
            float rz = (GetRand(200) - 100);
            consumable->SetTranslate({rx, 60.0f, rz});
        }
    }

    //The match: countdown, timer, game over -> Result scene. Created last, after all the characters
    //(試合：カウントダウン、タイマー、ゲームオーバー→Resultシーン。キャラを全員作った後、最後に作る)
    if(auto match = Scene::Object::Create<::Object>("Match"))
        match->AddComponent<ComponentMatch>();

    return true;
}

void PoittersPoint_MainStage::Update()
{
    itemSpawner_.Update(GetDeltaTime());
}

void PoittersPoint_MainStage::GUI()
{
    Super::GUI();
    itemSpawner_.GUI();
}

}    // namespace PoittersPoint
