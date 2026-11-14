#include "Game/Component/ComponentScatterFly.h"
#include "Game/Component/ComponentItemPickUp.h"

void ComponentScatterFly::Init()
{
    //Do not reset velocity_ here. SetVelocity() may be called before Init()(ここでvelocity_をリセットしない。SetVelocity()はInit()より先に呼ばれることがある)
    Super::Init();
}

ComponentScatterFlyPtr ComponentScatterFly::SetVelocity(const float3& v)
{
    velocity_ = v;
    flying_   = true;
    started_  = false;
    timer_    = 0.0f;
    return std::dynamic_pointer_cast<ComponentScatterFly>(shared_from_this());
}

void ComponentScatterFly::Update()
{
    Super::Update();

    if(!flying_)
        return;

    auto owner = GetOwner();

    //Someone grabbed it in the air: stop flying, the holder controls it now(空中で誰かが拾った：飛ぶのをやめる。今は持ち主が動かす)
    if(auto pickup = owner->GetComponent<ComponentItemPickUp>()) {
        if(pickup->IsHeld() || pickup->IsThrown()) {
            flying_ = false;
            return;
        }
    }

    //First frame: remember where we start(最初のフレーム：開始位置を覚える)
    if(!started_) {
        started_ = true;
        flyPos_  = owner->GetTranslate();
        startY_  = (float)flyPos_.y;
    }

    float delta  = GetDeltaTime();
    timer_      += delta;

    velocity_.y -= gravity_ * delta;    //Gravity pulls it down(重力で下に引っ張る)
    flyPos_      = flyPos_ + velocity_ * delta;
    owner->SetTranslate(flyPos_);

    //Stop when falling back to the start height, or after the safety time. Then the collision gravity lands it on the ground
    //(開始の高さまで落ちたら、または安全時間を過ぎたら止まる。あとはコリジョンの重力が地面に乗せる)
    if(((float)velocity_.y < 0.0f && (float)flyPos_.y <= startY_) || timer_ >= maxTime_)
        flying_ = false;
}

void ComponentScatterFly::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("ScatterFly")) {
            //Enable / disable(有効/無効)
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text(flying_ ? "Flying" : "Done");
            ImGui::DragFloat(u8"重力", &gravity_, 0.5f);
            ImGui::DragFloat(u8"最大秒数", &maxTime_, 0.05f);

            //Remove this component from the owner in the GUI(GUI上でオーナーからこのコンポーネントを削除する)
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentScatterFly)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentScatterFly)
