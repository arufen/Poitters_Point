#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentGroundShadow);

//Round shadow on the floor under the owner (a dark see-through circle).
//A line is cast straight down onto the Ground model every frame, so the shadow always sits on the floor
//(and tilts with ramps) instead of following the object up into the air.
//(オーナーの下の床に出る丸い影(半透明の黒い円)。
// 毎フレーム、真下のGroundモデルに線を飛ばすので、影はいつも床の上にある(坂では傾く)。オブジェクトと一緒に空中へ上がらない)
class ComponentGroundShadow : public Component
{
public:
    BP_COMPONENT_DECL(ComponentGroundShadow, u8"丸い影");

    void Init() override;

    void Update() override;

    void Draw() override;

    void GUI() override;

    //Shadow size. Chainable(影の大きさ。つなげて呼べる)
    ComponentGroundShadowPtr SetRadius(float radius);

    //Extra size multiplier, e.g. smaller while the model floats higher. 1.0 = normal(追加の大きさの倍率。例えばモデルが高く浮いている時に小さくする。1.0＝普通)
    void SetSizeScale(float scale) { sizeScale_ = scale; }

private:
    //Cast a line down onto the Ground model. Returns false if there is no floor below (off the stage)
    //(Groundモデルへ下向きに線を飛ばす。下に床がなければfalse(ステージの外))
    bool CastToGround();

    float radius_    = 6.0f;
    float sizeScale_ = 1.0f;
    int   alpha_     = 110;

    //Result of the last raycast(最後のレイキャストの結果)
    bool   hasGround_    = false;
    float3 groundPos_    = {0.0f, 0.0f, 0.0f};
    float3 groundNormal_ = {0.0f, 1.0f, 0.0f};

    //Cached Ground object, so we do not search the scene every frame(シーンを毎フレーム探さないように、Groundオブジェクトを覚えておく)
    std::weak_ptr<::Object> ground_;

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(radius_), CEREAL_NVP(alpha_));
        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentGroundShadow, 4);
