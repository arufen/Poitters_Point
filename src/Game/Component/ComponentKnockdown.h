#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentKnockdown);

//Getting hit: the character slides back, drops what it carries, and can not act for a moment.
//There is no knockdown animation yet, so a red bar above the head shows how long until they get up.
//(当たった時：キャラは後ろに滑って、持っている物を落とし、少しの間動けない。
// ダウンのアニメはまだないので、頭の上の赤いバーで起き上がるまでの時間を見せる)
class ComponentKnockdown : public Component
{
public:
    BP_COMPONENT_DECL(ComponentKnockdown, u8"ダウン");

    void Init() override;

    void Update() override;

    void PostDraw() override;

    void GUI() override;

    //Knock this character down. awayDir = the direction to slide (only x/z is used), distance = how far.
    //Getting hit again while down restarts the timer
    //(このキャラをダウンさせる。awayDir＝滑る方向(x/zだけ使う)、distance＝滑る距離。ダウン中にまた当たるとタイマーが最初から)
    void Knock(const float3& awayDir, float distance);

    //True while knocked down (can not move, pick up or throw)(ダウン中ならtrue(動けない・拾えない・投げられない))
    bool IsDown() const { return timer_ > 0.0f; }

private:
    float  timer_    = 0.0f;                  //Time left until they get up(起き上がるまでの残り時間)
    float  duration_ = 1.0f;                  //Full knockdown time, for the bar(ダウンの全体の時間。バー用)
    float3 slideVel_ = {0.0f, 0.0f, 0.0f};    //Slide speed, slows down every frame(滑る速さ。毎フレーム遅くなる)

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentKnockdown, 4);
