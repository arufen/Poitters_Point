//---------------------------------------------------------------------------
//!	@file	ItemBox.cpp
//! @brief	ItemBox (a throwable chest that explodes on landing and scatters items)
//---------------------------------------------------------------------------

#include "ItemBox.h"
#include "Component/ComponentItemBox.h"
#include <System/Component/ComponentModel.h>
#include <string>

namespace {
//Chest model, same folder as the apple(宝箱のモデル。リンゴと同じフォルダ)
const char* MODEL_CHEST = "data/Game/Models/Item/Chest.mv1";

//Chest size. 1.0 = normal. Change this if the chest looks too big or too small(宝箱の大きさ。1.0が普通。大きすぎ・小さすぎならここを変える)
const float SCALE_CHEST = 0.1f * 80.0f;
}    // namespace

namespace PoittersPoint {

ItemPtr SpawnItemBox(const float3& pos)
{
    //Make a normal Item first (collision, gravity, ComponentItemPickUp, weight...)(まず普通のItemを作る(当たり判定・重力・拾う機能・重さ...))
    auto box = Scene::Object::Create<Item>();
    if(!box)
        return nullptr;

    //Unique name. The hand attach finds objects by NAME, so it must not be the same as others(固有の名前。手にくっつける処理は名前で探すので、他と同じ名前にしない)
    static int boxCount = 0;
    boxCount++;
    box->SetName("ItemBox_" + std::to_string(boxCount));

    //Swap the rock model for the chest model(岩のモデルを宝箱のモデルに入れ替える)
    box->RemoveComponent<ComponentModel>();
    auto model = box->AddComponent<ComponentModel>(MODEL_CHEST);

    float3 size = {SCALE_CHEST, SCALE_CHEST, SCALE_CHEST};
    model->SetScaleAxisXYZ(size);

    box->SetTranslate(pos);

    if(auto col = box->GetComponent<ComponentCollisionCapsule>()) {
        float3 pos = {0.0f, -2.0f, 0.0f};
        col->SetTranslate(pos);
        col->SetRadius(16.44f);
        col->SetHeight(33.24f);
    }

    //The box behavior: explode when it lands after a throw(ボックスの動き：投げたあと着地したら爆発)
    box->AddComponent<ComponentItemBox>();

    return box;
}

}    // namespace PoittersPoint
