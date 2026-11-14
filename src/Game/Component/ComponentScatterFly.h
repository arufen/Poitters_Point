#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentScatterFly);

//Makes an object fly in an arc once (like popping out of a box), then it stops and normal gravity takes over
//(オブジェクトを1回だけ弧を描いて飛ばす(箱から飛び出す感じ)。止まったら普通の重力にまかせる)
class ComponentScatterFly : public Component
{
public:
    BP_COMPONENT_DECL(ComponentScatterFly, u8"飛び出し");

    void Init() override;

    void Update() override;

    void GUI() override;

    //Set the start speed. Call right after AddComponent(最初の速度を設定する。AddComponentの直後に呼ぶ)
    ComponentScatterFlyPtr SetVelocity(const float3& v);

    bool IsFlying() const { return flying_; }

private:
    float3 velocity_ = {0.0f, 0.0f, 0.0f};
    float  gravity_  = 150.0f;    //Same as the thrown item(投げたアイテムと同じ)
    float  startY_   = 0.0f;      //Height at the start. We stop when we fall back to it(開始の高さ。ここまで落ちたら止まる)
    float  maxTime_  = 2.0f;      //Safety: stop after this many seconds no matter what(安全のため：何があってもこの秒数で止まる)
    float  timer_    = 0.0f;
    bool   started_  = false;
    bool   flying_   = true;

    //Our own position while flying, so collision push-back does not cut the arc short(飛んでいる間の自分の位置。当たり判定の押し戻しで弧が途切れないように)
    float3 flyPos_ = {0.0f, 0.0f, 0.0f};

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(gravity_), CEREAL_NVP(maxTime_));
        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentScatterFly, 4);
