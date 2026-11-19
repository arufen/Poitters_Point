//---------------------------------------------------------------------------
//!	@file	ItemSpawner.h
//! @brief	Keeps the item pool full: fills the field at the start, and sends a carrier for every item that is missing
//!         (アイテムプールを満タンに保つ：開始時にフィールドを埋めて、なくなったアイテムごとに運び役を送る)
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>
#include "Item.h"
#include "StageConfig.h"
#include "Component/ComponentSpawnCarrier.h"
#include <vector>

namespace PoittersPoint {

//Plain class, not a Component: spawning is stage logic, so MainStage owns it as a member.
//"The thing that creates it, tracks it": items do not need to know they are counted.
//(コンポーネントではなく普通のクラス：スポーンはステージの処理なので、MainStageがメンバーとして持つ。
// 「作った物が数える」：アイテムは自分が数えられていることを知らなくていい)
class ItemSpawner
{
public:
    //Fill the field right away: every pool item falls from the sky at a random spot(すぐにフィールドを埋める：プールの全アイテムがランダムな場所に空から落ちる)
    void Init();

    //Check every slot. Missing item -> wait respawnDelay -> send a carrier(全スロットを調べる。アイテムがない→respawnDelay待つ→運び役を送る)
    void Update(float deltaTime);

    //Debug window with every slot(全スロットを見られるデバッグ画面)
    void GUI();

    //Debug: delete every pool item so they all come back by carrier(デバッグ用：プールのアイテムを全部消して、全部運び役で戻ってこさせる)
    void RespawnAll();

private:
    //One place in the pool. 3 obsidian = 3 slots(プールの1つ分の場所。黒曜石3個＝3スロット)
    struct Slot
    {
        StageConfig::ItemType        type{};
        float                        respawnDelay = 0.0f;
        ItemWeakPtr                  item;               //Item on the field(フィールドにあるアイテム)
        ComponentSpawnCarrierWeakPtr carrier;            //Carrier on the way(向かっている運び役)
        bool                         waiting = false;    //True while counting down to send a carrier(運び役を送るまでカウントダウン中ならtrue)
        float                        timer   = 0.0f;     //Seconds left until the carrier is sent(運び役を送るまでの残り秒数)
    };

    //Random free spot inside the arena: away from walls, characters, items and other carriers' targets
    //(アリーナの中のランダムな空いた場所：壁・キャラ・アイテム・ほかの運び役の目標から離れた所)
    float3 FindSpawnPoint() const;

    //True if the item still exists and was not released (exploded chest/TNT = false)(アイテムがまだあってReleaseされていなければtrue(爆発したチェスト・TNTはfalse))
    static ItemPtr GetAliveItem(const ItemWeakPtr& weak);

    std::vector<Slot> slots_;
};

}    // namespace PoittersPoint
