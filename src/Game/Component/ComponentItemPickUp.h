#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentItemPickUp);

class ComponentItemPickUp : public Component
{
public:
    BP_COMPONENT_DECL(ComponentItemPickUp, u8"アイテム拾い");

    void Init() override;

    void Update() override;

    void GUI() override;

    //Player calls this when it touches the item(プレイヤーがアイテムに触れた時に呼ぶ)
    void OnPickedUp(Object* holder);

    void ResetRotation();

    //Called when the player throws this item(プレイヤーがアイテムを投げた時に呼ぶ)
    void OnThrow();

    //Item calls this from its OnHit() when it hits "Ground"(アイテムがGroundに当たった時にOnHit()から呼ぶ)
    void OnLanded();

    bool IsHeld() const { return isHeld_; }

private:
    bool    isHeld_     = false;
    bool    isThrown_   = false;                 //!< true while flying in the air(空中を飛んでいる間true)
    bool    isGround_   = false;                 //!< true once it touched the ground(地面に触れたらtrue)
    float3  velocity_   = {0.0f, 0.0f, 0.0f};    //!< speed and direction while thrown(投げている間の速度と方向)
    Object* holder_     = nullptr;               //!< the player holding this item(このアイテムを持っているプレイヤー)
    float   throwSpeed_ = 100.0f;
    float   upBoost_    = 20.0f;
    float   gravity_    = 150.0f;

    //Height above the PLAYER where the item starts when thrown(投げた時にプレイヤーの上からどれだけ高い位置で始めるか)
    float throwHeight_ = 30.0f;

    //Distance in front of the PLAYER where the item starts when thrown(投げた時にプレイヤーの前どれだけ離れた位置で始めるか)
    float throwForward_ = 10.0f;

    //While > 0, ignore OnLanded() calls and use throwPos_ as the real position(0より大きい間はOnLanded()を無視し、throwPos_を本当の位置として使う)
    float throwGraceTimer_ = 0.0f;

    //Our own position while thrown. Collision can push the item to the ground right after the throw, so we do not trust GetTranslate() during the grace time(投げている間の自分の位置。投げた直後に当たり判定が地面へ押し戻すことがあるため、猶予時間の間はGetTranslate()を信用しない)
    float3 throwPos_ = {0.0f, 0.0f, 0.0f};

    //Stops the item once isGround_ is set by OnLanded()(OnLanded()でisGround_が設定されたらアイテムを止める)
    void CheckGround();

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentItemPickUp, 4);
