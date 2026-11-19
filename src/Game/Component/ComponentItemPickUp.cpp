#include "Game/Component/ComponentItemPickUp.h"
#include "Game/Component/ComponentStateIdleWalk.h"
#include "Game/MainStage.h"
#include <cmath>

namespace {
//Own name, because DxLib already has a "PI" and the name clashes(DxLibにもう「PI」があって名前がぶつかるので、自分専用の名前にする)
const float PICKUP_PI = 3.14159265f;
}    // namespace

void ComponentItemPickUp::Init()
{
    Super::Init();
    isHeld_   = false;
    isThrown_ = false;    //Item must not start in flight(アイテムは最初から飛んでいてはいけない)
    isGround_ = false;
    velocity_ = {0.0f, 0.0f, 0.0f};
    holder_   = nullptr;
    thrower_  = nullptr;
    hasHit_   = false;
}

void ComponentItemPickUp::Update()
{
    Super::Update();

    //Count down the grace timer after throwing to avoid an instant landing(投げた直後に着地扱いになるのを避けるためタイマーを減らす)
    if(throwGraceTimer_ > 0.0f && isThrown_)
        throwGraceTimer_ -= GetDeltaTime();

    if(isThrown_) {
        float delta = GetDeltaTime();

        float3 pos = GetOwner()->GetTranslate();

        //Right after the throw, collision may push the item to the ground, so use our own position(投げた直後は当たり判定が地面に押し戻すことがあるので、自分の位置を使う)
        if(throwGraceTimer_ > 0.0f)
            pos = throwPos_;
        //Falling, but something pushed us UP since last frame = we landed on top of something (another item, a head...)
        //Only "Ground" calls OnLanded(), so without this an item on top of another item would stay "thrown" forever
        //(落ちているのに前のフレームから上に押し戻された＝何かの上に乗った(別のアイテム・頭など)
        // OnLanded()を呼ぶのは「Ground」だけなので、これがないとアイテムの上に乗ったアイテムがずっと「投げ中」になる)
        else if((float)velocity_.y < 0.0f && (float)pos.y > (float)throwPos_.y + 0.01f)
            isGround_ = true;

        //Landed last frame: bounce or stop(前のフレームで着地した：バウンドするか止まる)
        CheckGround();

        //Safety timer(安全タイマー)
        thrownTime_ += delta;
        if(isThrown_ && thrownTime_ >= maxThrowTime_)
            StopThrow();

        if(!isThrown_)
            return;

        velocity_.y -= gravity_ * delta;    //Gravity pulls the item down(重力でアイテムを下に引っ張る)

        pos.x += velocity_.x * delta;
        pos.y += velocity_.y * delta;
        pos.z += velocity_.z * delta;
        GetOwner()->SetTranslate(pos);

        //Remember the position for the next frame(次のフレームのために位置を覚える)
        throwPos_ = pos;

        //Tumble(回転)
        spinAngle_ += spinNow_ * delta;
        if(spinAngle_ >= 360.0f)
            spinAngle_ -= 360.0f;
        ApplySpin();
    }
    else {
        velocity_ = {0.0f, 0.0f, 0.0f};
    }
}

void ComponentItemPickUp::CheckGround()
{
    //isGround_ is set by OnLanded() (Item::OnHit with "Ground") or by the landed-on-something check in Update()
    //(isGround_はOnLanded()(Item::OnHitで「Ground」の時)か、Update()の「何かの上に乗った」チェックで設定される)
    if(!isGround_)
        return;
    isGround_ = false;

    //Already going up = we just bounced and are still touching the ground. Ignore(もう上に向かっている＝バウンドした直後でまだ地面に触れている。無視する)
    if((float)velocity_.y > 0.0f)
        return;

    //Bounce: flip the up speed and lose some, and slow down forward + spin(バウンド：上向きの速さを反転して少し減らし、前向きの速さと回転も遅くする)
    if(bouncesLeft_ > 0) {
        float up = -(float)velocity_.y * bounceKeepUp_;
        if(up >= bounceMinSpeed_) {
            bouncesLeft_--;
            velocity_.y  = up;
            velocity_.x *= bounceKeepForward_;
            velocity_.z *= bounceKeepForward_;
            spinNow_    *= bounceKeepForward_;
            return;
        }
    }

    //No bounces left, or too slow to bounce: stop here(バウンドが残っていない、または遅すぎる：ここで止まる)
    StopThrow();
}

