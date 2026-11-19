//---------------------------------------------------------------------------
//!	@file	ItemBox.h
//! @brief	Make any throwable item: cobblestone, ice, obsidian, wood plank, chest, TNT
//!         (投げられるアイテムを作る：丸石・氷・黒曜石・木材の板・チェスト・TNT)
//---------------------------------------------------------------------------
#pragma once
#include "Item.h"
#include "StageConfig.h"

namespace PoittersPoint {

//Spawn a throwable item of that type at the position. Every type IS an Item, so pickup, lifting, throwing,
//damage and NPC AI all work with no changes to other files.
//Chest and TNT also get ComponentItemBox, so they explode after landing from a throw.
//(その種類の投げるアイテムを指定位置に出す。どの種類もItemなので、拾う・持ち上げ・投げる・ダメージ・NPCのAIが
// 他のファイルを変えずにそのまま動く。チェストとTNTはComponentItemBoxもつくので、投げて着地すると爆発する)
ItemPtr SpawnThrowable(StageConfig::ItemType type, const float3& pos);

}    // namespace PoittersPoint
