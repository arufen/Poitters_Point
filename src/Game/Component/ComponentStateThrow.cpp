#pragma once
#include "Game/Component/ComponentStateThrow.h"
#include "Game/Component/ComponentStateIdleWalk.h"
#include "Game/TutorialX_Bullet.h"

void ComponentStateThrow::Init()
{
    Super::Init();
    auto model = GetOwner()->GetComponent<ComponentModel>();
    model->PlayAnimationNoSame("throw");

    const float3 up_len = {0.0f, 5.0f, 0.0f};
    float3       vec    = {0.0f, 1.0f, 0.0f};
    if(auto model = GetOwner()->GetComponent<ComponentModel>()) {
        constexpr float front_len = 5.0f;

        vec = -model->GetWorldVectorAxisZ() * front_len;
        model->PlayAnimationNoSame("throw");
    }

    auto bullet = Scene::Object::Create<TutorialX::Bullet>();
    bullet->SetDirection(vec);
    bullet->SetTranslate(GetOwner()->GetTranslate() + up_len + vec);
    //bullet->RemoveComponent<ComponentCollisionSphere>(); //remove collission first
    bullet->GetComponent<ComponentCollisionSphere>()->SetStatus(Component::StatusBit::Enable, false);
    bullet_ = bullet;

    auto attach = bullet->AddComponent<ComponentAttachModel>();
    attach->SetAttachObject("Player", "mixamorig:RightHand");
    attach->SetAttachOffset({12, 17, 0});
}

void ComponentStateThrow::Update()
{
    Super::Update();

    if(auto bullet = bullet_.lock()) {
        auto model     = GetOwner()->GetComponent<ComponentModel>();
        auto anim_time = model->GetAnimationTime();
        if(anim_time >= 1.2f) {
            if(auto attach = bullet->GetComponent<ComponentAttachModel>()) {
                bullet->RemoveComponent(attach);
                //bullet->AddComponent<ComponentCollisionSphere>();
                bullet->GetComponent<ComponentCollisionSphere>()->SetStatus(Component::StatusBit::Enable, true);
            }
        }
    }

    if(auto model = GetOwner()->GetComponent<ComponentModel>()) {
        if(!model->IsPlaying()) {
            RemoveThisComponent();
            GetOwner()->AddComponent<ComponentStateIdleWalk>()->SetMoveSpeed(0.3f)->SetRotateSpeed(20.0f);
        }
    }
}

void ComponentStateThrow::GUI()
{
    __super::GUI();

    // GUI内に出現させる
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("State Throw")) {
            // 有効/無効
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            // GUI上でオーナーから自分(SampleObjectController)を削除します
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentStateThrow)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentStateThrow)
