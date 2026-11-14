#include "Game/Component/ComponentExplosionFx.h"

void ComponentExplosionFx::Init()
{
    Super::Init();
    timer_    = 0.0f;
    released_ = false;
}

void ComponentExplosionFx::Update()
{
    Super::Update();

    timer_ += GetDeltaTime();

    //Done: delete the effect object. Release only marks it, the scene deletes it at the end of the frame
    //(終わり：エフェクトのオブジェクトを消す。Releaseは印をつけるだけで、実際の削除はフレームの最後)
    if(timer_ >= duration_ && !released_) {
        released_ = true;
        Scene::Object::Release(GetOwnerPtr());
    }
}

void ComponentExplosionFx::PostDraw()
{
    Super::PostDraw();

    if(released_)
        return;

    float3        pos    = GetOwner()->GetTranslate();
    DxLib::VECTOR world  = VGet((float)pos.x, (float)pos.y + 10.0f, (float)pos.z);
    DxLib::VECTOR screen = ConvWorldPosToScreenPos(world);
    if(screen.z <= 0.0f || screen.z >= 1.0f)
        return;    //Behind the camera(カメラの後ろ)

    float t = timer_ / duration_;
    if(t > 1.0f)
        t = 1.0f;

    int x      = (int)screen.x;
    int y      = (int)screen.y;
    int rOuter = (int)(maxSize_ * (0.3f + 0.7f * t));
    int rInner = (int)(rOuter * 0.6f);
    int alpha  = (int)(255 * (1.0f - t));

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawCircle(x, y, rOuter, GetColor(255, 120, 20), TRUE);    //Orange outside(外側オレンジ)
    DrawCircle(x, y, rInner, GetColor(255, 230, 80), TRUE);    //Yellow inside(内側黄色)
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);                 //Always reset the blend mode(ブレンドモードは必ず戻す)
}

void ComponentExplosionFx::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("ExplosionFx")) {
            ImGui::DragFloat(u8"秒数", &duration_, 0.01f, 0.05f, 3.0f);
            ImGui::DragFloat(u8"大きさ", &maxSize_, 1.0f, 0.0f, 500.0f);

            //Remove this component from the owner in the GUI(GUI上でオーナーからこのコンポーネントを削除する)
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentExplosionFx)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentExplosionFx)
