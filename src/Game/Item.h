//---------------------------------------------------------------------------
//!	@file	PoittersPoint_Item.h
//! @brief	PoittersPoint_Item
//---------------------------------------------------------------------------
#include <System/Scene.h>
#include <string>

namespace PoittersPoint {
USING_PTR(Item);
class Item : public Object
{
public:
    BP_OBJECT_DECL(Item, "PoittersPoint::Item");

    //! @brief 初期化 / Initialize
    //! @return 初期化終了 / Initialization finished
    bool Init() override;

    // 当たり判定が行われたときに呼ばれる関数 / Called when collision occurs
    void OnHit(const ComponentCollision::HitInfo& hit_info) override;

    // Item weight type / アイテムの重さ種別
    enum class WeightType
    {
        Light,     // 軽い / Light
        Medium,    // 普通 / Medium
        Heavy      // 重い / Heavy
    };

    WeightType GetWeightType() const { return weightType_; }
    float      GetWeight() const { return weight_; }
    //Object size (model scale). Used to scale the lifting animation(オブジェクトの大きさ(モデルのスケール)。持ち上げアニメーションの速さに使う)
    float              GetSize() const { return size_; }
    const std::string& GetItemName() const { return itemName_; }
    void               RandomizeStats();

private:
    WeightType  weightType_ = WeightType::Medium;
    float       weight_     = 1.0f;
    float       size_       = 0.5f;    //Model scale set in Init(Initで設定したモデルのスケール)
    std::string itemName_;
};
}    // namespace PoittersPoint
