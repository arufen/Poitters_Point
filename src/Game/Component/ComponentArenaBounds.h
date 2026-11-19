#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentArenaBounds);

//Invisible wall. Keeps the owner inside the arena box from StageConfig.h (x/z only, never touches y so gravity still works)
//(見えない壁。オーナーをStageConfig.hのアリーナの範囲の中に入れておく(x/zだけ。yは触らないので重力はそのまま動く))
class ComponentArenaBounds : public Component
{
public:
    BP_COMPONENT_DECL(ComponentArenaBounds, u8"アリーナの壁");

    void Init() override;

    //Clamp after everything moved this frame (walking, throwing, collision push-out)(このフレームの移動(歩く・投げる・押し戻し)が全部終わった後に押し戻す)
    void PostUpdate() override;

    void Draw() override;

    void GUI() override;

    //Extra distance from the wall, so a big object does not stick into it. Chainable(壁からの追加の距離。大きい物が壁にめり込まないように。つなげて呼べる)
    ComponentArenaBoundsPtr SetMargin(float margin);

private:
    float margin_     = 0.0f;
    bool  drawBounds_ = false;    //Debug: draw the wall lines(デバッグ用：壁の線を描く)

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(margin_));
        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentArenaBounds, 4);
