#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentKnockdown);

//Getting hit: the character slides back, drops what it carries, and can not act for a moment.
//A red bar above the head shows how long until they get up.
//[ADD] The character now also FALLS DOWN and GETS UP again. After a K.O. the character stays lying on the ground.
//  - Model with "down" and "getup" animations (the Player: OR_Anims/Down.mv1, GetUp.mv1)
//      -> it turns to face the hit, then plays those animations (real bone animation)
//  - Model without them (NPCs for now) -> the whole model is tilted in code, as a fallback
//(当たった時：キャラは後ろに滑って、持っている物を落とし、少しの間動けない。頭の上の赤いバーで起き上がるまでの時間を見せる。
// [追加] キャラが「倒れて」、また「起き上がる」ようになった。K.O.された後は倒れたままになる。
//  - 「down」と「getup」のアニメがあるモデル(プレイヤー：OR_Anims/Down.mv1, GetUp.mv1)
//      → 当たった方を向いてから、そのアニメを再生する(ボーンが動く本物のアニメ)
//  - アニメがないモデル(今はNPC) → 予備として、モデル全体をコードで傾ける)
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

    //[ADD] True while the down/getup ANIMATION owns the model (also while lying after a K.O.).
    //The owner must not play "idle" over it during this time (see Player::Update)
    //([追加] down/getupの「アニメ」がモデルを動かしている間true(K.O.で倒れたままの間も)。
    // この間、持ち主は上から「idle」を再生してはいけない(Player::Updateを参照))
    bool IsPlayingKnockAnimation() const { return animActive_; }

    //[ADD] Knockback distance x scale. 1.0 = normal, bigger = blown further (cha0002 CVS = 1.6). Chainable
    //([追加] ふっとぶ距離×scale。1.0＝普通、大きいほど遠くへ飛ぶ(cha0002 CVS＝1.6)。つなげて呼べる)
    ComponentKnockdownPtr SetKnockbackScale(float scale)
    {
        knockbackScale_ = scale;
        return std::dynamic_pointer_cast<ComponentKnockdown>(shared_from_this());
    }

    //[ADD] Animation names the model must register to use the animation files([追加]アニメのファイルを使うためにモデルが登録する名前)
    static constexpr const char* ANIM_DOWN  = "down";
    static constexpr const char* ANIM_GETUP = "getup";

private:
    float  timer_          = 0.0f;                  //Time left until they get up(起き上がるまでの残り時間)
    float  duration_       = 1.0f;                  //Full knockdown time, for the bar(ダウンの全体の時間。バー用)
    float3 slideVel_       = {0.0f, 0.0f, 0.0f};    //Slide speed, slows down every frame(滑る速さ。毎フレーム遅くなる)
    float  knockbackScale_ = 1.0f;                  //[ADD] Knockback distance x this([追加] ふっとぶ距離×これ)

    //--------------------------------------------------------------------
    //[ADD] Animation-file mode (Player)(アニメファイルのモード(プレイヤー))
    //Timeline of one knockdown (KNOCKDOWN_TIME = 1.0 s):
    //  0.00 - 0.40 s : "down"  hit, fly back, land on the back, small bounce   (downAnimTime_)
    //  0.40 - 0.60 s : lie on the back (the last frame of "down" is held)
    //  0.60 - 1.00 s : "getup" sit up, crouch, stand                          (getUpAnimTime_)
    //(1回のダウンの流れ(KNOCKDOWN_TIME＝1.0秒)：
    //  0.00〜0.40秒：「down」当たる→後ろに飛ぶ→背中から落ちる→小さくバウンド(downAnimTime_)
    //  0.40〜0.60秒：仰向けに倒れている(「down」の最後のフレームのまま)
    //  0.60〜1.00秒：「getup」起き上がる→しゃがむ→立つ(getUpAnimTime_))
    //--------------------------------------------------------------------
    bool  animActive_    = false;    //The animations own the model now(今アニメがモデルを動かしている)
    bool  getUpStarted_  = false;    //"getup" was started for this knockdown(このダウンで「getup」を始めた)
    float downAnimTime_  = 0.40f;    //Seconds to play the whole "down" animation(「down」アニメ全体を再生する秒数)
    float getUpAnimTime_ = 0.40f;    //Seconds to play the whole "getup" animation(「getup」アニメ全体を再生する秒数)

    //The model has both animations registered(モデルに両方のアニメが登録されているか)
    bool HasKnockAnimations() const;

    //--------------------------------------------------------------------
    //[ADD] Fallback: tilt the whole model (models without the animations)(予備：モデル全体を傾ける(アニメがないモデル))
    //Timeline of one knockdown (KNOCKDOWN_TIME = 1.0 s):
    //  0.00 - 0.20 s : fall backward with a small hop   (fallTime_)
    //  0.20 - 0.65 s : lie on the ground
    //  0.65 - 1.00 s : get up                            (getUpTime_)
    //(1回のダウンの流れ(KNOCKDOWN_TIME＝1.0秒)：
    //  0.00〜0.20秒：小さく跳ねながら後ろに倒れる(fallTime_)
    //  0.20〜0.65秒：地面に倒れている
    //  0.65〜1.00秒：起き上がる(getUpTime_))
    //--------------------------------------------------------------------
    bool   fallEnabled_ = true;                  //Turn the animation on/off (ImGui)(アニメのオン/オフ(ImGui))
    float  fallMaxDeg_  = 80.0f;                 //How far the body tilts. 90 = flat on the ground(体が傾く角度。90で地面にぺったり)
    float  fallTime_    = 0.20f;                 //Seconds to fall(倒れるまでの秒数)
    float  getUpTime_   = 0.35f;                 //Seconds to get up, at the end of the knockdown(ダウンの最後に起き上がる秒数)
    float  hopHeight_   = 3.0f;                  //Small jump while falling, like being knocked off your feet(倒れる間の小さなジャンプ。足をすくわれたように)
    bool   poseSaved_   = false;                 //True while we are changing the model's pose(モデルの姿勢を変えている間true)
    matrix basePose_    = matrix::identity();    //Model matrix from before the fall, put back when standing up(倒れる前のモデル行列。起き上がった時に戻す)
    float3 fallDir_     = {0.0f, 0.0f, 1.0f};    //Direction the body falls toward (flat, length 1)(体が倒れる方向(水平・長さ1))
    float  fallSign_    = 1.0f;                  //+1 or -1, so the HEAD moves toward fallDir_ (see Knock)(頭がfallDir_の方へ動くように+1か-1(Knockを参照))
    float  fallDegNow_  = 0.0f;                  //Current tilt, only for the GUI(今の傾き。GUI表示用)

    //Tilt angle (degrees) for the current moment of the knockdown. dead = stay down(ダウン中の今の傾き(度)。deadなら倒れたまま)
    float GetFallDegrees(bool dead) const;

    //Small hop height for the current moment(今の小さなジャンプの高さ)
    float GetHopHeight() const;

    //Tilt the model by "degrees" toward fallDir_, pivoting at the feet(足元を中心に、fallDir_の方へ「degrees」度モデルを傾ける)
    void ApplyFallPose(float degrees, float hop);

    //Stand the model up again with the matrix from before the fall(倒れる前の行列でモデルをまた立たせる)
    void RestorePose();

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentKnockdown, 4);