void ComponentItemPickUp::StopThrow()
{
    isThrown_ = false;
    velocity_ = {0.0f, 0.0f, 0.0f};

    //Stand the model up again so it rests normally on the ground(モデルをまっすぐに戻して、地面に普通に置かれるようにする)
    ResetSpin();
}

ComponentItemPickUpPtr ComponentItemPickUp::SetBounce(int count, float keepUp, float keepForward, float minSpeed)
{
    bounceCount_       = count;
    bounceKeepUp_      = keepUp;
    bounceKeepForward_ = keepForward;
    bounceMinSpeed_    = minSpeed;
    return std::dynamic_pointer_cast<ComponentItemPickUp>(shared_from_this());
}

ComponentItemPickUpPtr ComponentItemPickUp::SetSpin(float degreesPerSecond)
{
    spinSpeed_ = degreesPerSecond;
    return std::dynamic_pointer_cast<ComponentItemPickUp>(shared_from_this());
}

void ComponentItemPickUp::OnHitWall(const float3& normal, float keep)
{
    if(!isThrown_)
        return;

    //Speed going INTO the wall. Negative = moving out of the arena(壁に向かう速さ。マイナス＝アリーナの外へ動いている)
    float into = (float)velocity_.x * (float)normal.x + (float)velocity_.z * (float)normal.z;
    if(into < 0.0f) {
        //Flip that part of the speed and keep only some of it, like a ball off a wall(その分の速さを反転して一部だけ残す。壁に当たったボールのように)
        velocity_.x -= normal.x * into * (1.0f + keep);
        velocity_.z -= normal.z * into * (1.0f + keep);

        //Face the new direction so it keeps tumbling forward(新しい方向を向いて、前に転がり続けるように)
        spinYaw_ = atan2f((float)velocity_.x, (float)velocity_.z) * (180.0f / PICKUP_PI);
    }

    //The wall moved us, so our own remembered position must follow(壁が位置を動かしたので、覚えている位置も合わせる)
    throwPos_ = GetOwner()->GetTranslate();
}

ComponentItemPickUpPtr ComponentItemPickUp::SetModelTransform(float modelScale, const float3& rotationDegrees)
{
    const float toRad = PICKUP_PI / 180.0f;

    //Matrices are applied LEFT to RIGHT in this engine (row vectors, like world = mul(model, owner)).
    //Note: "*" between matrices multiplies each element, so a real matrix product must use mul()
    //(このエンジンの行列は左から右の順にかかる(行ベクトル。world = mul(model, owner)と同じ)。
    // 注意：行列どうしの「*」は要素ごとの掛け算なので、本当の行列の掛け算はmul()を使う)
    matrix fix = mul(mul(matrix::rotateX((float)rotationDegrees.x * toRad),    //
                         matrix::rotateY((float)rotationDegrees.y * toRad)),
                     matrix::rotateZ((float)rotationDegrees.z * toRad));

    modelBase_ = mul(matrix::scale(modelScale), fix);

    ResetSpin();    //Show it right away(すぐに反映する)
    return std::dynamic_pointer_cast<ComponentItemPickUp>(shared_from_this());
}

