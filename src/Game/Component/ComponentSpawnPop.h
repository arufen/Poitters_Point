#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentSpawnPop);

//Spawn animation: the owner grows from tiny to its real size with a little bounce, then this component removes itself
//(出現アニメ：オーナーがとても小さい状態から本来の大きさまで少し弾むように大きくなる。終わったらこのコンポーネントは自分を消す)
class ComponentSpawnPop : public Component
{
public:
    BP_COMPONENT_DECL(ComponentSpawnPop, u8"出現アニメ");

    void Init() override;

    void Update() override;

    void GUI() override;

    //Call right after AddComponent. Makes the owner tiny now and remembers the real size. Chainable
    //(AddComponentの直後に呼ぶ。今すぐオーナーを小さくして、本来の大きさを覚える。つなげて呼べる)
    ComponentSpawnPopPtr Start(float targetScale, float duration);

private:
    float targetScale_ = 1.0f;     //Real size(本来の大きさ)
    float duration_    = 0.25f;    //Seconds to grow(大きくなる秒数)
    float timer_       = 0.0f;
    bool  finished_    = false;

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
