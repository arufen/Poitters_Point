//---------------------------------------------------------------------------
//!	@file	ItemBox.h
//! @brief	ItemBox (a throwable chest that explodes on landing and scatters items)
//---------------------------------------------------------------------------
#pragma once
#include "Item.h"

namespace PoittersPoint {

//Spawn an item box at the position. The box IS an Item with a chest model and ComponentItemBox,
//so pickup, lifting, throwing, damage and NPC AI all work with no changes to other files.
//(指定位置にアイテムボックスを出す。ボックスの正体は「宝箱モデル＋ComponentItemBox」をつけたItemなので、
// 拾う・持ち上げ・投げる・ダメージ・NPCのAIが、他のファイルを変えずにそのまま動く)
ItemPtr SpawnItemBox(const float3& pos);

}    // namespace PoittersPoint
