#include "Game/Component/ComponentItemPickUp.h"
#include "Game/Component/ComponentStateIdleWalk.h"
#include "Game/MainStage.h"
#include <cmath>
#include <cctype>
#include <string>

namespace {
//Own name, because DxLib already has a "PI" and the name clashes(DxLibにもう「PI」があって名前がぶつかるので、自分専用の名前にする)
const float PICKUP_PI = 3.14159265f;

//[ADD] Lower-case copy, so "RightHand" and "righthand" are treated the same([追加]小文字にしたコピー。「RightHand」と「righthand」を同じとみなすため)
std::string ToLowerText(std::string_view text)
{
    std::string out(text);
    for(char& c : out) c = (char)std::tolower((unsigned char)c);
    return out;
}

//[ADD] Find one hand bone of a character model. Returns the bone index, or -1 if there is none
//([追加]キャラモデルの片手のボーンを探す。ボーン番号を返す。なければ-1)
int FindHandBone(ComponentModel& model, bool right)
{
    //1. Exact names. This project's Salaryman model first, then common Mixamo names (for other characters)
    //(1. 名前がぴったり一致するもの。まずこのプロジェクトのサラリーマンモデル、次にMixamoでよくある名前(ほかのキャラ用))
    static const char* RIGHT_NAMES[] = {"Arm:Right:Lower_end_end", "Arm:Right:Lower_end", "mixamorig:RightHand", "RightHand"};
    static const char* LEFT_NAMES[]  = {"Arm:Left:Lower_end_end", "Arm:Left:Lower_end", "mixamorig:LeftHand", "LeftHand"};

    for(const char* name : (right ? RIGHT_NAMES : LEFT_NAMES)) {
        int no = model.GetNodeIndex(name);
        if(no >= 0)
            return no;
    }

    //2. Fuzzy: the first bone whose name has "right"/"left" AND "hand" (parents come before fingers, so this is the hand)
    //(2. あいまい検索：名前に「right/left」と「hand」の両方が入っている最初のボーン(親が指より先に来るので、これが手になる))
    const char* side  = right ? "right" : "left";
    auto        names = model.GetNodesName();
    for(int i = 0; i < (int)names.size(); ++i) {
        std::string lower = ToLowerText(names[i]);
        if(lower.find(side) != std::string::npos && lower.find("hand") != std::string::npos)
            return i;
    }

    return -1;    //No hand bone. GetHoldPose() then uses a fixed spot in front of the chest(手のボーンなし。GetHoldPose()は胸の前の決まった位置を使う)
}
}    // namespace