void ComponentItemPickUp::ApplySpin()
{
    const float toRad = PICKUP_PI / 180.0f;

    //scale -> rotation fix -> forward tumble (X) -> turn to the throw direction (Y)
    //The fix comes first, so whatever rotX/Y/Z a model needs, it still tumbles forward
    //(スケール→回転の補正→前への回転(X)→投げた方向を向く(Y)
    // 補正が最初なので、モデルにどんなrotX/Y/Zが必要でも、前に転がる)
    if(auto model = GetOwner()->GetComponent<ComponentModel>()) {
        matrix tumble   = matrix::rotateX(spinAngle_ * toRad);
        matrix face     = matrix::rotateY(spinYaw_ * toRad);
        model->Matrix() = mul(mul(modelBase_, tumble), face);
    }
}

void ComponentItemPickUp::ResetSpin()
{
    //Back to the resting pose (scale + rotation fix), not to plain 0(ただの0ではなく、普段の姿(スケール＋回転の補正)に戻す)
    spinAngle_ = 0.0f;
    if(auto model = GetOwner()->GetComponent<ComponentModel>())
        model->Matrix() = modelBase_;
}

void ComponentItemPickUp::OnLanded()
{
    //Ignore landing right after the throw(投げた直後の着地判定は無視する)
    if(throwGraceTimer_ > 0.0f)
        return;

    isGround_ = true;
}

void ComponentItemPickUp::OnHitCharacter()
{
    hasHit_ = true;

    //Lose most sideways speed so the item drops near the target instead of flying through(横の速度をほぼなくして、貫通せずに相手の近くに落ちるようにする)
    velocity_.x *= 0.2f;
    velocity_.z *= 0.2f;
}

void ComponentItemPickUp::OnPickedUp(Object* holder)
{
    if(isHeld_)
        return;    //Already held, do not pick up twice(すでに持たれているので二重に拾わない)

    isHeld_   = true;
    isThrown_ = false;    //In case it was still flying(まだ飛んでいた場合に備えて)

    //Turn off the item's own collision so it does not block the player(プレイヤーの邪魔にならないようアイテムの当たり判定を切る)
    if(auto col = GetOwner()->GetComponent<ComponentCollision>())
        col->SetStatus(Component::StatusBit::Enable, false);

    //Reset size(サイズをリセット)
    float3 defaultSize = GetOwner()->GetScaleAxisXYZ();

    //Attach the item to the holder's hand(アイテムを持ち主の手にくっつける)
    auto attach = GetOwner()->AddComponent<ComponentAttachModel>();
    attach->SetAttachObject(holder->GetName(), "Arm:Right:Lower_end_end");
    //attach->SetAttachOffset({12, 17, 0});

    GetOwner()->SetScaleAxisXYZ(defaultSize);

    //Remember the holder to know the throw direction later(あとで投げる方向を知るため持ち主を覚える)
    holder_ = holder;

    //Stand the model up in the hand, in case it was still tumbling(まだ回っていた場合に備えて、手の中ではモデルをまっすぐにする)
    ResetSpin();

    throwGraceTimer_ = 0.2f;
}

void ComponentItemPickUp::OnDropped()
{
    if(!isHeld_)
        return;

    isHeld_   = false;
    isThrown_ = false;    //Not a throw: no damage, no bounce(投げではない：ダメージもバウンドもなし)

    //Start a bit above the holder's feet, so it falls next to them(持ち主の足元の少し上から始めて、すぐそばに落ちる)
    float3 pos = GetOwner()->GetTranslate();
    if(holder_) {
        pos    = holder_->GetTranslate();
        pos.y += throwHeight_;
    }

    //Detach from the hand and stand up straight(手から外して、まっすぐにする)
    GetOwner()->RemoveComponent<ComponentAttachModel>();
    ResetRotation();
    ResetSpin();
    GetOwner()->SetTranslate(pos);

    //Collision back on: the item's gravity drops it onto the floor(当たり判定を戻す：アイテムの重力で床に落ちる)
    if(auto col = GetOwner()->GetComponent<ComponentCollision>())
        col->SetStatus(Component::StatusBit::Enable, true);

    holder_ = nullptr;
}

void ComponentItemPickUp::ResetRotation()
{
    float3 zero{0.0f, 0.0f, 0.0f};
    GetOwner()->SetRotationAxisXYZ(zero);
}

