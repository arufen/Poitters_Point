#include "Game/Component/ComponentArenaBounds.h"
#include "Game/Component/ComponentItemPickUp.h"
#include "Game/StageConfig.h"

namespace SC = PoittersPoint::StageConfig;

void ComponentArenaBounds::Init()
{
    Super::Init();
}

ComponentArenaBoundsPtr ComponentArenaBounds::SetMargin(float margin)
{
    margin_ = margin;
    return std::dynamic_pointer_cast<ComponentArenaBounds>(shared_from_this());
}

void ComponentArenaBounds::PostUpdate()
{
    Super::PostUpdate();

    auto owner = GetOwner();

    //While an item is in someone's hand, the hand controls the position. Do not fight it
    //(アイテムが手に持たれている間は、手が位置を決める。じゃましない)
    if(auto pickup = owner->GetComponent<ComponentItemPickUp>()) {
        if(pickup->IsHeld())
            return;
    }

    float3 pos = owner->GetTranslate();
    float  x   = (float)pos.x;
    float  z   = (float)pos.z;

    float minX = SC::ARENA_MIN_X + margin_;
    float maxX = SC::ARENA_MAX_X - margin_;
    float minZ = SC::ARENA_MIN_Z + margin_;
    float maxZ = SC::ARENA_MAX_Z - margin_;

    bool changed = false;
    if(x < minX) {
        x       = minX;
        changed = true;
    }
    if(x > maxX) {
        x       = maxX;
        changed = true;
    }
    if(z < minZ) {
        z       = minZ;
        changed = true;
    }
    if(z > maxZ) {
        z       = maxZ;
        changed = true;
    }

    //Only write when outside, so we never touch y for objects that are inside(外に出た時だけ書く。中にいる物のyは絶対に触らない)
    if(changed) {
        pos.x = x;
        pos.z = z;
        owner->SetTranslate(pos);
    }
}

void ComponentArenaBounds::Draw()
{
    Super::Draw();

    if(!drawBounds_)
        return;

    //Draw the 4 walls as lines on the floor and at head height(4つの壁を床の高さと頭の高さに線で描く)
    const float y0 = SC::ARENA_FLOOR_Y;
    const float y1 = SC::ARENA_FLOOR_Y + 30.0f;
    const u32   c  = GetColor(255, 0, 0);

    for(float y : {y0, y1}) {
        DrawLine3D(VGet(SC::ARENA_MIN_X, y, SC::ARENA_MIN_Z), VGet(SC::ARENA_MAX_X, y, SC::ARENA_MIN_Z), c);
        DrawLine3D(VGet(SC::ARENA_MAX_X, y, SC::ARENA_MIN_Z), VGet(SC::ARENA_MAX_X, y, SC::ARENA_MAX_Z), c);
        DrawLine3D(VGet(SC::ARENA_MAX_X, y, SC::ARENA_MAX_Z), VGet(SC::ARENA_MIN_X, y, SC::ARENA_MAX_Z), c);
        DrawLine3D(VGet(SC::ARENA_MIN_X, y, SC::ARENA_MAX_Z), VGet(SC::ARENA_MIN_X, y, SC::ARENA_MIN_Z), c);
    }
}

void ComponentArenaBounds::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("ArenaBounds")) {
            //Enable / disable(有効/無効)
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Checkbox(u8"範囲を表示", &drawBounds_);
            ImGui::DragFloat(u8"壁からの距離", &margin_, 0.1f, 0.0f, 50.0f);
            ImGui::Text("X: %.1f ~ %.1f  Z: %.1f ~ %.1f", SC::ARENA_MIN_X, SC::ARENA_MAX_X, SC::ARENA_MIN_Z, SC::ARENA_MAX_Z);
            ImGui::Text(u8"広さはStageConfig.hで変える");

            //Remove this component from the owner in the GUI(GUI上でオーナーからこのコンポーネントを削除する)
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentArenaBounds)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentArenaBounds)
