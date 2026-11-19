#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentFloatingPickup);

//"Pick me up" look for consumables: the MODEL bobs up and down and turns slowly. The object itself does not move,
//so the pickup range and the collision stay exactly where they were. Starts only after the item has landed.
//(消費アイテムの「拾って」という見た目：モデルが上下に浮き沈みしてゆっくり回る。オブジェクト自体は動かないので、
// 拾える範囲と当たり判定はそのまま。アイテムが着地してから始まる)
class ComponentFloatingPickup : public Component
{
public:
    BP_COMPONENT_DECL(ComponentFloatingPickup, u8"浮くアイテム");

    void Init() override;

    void Update() override;

    void GUI() override;

private:
    //True when the item is resting on the floor: not flying out of a chest and not falling(床に乗っている時true：チェストから飛び出し中でも落下中でもない)
    bool HasLanded();

    bool   started_   = false;    //Floating has begun(浮き始めた)
    float  lastY_     = 0.0f;     //Height last frame, to see if it is still falling(前のフレームの高さ。まだ落ちているかを見る)
    float  stillTime_ = 0.0f;     //How long the height has not changed(高さが変わっていない時間)
    float  timer_     = 0.0f;
    float  phase_     = 0.0f;    //Random start, so items do not all bob together(ランダムな開始位置。全部のアイテムが同時に浮き沈みしないように)
    float  spinAngle_ = 0.0f;
    matrix modelBase_ = matrix::scale(0.1f);    //Model matrix when floating started (its scale)(浮き始めた時のモデルの行列(スケール))

    float lift_      = 4.0f;
    float amplitude_ = 2.5f;
    float speed_     = 0.6f;
    float spinSpeed_ = 90.0f;

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(lift_), CEREAL_NVP(amplitude_), CEREAL_NVP(speed_), CEREAL_NVP(spinSpeed_));
        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
};

CEREAL_CLASS_VERSION(ComponentFloatingPickup, 4);
