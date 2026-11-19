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
    bool IsThrown() const { return isThrown_; }

    //Flying speed and direction while thrown (zero when not thrown). Used for the knockback direction
    //(投げられている間の速さと方向(投げていない時は0)。ふっとぶ方向に使う)
    const float3& GetVelocity() const { return velocity_; }

    //True only while flying and before it hit anyone(飛んでいる間で、まだ誰にも当たっていない時だけtrue)
    bool CanDealDamage() const { return isThrown_ && !hasHit_; }

    //Who threw this item. Only compare it, never use it as a live pointer(誰が投げたか。比較にだけ使い、そのまま触らない)
    const Object* GetThrower() const { return thrower_; }

    //Item calls this after it hurt a character. Only one hit per throw, and it loses most sideways speed(キャラに当たってダメージを与えた後にアイテムが呼ぶ。1回の投げで1回だけ当たり、横の速度はほぼなくなる)
    void OnHitCharacter();

    //The holder let go without throwing (e.g. K.O.). The item falls straight down near the holder and can be picked up again
    //(持ち主が投げずに手を離した(K.O.など)。アイテムは持ち主の近くにそのまま落ちて、また拾える)
    void OnDropped();

    //How it bounces after a throw. count 0 = stops on the first landing (chest, TNT). Chainable
    //(投げた後のバウンドの仕方。countが0なら最初の着地で止まる(チェスト・TNT)。つなげて呼べる)
    ComponentItemPickUpPtr SetBounce(int count, float keepUp, float keepForward, float minSpeed);

    //Tumble speed while flying, degrees per second. Only the MODEL turns, so the collision does not move. Chainable
    //(飛んでいる間の回転の速さ(度/秒)。回るのはモデルだけなので、当たり判定は動かない。つなげて呼べる)
    ComponentItemPickUpPtr SetSpin(float degreesPerSecond);

    //Resting model transform: real model scale (e.g. 0.1) and a rotation fix in degrees (X, then Y, then Z) for models
    //exported facing the wrong way. The throw tumble is applied AFTER this fix, so the fix never changes the tumble direction. Chainable
    //(モデルの普段の姿：本当のモデルスケール(例 0.1)と、向きがずれて出力されたモデル用の回転の補正(度。X→Y→Zの順)。
    // 投げた時の回転はこの補正の「後」にかかるので、補正で回転の向きが変わることはない。つなげて呼べる)
    ComponentItemPickUpPtr SetModelTransform(float modelScale, const float3& rotationDegrees);

    //ComponentArenaBounds calls this when a thrown item was pushed back from the wall. normal points into the arena
    //(投げたアイテムが壁から押し戻された時にComponentArenaBoundsが呼ぶ。normalはアリーナの内側を向く)
    void OnHitWall(const float3& normal, float keep);

private:
    bool    isHeld_     = false;
    bool    isThrown_   = false;                 //!< true while flying in the air(空中を飛んでいる間true)
    bool    isGround_   = false;                 //!< true once it touched the ground(地面に触れたらtrue)
    float3  velocity_   = {0.0f, 0.0f, 0.0f};    //!< speed and direction while thrown(投げている間の速度と方向)
    Object* holder_     = nullptr;               //!< the player holding this item(このアイテムを持っているプレイヤー)
    Object* thrower_    = nullptr;               //!< who threw it last, used to skip self-hit(最後に投げた人。自分に当たるのを防ぐために使う)
    bool    hasHit_     = false;    //!< true after it hurt someone, so it hits once per throw(誰かにダメージを与えたらtrue。1回の投げで1回だけ当たる)
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

    //Bounce settings (set by Item::SetItemType from StageConfig.h)(バウンドの設定(Item::SetItemTypeがStageConfig.hから設定する))
    int   bounceCount_       = 0;        //Bounces per throw(1回の投げでのバウンド数)
    float bounceKeepUp_      = 0.5f;     //Up speed kept after a bounce(バウンド後に残る上向きの速さ)
    float bounceKeepForward_ = 0.65f;    //Forward speed kept after a bounce(バウンド後に残る前向きの速さ)
    float bounceMinSpeed_    = 5.0f;     //Slower than this = stop instead of bouncing(これより遅いならバウンドせずに止まる)
    int   bouncesLeft_       = 0;        //Bounces left in this throw(今回の投げで残っているバウンド数)

    //Tumble while flying(飛んでいる間の回転)
    float  spinSpeed_ = 540.0f;                 //Setting, degrees per second(設定。度/秒)
    float  spinNow_   = 0.0f;                   //Current speed, slows down on every bounce(今の速さ。バウンドするたびに遅くなる)
    float  spinAngle_ = 0.0f;                   //Current tumble angle(今の回転角度)
    float  spinYaw_   = 0.0f;                   //Throw direction, so it tumbles forward(投げた方向。前に転がるように回る)
    matrix modelBase_ = matrix::scale(0.1f);    //Resting model matrix = scale x rotation fix(モデルの普段の行列＝スケール×回転の補正)

    //Turn the model only (not the object, so the collision capsule stays put)(モデルだけを回す(オブジェクトは回さないので当たり判定はそのまま))
    void ApplySpin();
    void ResetSpin();

    //Safety: stop after this many seconds in the air, so an item can never stay "thrown" (unpickable) forever
    //(安全のため：空中でこの秒数たったら止める。アイテムがずっと「投げ中」(拾えない)にならないように)
    float thrownTime_   = 0.0f;
    float maxThrowTime_ = 5.0f;

    //A landing happened (isGround_): bounce if bounces are left, otherwise stop(着地した(isGround_)：バウンドが残っていれば跳ねて、なければ止まる)
    void CheckGround();

    //End the throw: not flying anymore, can be picked up again(投げを終わる：もう飛んでいない。また拾える)
    void StopThrow();

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentItemPickUp, 4);
