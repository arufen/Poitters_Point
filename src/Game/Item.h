//---------------------------------------------------------------------------
//!	@file	PoittersPoint_Item.h
//! @brief	PoittersPoint_Item
//---------------------------------------------------------------------------
#include <System/Scene.h>

namespace PoittersPoint {
USING_PTR(Item);
class Item : public Object
{
public:
    BP_OBJECT_DECL(Item, "PoittersPoint::Item");

    //! @brief 初期化
    //! @return 初期化終了
    bool Init() override;
};
}    // namespace PoittersPoint
