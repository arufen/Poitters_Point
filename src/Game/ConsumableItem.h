//---------------------------------------------------------------------------
//!	@file	ConsumableItem.h
//! @brief	ConsumableItem (apple, golden apple, speed potion)
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>

namespace PoittersPoint {
USING_PTR(ConsumableItem);

//Thin object. All the behavior is in ComponentConsumableItem(薄いObject。動きはすべてComponentConsumableItemにある)
class ConsumableItem : public Object
{
public:
    BP_OBJECT_DECL(ConsumableItem, "PoittersPoint::ConsumableItem");

    //! @brief 初期化 / Initialize
    //! @return 初期化終了 / Initialization finished
    bool Init() override;
};
}    // namespace PoittersPoint
