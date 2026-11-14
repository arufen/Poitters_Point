#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentItemBox);

//Put this on an Item to make it an item box. After it is thrown and lands, it explodes and scatters items around
//(Itemにつけるとアイテムボックスになる。投げられて着地すると爆発して、まわりにアイテムをばらまく)
class ComponentItemBox : public Component
{
public:
    BP_COMPONENT_DECL(ComponentItemBox, u8"アイテムボックス");

    void Init() override;

    void Update() override;

    void GUI() override;

    //Content settings, chainable(中身の設定。つなげて呼べる)
    ComponentItemBoxPtr SetContentCount(int minCount, int maxCount);
    ComponentItemBoxPtr SetConsumableChance(int percent);

    //Boom now. Spawns the items, the blast effect, and deletes the box(今すぐ爆発。アイテムと爆発エフェクトを出して、箱を消す)
    void Explode();

private:
    //Create one item that flies out in that direction(その方向に飛び出すアイテムを1つ作る)
    void SpawnOneItem(const float3& center, float angle);

private:
    bool  wasThrown_  = false;    //Was it flying last frame?(前のフレームで飛んでいたか)
    bool  exploded_   = false;    //True after the boom, so it only explodes once(爆発したらtrue。1回だけ爆発する)
    float thrownTime_ = 0.0f;     //How long it has been flying(飛んでいる時間)

    float maxFlyTime_ =
        5.0f;    //Safety: explode anyway if it flies this long (fell off the stage etc.)(安全のため：この秒数飛び続けたら爆発する(ステージ外に落ちた時など))

    int minCount_ = 3;    //Least items inside(中身の最小数)
    int maxCount_ = 5;    //Most items inside(中身の最大数)
    int consumableChance_ =
        60;    //% chance each item is a consumable (apple etc.), the rest are throwables(各アイテムが消費アイテムになる確率%。残りは投げるアイテム)

    float flySpeedMin_ = 40.0f;    //Sideways speed of the scattered items(ばらまくアイテムの横の速さ)
    float flySpeedMax_ = 70.0f;
    float flyUpMin_    = 60.0f;    //Upward speed of the scattered items(ばらまくアイテムの上の速さ)
    float flyUpMax_    = 90.0f;

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(maxFlyTime_),
            CEREAL_NVP(minCount_),
            CEREAL_NVP(maxCount_),
            CEREAL_NVP(consumableChance_),
            CEREAL_NVP(flySpeedMin_),
            CEREAL_NVP(flySpeedMax_),
            CEREAL_NVP(flyUpMin_),
            CEREAL_NVP(flyUpMax_));

        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentItemBox, 4);
