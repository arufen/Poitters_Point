#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>
#include <System/EaseCurve.h>

USING_PTR(ComponentSpawnPop);

//Spawn animation: the owner grows from tiny to its real size along an ease curve, then this component removes itself
//The curve comes from the engine (System/EaseCurve.h). OutBack = a little bounce, OutElastic = wobble, OutBounce = bounce
//(出現アニメ：オーナーがとても小さい状態からイージングカーブにそって本来の大きさになる。終わったらこのコンポーネントは自分を消す
// カーブはエンジンのもの(System/EaseCurve.h)。OutBack＝少し弾む、OutElastic＝ぷるぷる、OutBounce＝バウンド)
class ComponentSpawnPop : public Component
{
public:
    BP_COMPONENT_DECL(ComponentSpawnPop, u8"出現アニメ");

    void Init() override;

    void Update() override;

    void GUI() override;

    //Call right after AddComponent. Makes the owner tiny now and remembers the real size. Chainable
    //(AddComponentの直後に呼ぶ。今すぐオーナーを小さくして、本来の大きさを覚える。つなげて呼べる)
    ComponentSpawnPopPtr Start(float targetScale, float duration, EaseType ease = EaseType::OutBack);

private:
    float    targetScale_ = 1.0f;                 //Real size(本来の大きさ)
    float    duration_    = 0.25f;                //Seconds to grow(大きくなる秒数)
    EaseType ease_        = EaseType::OutBack;    //Shape of the growing(大きくなり方のカーブ)
    float    timer_       = 0.0f;
    bool     finished_    = false;

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(duration_));
        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentSpawnPop, 4);
