#include "Game/Component/ComponentConsumableItem.h"
#include "Game/Component/ComponentCharacterStats.h"
#include <cmath>

namespace {
//Model files. If your folder is different, change the paths here(モデルファイル。フォルダが違う場合はここのパスを変える)
const char* MODEL_APPLE        = "data/Game/Models/Item/RedApple.mv1";
const char* MODEL_GOLDEN_APPLE = "data/Game/Models/Item/GoldenApple.mv1";
const char* MODEL_SPEED_POTION = "data/Game/Models/Item/SpeedPotion.mv1";

//Model SIZE for each type. 1.0 = normal. Bigger number = bigger model. Change ONLY the one you want
//(モデルの大きさ。1.0が普通。数字が大きいほど大きい。直したいモデルの数字だけ変える)
const float SCALE_APPLE        = 0.1f / 2.0f;
const float SCALE_GOLDEN_APPLE = 0.1f / 2.0f;
const float SCALE_SPEED_POTION = 0.1f * 4.0f;
}    // namespace

void ComponentConsumableItem::Init()
{
    //Do not reset type_ here. SetItemType() is called before Init()(ここでtype_をリセットしない。SetItemType()はInit()より先に呼ばれる)
    Super::Init();
}

ComponentConsumableItemPtr ComponentConsumableItem::SetItemType(ItemType type)
{
    type_ = (int)type;
    return std::dynamic_pointer_cast<ComponentConsumableItem>(shared_from_this());
}

const char* ComponentConsumableItem::GetModelPath(ItemType type)
{
    switch(type) {
    case ItemType::Apple:
        return MODEL_APPLE;
    case ItemType::GoldenApple:
        return MODEL_GOLDEN_APPLE;
    case ItemType::SpeedPotion:
        return MODEL_SPEED_POTION;
    }
    return MODEL_APPLE;
}

float ComponentConsumableItem::GetModelScale(ItemType type)
{
    switch(type) {
    case ItemType::Apple:
        return SCALE_APPLE;
    case ItemType::GoldenApple:
        return SCALE_GOLDEN_APPLE;
    case ItemType::SpeedPotion:
        return SCALE_SPEED_POTION;
    }
    return 1.0f;
}

const char* ComponentConsumableItem::GetItemName(ItemType type)
{
    switch(type) {
    case ItemType::Apple:
        return "Apple";
    case ItemType::GoldenApple:
        return "GoldenApple";
    case ItemType::SpeedPotion:
        return "SpeedPotion";
    }
    return "Apple";
}

bool ComponentConsumableItem::ApplyEffect(ComponentCharacterStats& stats)
{
    switch(GetItemType()) {
    case ItemType::Apple:
    case ItemType::GoldenApple:
        {
            //Full HP: healing would be wasted(HP満タンだと回復がもったいない)
            if(skipHealWhenFull_ && stats.GetHP() >= stats.GetMaxHP())
                return false;

            stats.Heal(GetItemType() == ItemType::Apple ? healApple_ : healGolden_);
            return true;
        }

    case ItemType::SpeedPotion:
        //Taking another potion while it is active just refreshes the timer, it does not stack(効果中にもう1本取ると時間が戻るだけで、重ねがけはしない)
        stats.ApplySpeedBuff(speedMultiplier_, speedDuration_);
        return true;
    }

    return false;
}

void ComponentConsumableItem::Update()
{
    Super::Update();

    if(collected_)
        return;

    auto   ownerPtr = GetOwnerPtr();
    float3 myPos    = ownerPtr->GetTranslate();

    //Look at every object in the scene. Anyone with ComponentCharacterStats can take it, so Player and NPCs work the same
    //(シーンの全オブジェクトを見る。ComponentCharacterStatsを持っていれば誰でも取れるので、プレイヤーもNPCも同じ)
    for(auto& obj : Scene::GetCurrentScene()->GetObjectPtrVec()) {
        if(obj.get() == ownerPtr.get())
            continue;

        if(!obj->GetStatus(::Object::StatusBit::Alive))
            continue;

        auto stats = obj->GetComponent<ComponentCharacterStats>();
        if(!stats || stats->IsDead())
            continue;    //Not a character, or already dead(キャラではない、またはすでに死んでいる)

        //Distance check: height difference first, then flat distance(距離チェック：先に高さの差、次に平面の距離)
        float3 diff = obj->GetTranslate() - myPos;
        float  dy   = (float)diff.y;
        if(fabsf(dy) > pickupHeight_)
            continue;

        diff.y = 0.0f;    //Flat distance only(平面の距離だけ見る)
        if((float)length(diff) > pickupRange_)
            continue;

        if(!ApplyEffect(*stats))
            continue;

        //Taken! Remove this item from the scene. Release only marks it, the scene deletes it at the end of the frame
        //(取られた！シーンからこのアイテムを消す。Releaseは印をつけるだけで、実際の削除はフレームの最後)
        collected_ = true;
        Scene::Object::Release(ownerPtr);
        return;
    }
}

void ComponentConsumableItem::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("ConsumableItem")) {
            //Enable / disable(有効/無効)
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            //Changing the type here only changes the effect. The model stays the same(ここで種類を変えるのは効果だけ。モデルは変わらない)
            ImGui::Combo(u8"種類", &type_, "Apple\0GoldenApple\0SpeedPotion\0");

            ImGui::DragInt(u8"リンゴの回復量", &healApple_, 1, 0, 99);
            ImGui::DragInt(u8"金のリンゴの回復量", &healGolden_, 1, 0, 99);
            ImGui::DragFloat(u8"スピード倍率", &speedMultiplier_, 0.01f, 1.0f, 3.0f);
            ImGui::DragFloat(u8"スピード効果の秒数", &speedDuration_, 0.1f, 0.0f, 60.0f);
            ImGui::DragFloat(u8"取れる距離", &pickupRange_, 0.5f, 0.0f, 100.0f);
            ImGui::DragFloat(u8"取れる高さ", &pickupHeight_, 0.5f, 0.0f, 100.0f);
            ImGui::Checkbox(u8"満タンの時は回復アイテムを取らない", &skipHealWhenFull_);

            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentConsumableItem)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentConsumableItem)