void ComponentItemPickUp::OnThrow()
{
    if(!isHeld_)
        return;

    isHeld_ = false;

    //Get the direction the player is facing, same as ComponentStateThrow(プレイヤーの向きを取得する。ComponentStateThrowと同じ方法)
    float3 direction = {0.0f, 0.0f, 1.0f};    //Fallback if no model is found(モデルが見つからない場合の予備)

    if(holder_) {
        if(auto model = holder_->GetComponent<ComponentModel>())
            direction = normalize(-model->GetWorldVectorAxisZ());
    }

    //Detach from the hand(手から外す)
    GetOwner()->RemoveComponent<ComponentAttachModel>();

    //Reset rotation(回転をリセット)
    ResetRotation();

    //It is flying again, so not on the ground(また飛ぶので地面には触れていない)
    isGround_ = false;

    //Start position comes from the PLAYER, not the item. The item's position can be stale while attached(開始位置はアイテムではなくプレイヤーから決める。アイテムの位置は手にくっついている間は古いことがある)
    auto   owner = GetOwner();
    float3 startPos;

    if(holder_) {
        startPos    = holder_->GetTranslate();                 //Player position(プレイヤーの位置)
        startPos    = startPos + direction * throwForward_;    //A bit in front of the player(プレイヤーの少し前)
        startPos.y += throwHeight_;                            //Lift up from the feet(足元から上に上げる)
    }
    else {
        //No holder, use the old way with the item position(持ち主がいないので、アイテムの位置を使う従来の方法にする)
        startPos    = owner->GetTranslate();
        startPos.y += throwHeight_;
    }

    owner->SetTranslate(startPos);

    //Remember the start position as our own position(開始位置を自分の位置として覚える)
    throwPos_ = startPos;

    //Turn collision back on from the new safe position(新しい安全な位置から当たり判定を戻す)
    if(auto col = GetOwner()->GetComponent<ComponentCollision>())
        col->SetStatus(Component::StatusBit::Enable, true);

    velocity_    = direction * throwSpeed_;
    velocity_.y += upBoost_;
    isThrown_    = true;

    //Fresh bounces, safety timer and tumble for this throw(今回の投げのバウンド数・安全タイマー・回転をリセット)
    bouncesLeft_ = bounceCount_;
    thrownTime_  = 0.0f;
    spinNow_     = spinSpeed_;
    spinAngle_   = 0.0f;
    spinYaw_     = atan2f((float)direction.x, (float)direction.z) * (180.0f / PICKUP_PI);

    //New throw: remember who threw it and allow one hit again(新しい投げ：誰が投げたかを覚えて、もう一度1回だけ当たれるようにする)
    thrower_ = holder_;
    hasHit_  = false;

    //Short window where landings are ignored(着地を無視する短い猶予時間)
    throwGraceTimer_ = 0.2f;

    holder_ = nullptr;
}

void ComponentItemPickUp::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("PickupAbleItem")) {
            //Enable / disable(有効/無効)
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text(isHeld_ ? "Held" : (isThrown_ ? "Thrown" : "On ground"));

            //Basic movement info(移動の基本情報)
            ImGui::DragFloat(u8"投げるの速さ", &throwSpeed_, 0.1f);
            ImGui::DragFloat(u8"上の投げる速さ", &upBoost_, 0.1f);
            ImGui::DragFloat(u8"重力", &gravity_, 0.1f);
            ImGui::DragFloat(u8"投げ始めの高さ", &throwHeight_, 0.1f);
            ImGui::DragFloat(u8"投げ始めの前方", &throwForward_, 0.1f);
            float vel[3] = {velocity_.x, velocity_.y, velocity_.z};
            if(ImGui::DragFloat3(u8"速度", vel, 0.1f))
                velocity_ = {vel[0], vel[1], vel[2]};

            //Remove this component from the owner in the GUI(GUI上でオーナーからこのコンポーネントを削除する)
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentItemPickUp)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentItemPickUp)
