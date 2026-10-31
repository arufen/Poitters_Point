#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentCharacterStats);

//Stats for any character (Player or NPC): HP, strength, speed. Also tells how long lifting an item takes.
//(キャラクター共通のステータス：HP・筋力・速度。アイテムを持ち上げる時間も計算する。PlayerもNPCも同じものを使う)
class ComponentCharacterStats : public Component
{
public:
    BP_COMPONENT_DECL(ComponentCharacterStats, u8"キャラクターステータス");

    void Init() override;

    void GUI() override;

    //Set max HP. If fill is true, HP becomes full too(最大HPを設定。fillがtrueなら現在HPも満タンにする)
    ComponentCharacterStatsPtr SetMaxHP(int maxHp, bool fill = true);

    //Strength: stronger = lifts heavy items faster(筋力：強いほど重い物を早く持ち上げられる)
    ComponentCharacterStatsPtr SetStrength(float strength);

    //Move speed per frame(1フレームの移動量)
    ComponentCharacterStatsPtr SetSpeed(float speed);

    //Give damage. Returns true if HP really went down(ダメージを与える。HPが実際に減ったらtrue)
    bool TakeDamage(int damage);

    //Heal HP(HPを回復する)
    void Heal(int amount);

    //How many seconds this character needs to lift an item of this weight(このキャラがその重さのアイテムを持ち上げるのにかかる秒数)
    float GetLiftTime(float itemWeight) const;

    int GetHP() const { return hp_; }
    int GetMaxHP() const { return maxHp_; }

    //HP from 0.0 to 1.0, good for HP bar(HPの割合。HPバー用)
    float GetHPRatio() const { return maxHp_ > 0 ? (float)hp_ / (float)maxHp_ : 0.0f; }
    bool  IsDead() const { return hp_ <= 0; }
    float GetStrength() const { return strength_; }
    float GetSpeed() const { return speed_; }

private:
    int   hp_       = 15;      //Current HP(現在のHP)
    int   maxHp_    = 15;      //Max HP(最大HP)
    float strength_ = 1.0f;    //Strength(筋力)
    float speed_    = 1.0f;    //Move speed(移動速度)

    float liftTimeScale_ = 0.3f;    //Bigger = lifting takes longer(大きいほど持ち上げに時間がかかる)
    float maxLiftTime_   = 3.5f;    //Longest lift time in seconds(持ち上げ時間の最大秒数)

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(hp_), CEREAL_NVP(maxHp_), CEREAL_NVP(strength_), CEREAL_NVP(speed_), CEREAL_NVP(liftTimeScale_), CEREAL_NVP(maxLiftTime_));

        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentCharacterStats, 4);
