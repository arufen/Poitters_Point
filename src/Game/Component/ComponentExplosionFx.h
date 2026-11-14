#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentExplosionFx);

//Simple blast drawn with DxLib circles. Grows, fades, then deletes its owner. Swap for Effekseer later
//(DxLibの円でかんたんな爆発を描く。大きくなって消えて、最後にオーナーを消す。あとでEffekseerに変える)
class ComponentExplosionFx : public Component
{
public:
    BP_COMPONENT_DECL(ComponentExplosionFx, u8"爆発エフェクト");

    void Init() override;

    void Update() override;

    void PostDraw() override;

    void GUI() override;

private:
    float timer_    = 0.0f;
    float duration_ = 0.35f;    //Seconds the blast is shown(爆発を出す秒数)
    float maxSize_  = 90.0f;    //Max circle radius in pixels(円の最大半径(ピクセル))
    bool  released_ = false;    //Do not release twice(二重にReleaseしない)

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(duration_), CEREAL_NVP(maxSize_));
        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentExplosionFx, 4);
