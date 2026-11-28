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

    //[ADD] While held, put the item between the holder's two hands. Runs after the holder's animation pose is done
    //([追加] 持たれている間、アイテムを持ち主の両手の間に置く。持ち主のアニメの姿勢が決まった後に実行する)
    void PostUpdate() override;

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

    //--------------------------------------------------------------------
    //[ADD] Holding in both hands(両手で持つ)
    //Before: the item was stuck to ONE bone of the right forearm (ComponentAttachModel), and it also copied that bone's
    //rotation, so in a two-handed carry pose it sat off-center and tilted.
    //Now: the item goes to the MIDDLE of the left and right hand bones, and only turns with the direction the holder faces.
    //(前：アイテムは右の前腕のボーン1本にくっついていて(ComponentAttachModel)、そのボーンの回転もコピーしていたので、
    // 両手で持つポーズだと真ん中からずれて、傾いていた。
    // 今：アイテムは左右の手のボーンの「真ん中」に置き、持ち主が向いている方向にだけ回る)
    //--------------------------------------------------------------------
    int    handRight_  = -1;    //Right hand bone index of the holder's model. -1 = not found(持ち主のモデルの右手のボーン番号。-1は見つからない)
    int    handLeft_   = -1;    //Left hand bone index. -1 = not found(左手のボーン番号。-1は見つからない)
    float3 holdScale_  = {1.0f, 1.0f, 1.0f};    //Item size when picked up, kept while held(拾った時のアイテムの大きさ。持っている間はこれを保つ)
    float3 pickupPos_  = {0.0f, 0.0f, 0.0f};    //Where the item was when picked up, for the blend(拾った時のアイテムの位置。なめらかに動かすために使う)
    float  holdBlend_  = 0.0f;                  //0 = still at pickupPos_, 1 = fully in the hands(0＝まだ拾った位置、1＝完全に手の中)
    bool   holdWarped_ = false;                 //Already told the transform "this was a warp, not a move"(「移動ではなくワープ」とトランスフォームに伝えたか)

    //Tuning values. Can be changed live in the ImGui window "PickupAbleItem"(調整用の値。ImGuiの「PickupAbleItem」で実行中に変えられる)
    float holdUp_        = 0.0f;     //Move the item up from the hands' middle(手の真ん中から上へずらす量)
    float holdForward_   = 0.0f;     //Move the item forward (the way the holder faces)(前(持ち主が向いている方)へずらす量)
    float holdBlendTime_ = 0.15f;    //Seconds to move from the ground into the hands(地面から手の中へ移るまでの秒数)

    //Used only when the holder's model has no hand bones we can find(持ち主のモデルに手のボーンが見つからない時だけ使う)
    float holdFallbackHeight_ = 18.0f;    //Height above the holder's feet(持ち主の足元からの高さ)
    float holdFallbackFront_  = 6.0f;     //Distance in front of the holder(持ち主の前方への距離)

    //Find the holder's hand bones and remember their indices(持ち主の手のボーンを探して番号を覚える)
    void FindHandBones();

    //Where the item should be now, and which way it should face (flat X and Z axes)
    //(今アイテムを置くべき位置と、向くべき方向(水平なX軸とZ軸))
    void GetHoldPose(float3& outPos, float3& outAxisX, float3& outAxisZ) const;

    //End the throw: not flying anymore, can be picked up again(投げを終わる：もう飛んでいない。また拾える)
    void StopThrow();

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentItemPickUp, 4);
