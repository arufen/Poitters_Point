#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentConsumableItem);

class ComponentCharacterStats;    //Forward declaration, the full class is only needed in the cpp(前方宣言。cppでだけ中身が必要)

//Consumable item: apple, golden apple, speed potion. Anyone with ComponentCharacterStats who walks close gets the effect, then the item disappears.
//(消費アイテム：リンゴ・金のリンゴ・スピードポーション。ComponentCharacterStatsを持つキャラが近づくと効果を受け取り、アイテムは消える)
class ComponentConsumableItem : public Component
{
public:
    BP_COMPONENT_DECL(ComponentConsumableItem, u8"消費アイテム");

    //Item types(アイテムの種類)
    enum class ItemType
    {
        Apple,          //Heal a little(少し回復)
        GoldenApple,    //Heal more(たくさん回復)
        SpeedPotion,    //Faster for a short time(少しの間、速くなる)
    };

    void Init() override;

    void Update() override;

    void GUI() override;

    //Set the type. Call this right after AddComponent (before the first Update)(種類を設定する。AddComponentの直後に呼ぶ)
    ComponentConsumableItemPtr SetItemType(ItemType type);

    ItemType GetItemType() const { return (ItemType)type_; }

    //Model file for each type, so the Object can load the right model(種類ごとのモデルファイル。Object側が正しいモデルを読み込むために使う)
    static const char* GetModelPath(ItemType type);

    //Model size for each type. Change the number in the cpp to make one model bigger or smaller(種類ごとのモデルの大きさ。cppの数字を変えると、そのモデルだけ大きさが変わる)
    static float GetModelScale(ItemType type);

    //Short name for each type, used for the object name(種類ごとの短い名前。オブジェクト名に使う)
    static const char* GetItemName(ItemType type);

private:
    //Give the effect to the character. Returns false if nothing happened (the item stays)(キャラに効果を与える。何も起きなかったらfalse(アイテムは残る))
    bool ApplyEffect(ComponentCharacterStats& stats);

private:
    int  type_      = 0;        //ItemType as int, so cereal can save it(ItemTypeをintで持つ。cerealで保存しやすくするため)
    bool collected_ = false;    //True once someone took it, so it can not be taken twice(誰かが取ったらtrue。二重に取れないようにする)

    int   healApple_       = 2;       //HP for Apple(リンゴのHP回復量)
    int   healGolden_      = 5;       //HP for Golden Apple(金のリンゴのHP回復量)
    float speedMultiplier_ = 1.2f;    //Speed potion: 1.2 = 20% faster(スピードポーション：1.2＝20%速い)
    float speedDuration_   = 5.0f;    //Speed potion: seconds(スピードポーション：効果の秒数)

    float pickupRange_  = 12.0f;    //Flat distance to collect, in world units(取れる距離(平面)。ワールド単位)
    float pickupHeight_ = 30.0f;    //Max height difference to collect(取れる高さの差の最大)

    bool skipHealWhenFull_ = false;    //true = healing items are not taken when HP is full(trueならHP満タンの時は回復アイテムを取らない)

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(type_),
            CEREAL_NVP(healApple_),
            CEREAL_NVP(healGolden_),
            CEREAL_NVP(speedMultiplier_),
            CEREAL_NVP(speedDuration_),
            CEREAL_NVP(pickupRange_),
            CEREAL_NVP(pickupHeight_),
            CEREAL_NVP(skipHealWhenFull_));

        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentConsumableItem, 4);
