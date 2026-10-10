#include "Game/Component/ComponentItemPickUp.h"
#include "Game/Component/ComponentStateIdleWalk.h"
#include "Game/MainStage.h"

void ComponentItemPickUp::Init()
{
    Super::Init();
    isHeld_   = false;
    isThrown_ = false;    //Item must not start in flight(アイテムは最初から飛んでいてはいけない)
    isGround_ = false;
    velocity_ = {0.0f, 0.0f, 0.0f};
    holder_   = nullptr;
}

void ComponentItemPickUp::Update()
{
    Super::Update();

    //Count down the grace timer after throwing to avoid an instant landing(投げた直後に着地扱いになるのを避けるためタイマーを減らす)
    if(throwGraceTimer_ > 0.0f && isThrown_)
        throwGraceTimer_ -= GetDeltaTime();

    if(isThrown_) {
        float delta = GetDeltaTime();

        velocity_.y -= gravity_ * delta;    //Gravity pulls the item down(重力でアイテムを下に引っ張る)

        float3 pos = GetOwner()->GetTranslate();

        //Right after the throw, collision may push the item to the ground, so use our own position(投げた直後は当たり判定が地面に押し戻すことがあるので、自分の位置を使う)
        if(throwGraceTimer_ > 0.0f)
            pos = throwPos_;

        pos.x += velocity_.x * delta;
        pos.y += velocity_.y * delta;
        pos.z += velocity_.z * delta;
        GetOwner()->SetTranslate(pos);

        //Remember the position for the next frame(次のフレームのために位置を覚える)
        throwPos_ = pos;

        CheckGround();
    }
    else {
        velocity_ = {0.0f, 0.0f, 0.0f};
    }
}

void ComponentItemPickUp::CheckGround()
{
    //isGround_ is set by OnLanded(), which Item::OnHit() calls when touching the ground(isGround_はItem::OnHit()から呼ばれるOnLanded()で設定される)
    if(isGround_) {
        isThrown_ = false;
        velocity_ = {0.0f, 0.0f, 0.0f};
    }
}

void ComponentItemPickUp::OnLanded()
{
    //Ignore landing right after the throw(投げた直後の着地判定は無視する)
    if(throwGraceTimer_ > 0.0f)
        return;

    isGround_ = true;
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

    throwGraceTimer_ = 0.2f;
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
