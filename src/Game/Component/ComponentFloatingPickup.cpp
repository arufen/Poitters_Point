#include "Game/Component/ComponentFloatingPickup.h"
#include "Game/Component/ComponentScatterFly.h"
#include "Game/Component/ComponentGroundShadow.h"
#include "Game/StageConfig.h"
#include <System/Component/ComponentModel.h>
#include <cmath>

namespace SC = PoittersPoint::StageConfig;

namespace {
//Own name, because DxLib already has a "PI" and the name clashes(DxLibにもう「PI」があって名前がぶつかるので、自分専用の名前にする)
const float FLOAT_PI = 3.14159265f;

//The height must stay still this long to count as landed(着地とみなすには、高さがこの秒数変わらないこと)
const float LAND_STILL_TIME = 0.15f;
//Height change smaller than this counts as "not moving"(これより小さい高さの変化は「動いていない」とみなす)
const float LAND_EPSILON = 0.05f;
}    // namespace

void ComponentFloatingPickup::Init()
{
    Super::Init();

    lift_      = SC::FLOAT_LIFT;
    amplitude_ = SC::FLOAT_AMPLITUDE;
    speed_     = SC::FLOAT_SPEED;
    spinSpeed_ = SC::FLOAT_SPIN_SPEED;

    phase_     = (float)GetRand(1000) * 0.001f * 2.0f * FLOAT_PI;
    spinAngle_ = (float)GetRand(359);
    started_   = false;
    stillTime_ = 0.0f;
    lastY_     = (float)GetOwner()->GetTranslate().y;
}

bool ComponentFloatingPickup::HasLanded()
{
    auto owner = GetOwner();

    //Still flying out of a chest(まだチェストから飛び出している)
    if(auto fly = owner->GetComponent<ComponentScatterFly>()) {
        if(fly->IsFlying()) {
            stillTime_ = 0.0f;
            lastY_     = (float)owner->GetTranslate().y;
            return false;
        }
    }

    //Still falling: the height keeps changing(まだ落ちている：高さが変わり続けている)
    float y = (float)owner->GetTranslate().y;
    if(fabsf(y - lastY_) < LAND_EPSILON)
        stillTime_ += GetDeltaTime();
    else
        stillTime_ = 0.0f;
    lastY_ = y;

    return stillTime_ >= LAND_STILL_TIME;
}

void ComponentFloatingPickup::Update()
{
    Super::Update();

    auto owner = GetOwner();
    auto model = owner->GetComponent<ComponentModel>();
    if(!model)
        return;

    if(!started_) {
        if(!HasLanded())
            return;

        //Remember the model's resting matrix (its scale) and build on top of it every frame(モデルの普段の行列(スケール)を覚えて、毎フレームその上に重ねる)
        started_   = true;
        timer_     = 0.0f;
        modelBase_ = model->GetMatrix();
    }

    float delta  = GetDeltaTime();
    timer_      += delta;
    spinAngle_  += spinSpeed_ * delta;
    if(spinAngle_ >= 360.0f)
        spinAngle_ -= 360.0f;

    //wave: -1..1(波：-1〜1)
    float wave   = sinf(timer_ * speed_ * 2.0f * FLOAT_PI + phase_);
    float height = lift_ + amplitude_ * wave;

    //scale -> slow turn -> lift. mul() = real matrix product, applied left to right(スケール→ゆっくり回転→持ち上げ。mul()＝本当の行列の掛け算。左から右の順)
    matrix spin     = matrix::rotateY(spinAngle_ * (FLOAT_PI / 180.0f));
    matrix up       = matrix::translate(0.0f, height, 0.0f);
    model->Matrix() = mul(mul(modelBase_, spin), up);

    //Shadow gets smaller as the item goes up(アイテムが上がるほど影は小さくなる)
    if(auto shadow = owner->GetComponent<ComponentGroundShadow>()) {
        float top = (wave + 1.0f) * 0.5f;    //0 at the bottom, 1 at the top(一番下で0、一番上で1)
        shadow->SetSizeScale(1.0f - SC::FLOAT_SHADOW_SHRINK * top);
    }
}

void ComponentFloatingPickup::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("FloatingPickup")) {
            //Enable / disable(有効/無効)
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text(started_ ? "Floating" : "Waiting to land");
            ImGui::DragFloat(u8"浮く高さ", &lift_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat(u8"上下の幅", &amplitude_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat(u8"上下の速さ", &speed_, 0.01f, 0.0f, 5.0f);
            ImGui::DragFloat(u8"回転の速さ", &spinSpeed_, 1.0f, 0.0f, 720.0f);

            //Remove this component from the owner in the GUI(GUI上でオーナーからこのコンポーネントを削除する)
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentFloatingPickup)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentFloatingPickup)