void ComponentItemPickUp::Init()
{
    Super::Init();

    //[ADD] Run PostUpdate() LAST, after every character has finished its animation pose this frame.
    //Otherwise the hand bones would still be in last frame's position and the item would lag behind the hands
    //([追加]PostUpdate()を「最後」に実行する。このフレームで全キャラのアニメの姿勢が決まった後。
    // そうしないと手のボーンが前のフレームの位置のままで、アイテムが手から遅れてついてくる)
    SetPriority(ProcTiming::PostUpdate, ProcPriority::LOWEST);
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

//[ADD] Put the held item between the holder's hands. Called every frame AFTER all animations (see SetPriority in Init)
//([追加] 持っているアイテムを持ち主の両手の間に置く。全部のアニメの後に毎フレーム呼ばれる(InitのSetPriorityを参照))
void ComponentItemPickUp::PostUpdate()
{
    Super::PostUpdate();

    if(!isHeld_ || !holder_)
        return;

    float3 target, axisX, axisZ;
    GetHoldPose(target, axisX, axisZ);

    //Move from the ground into the hands over holdBlendTime_ seconds, so the item does not pop into the hands
    //(holdBlendTime_秒かけて地面から手の中へ動かす。アイテムが急に手の中へ飛ばないように)
    if(holdBlendTime_ > 0.0f) {
        holdBlend_ += GetDeltaTime() / holdBlendTime_;
        if(holdBlend_ > 1.0f)
            holdBlend_ = 1.0f;
    }
    else {
        holdBlend_ = 1.0f;
    }

    //Smoothstep: starts slow, ends slow(スムーズステップ：ゆっくり始まって、ゆっくり終わる)
    float  t   = holdBlend_ * holdBlend_ * (3.0f - 2.0f * holdBlend_);
    float3 pos = pickupPos_ + (target - pickupPos_) * t;

    //Build the item's matrix: face the same way as the holder, always upright, keep its own size
    //(アイテムの行列を作る：持ち主と同じ方向を向き、いつもまっすぐ立てて、自分の大きさを保つ)
    auto owner = GetOwner();
    owner->SetVectorAxisX(axisX * (float)holdScale_.x);
    owner->SetVectorAxisY(float3(0.0f, 1.0f, 0.0f) * (float)holdScale_.y);
    owner->SetVectorAxisZ(axisZ * (float)holdScale_.z);
    owner->SetTranslate(pos);

    //The first frame is a jump from the ground, not a move. Tell the transform, same as ComponentAttachModel did
    //(最初のフレームは地面からのジャンプで、移動ではない。ComponentAttachModelと同じようにトランスフォームに伝える)
    if(!holdWarped_) {
        holdWarped_ = true;
        owner->UseWarp();
    }
}

//[ADD] Look up the holder's hand bones once, when the item is picked up([追加]拾った時に一度だけ、持ち主の手のボーンを探す)
void ComponentItemPickUp::FindHandBones()
{
    handRight_ = -1;
    handLeft_  = -1;

    if(!holder_)
        return;

    if(auto model = holder_->GetComponent<ComponentModel>()) {
        handRight_ = FindHandBone(*model, true);
        handLeft_  = FindHandBone(*model, false);
    }
}

//[ADD] Where the held item should be, and which way it faces([追加]持っているアイテムを置く位置と、向く方向)
void ComponentItemPickUp::GetHoldPose(float3& outPos, float3& outAxisX, float3& outAxisZ) const
{
    const float3 up = {0.0f, 1.0f, 0.0f};

    //Facing: the holder MODEL's X and Z axes, made flat (no tilt) and length 1
    //(向き：持ち主の「モデル」のX軸とZ軸を、水平(傾きなし)で長さ1にしたもの)
    outAxisX = {1.0f, 0.0f, 0.0f};
    outAxisZ = {0.0f, 0.0f, 1.0f};

    auto model = holder_->GetComponent<ComponentModel>();
    if(model) {
        matrix world = model->GetWorldMatrix();
        float3 ax    = world.axisX();
        float3 az    = world.axisZ();
        ax.y         = 0.0f;
        az.y         = 0.0f;
        if((float)length(ax) > 0.0001f)
            outAxisX = normalize(ax);
        if((float)length(az) > 0.0001f)
            outAxisZ = normalize(az);
    }

    //The models look toward -Z, same as OnThrow() uses(モデルは-Z方向を向いている。OnThrow()と同じ)
    float3 front = -outAxisZ;

    //Middle of both hands. If only one hand was found, use that one. If none, a fixed spot in front of the chest
    //(両手の真ん中。片手しか見つからなければその手。どちらもなければ胸の前の決まった位置)
    float3 center;
    if(model && handRight_ >= 0 && handLeft_ >= 0)
        center = (model->GetNodePosition(handRight_) + model->GetNodePosition(handLeft_)) * 0.5f;
    else if(model && handRight_ >= 0)
        center = model->GetNodePosition(handRight_);
    else if(model && handLeft_ >= 0)
        center = model->GetNodePosition(handLeft_);
    else
        center = holder_->GetTranslate() + up * holdFallbackHeight_ + front * holdFallbackFront_;

    outPos = center + up * holdUp_ + front * holdForward_;
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

    //Remember the holder to know the throw direction later(あとで投げる方向を知るため持ち主を覚える)
    holder_ = holder;

    //[CHANGE] No ComponentAttachModel any more. It found the holder by NAME (and there could be 2 objects called "Player"),
    //and it followed only the right forearm. Now PostUpdate() puts the item between BOTH hands every frame
    //([変更]ComponentAttachModelはもう使わない。持ち主を「名前」で探していて(「Player」という名前のオブジェクトが2つあることもあった)、
    // 右の前腕しか追わなかった。今はPostUpdate()が毎フレーム「両手」の間にアイテムを置く)
    holdScale_  = GetOwner()->GetScaleAxisXYZ();    //Keep the item's own size(アイテム自身の大きさを保つ)
    pickupPos_  = GetOwner()->GetTranslate();       //Start the blend from where it lies(置いてある位置からなめらかに動かし始める)
    holdBlend_  = 0.0f;
    holdWarped_ = false;
    FindHandBones();

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

    //Stand up straight. isHeld_ is false now, so PostUpdate() stops following the hands(まっすぐにする。isHeld_がfalseなので、PostUpdate()はもう手を追わない)
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

    //isHeld_ is false now, so PostUpdate() stops following the hands(isHeld_がfalseなので、PostUpdate()はもう手を追わない)

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

            //[ADD] Hold position tuning. Change these while holding the item to see the result live
            //([追加] 持つ位置の調整。アイテムを持ったまま変えると、すぐに結果が見える)
            ImGui::Separator();
            ImGui::Text(u8"持つ位置 / Hold position");
            ImGui::Text("Hand bones R:%d L:%d (-1 = not found)", handRight_, handLeft_);
            ImGui::DragFloat(u8"持つ位置 上(Up)", &holdUp_, 0.1f);
            ImGui::DragFloat(u8"持つ位置 前(Forward)", &holdForward_, 0.1f);
            ImGui::DragFloat(u8"手に移る秒数(Blend sec)", &holdBlendTime_, 0.01f, 0.0f, 1.0f);

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
