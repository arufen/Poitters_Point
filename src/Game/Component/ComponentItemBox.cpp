#include "Game/Component/ComponentItemBox.h"
#include "Game/Component/ComponentItemPickUp.h"
#include "Game/Component/ComponentScatterFly.h"
#include "Game/Component/ComponentExplosionFx.h"
#include "Game/Item.h"
#include "Game/ConsumableItem.h"
#include <cmath>

namespace {
//Random float between a and b(aとbの間のランダムな小数)
float RandRange(float a, float b)
{
    return a + (b - a) * (GetRand(1000) * 0.001f);
}

//Own name, because DxLib already has a "PI" and the name clashes(DxLibにもう「PI」があって名前がぶつかるので、自分専用の名前にする)
const float ITEMBOX_PI = 3.14159265f;
}    // namespace

void ComponentItemBox::Init()
{
    Super::Init();
    wasThrown_  = false;
    exploded_   = false;
    thrownTime_ = 0.0f;
}

ComponentItemBoxPtr ComponentItemBox::SetContentCount(int minCount, int maxCount)
{
    minCount_ = minCount;
    maxCount_ = (maxCount < minCount) ? minCount : maxCount;
    return std::dynamic_pointer_cast<ComponentItemBox>(shared_from_this());
}

ComponentItemBoxPtr ComponentItemBox::SetConsumableChance(int percent)
{
    consumableChance_ = percent;
    return std::dynamic_pointer_cast<ComponentItemBox>(shared_from_this());
}

void ComponentItemBox::Update()
{
    Super::Update();

    if(exploded_)
        return;

    auto pickup = GetOwner()->GetComponent<ComponentItemPickUp>();
    if(!pickup)
        return;

    bool thrown = pickup->IsThrown();

    if(thrown) {
        //Safety timer while flying(飛んでいる間の安全タイマー)
        thrownTime_ += GetDeltaTime();
        if(thrownTime_ >= maxFlyTime_) {
            Explode();
            return;
        }
    }
    else {
        thrownTime_ = 0.0f;

        //It was flying last frame and now it is not, and nobody caught it = it landed after a throw -> BOOM
        //Item::OnHit + ComponentItemPickUp already handle the landing, we only watch the change
        //(前のフレームは飛んでいて今は飛んでいない、しかも誰も持っていない＝投げたあと着地した→爆発
        // 着地の処理はItem::OnHitとComponentItemPickUpがやっているので、ここは変化を見るだけ)
        if(wasThrown_ && !pickup->IsHeld()) {
            Explode();
            return;
        }
    }

    wasThrown_ = thrown;
}

void ComponentItemBox::Explode()
{
    if(exploded_)
        return;
    exploded_ = true;

    auto   ownerPtr = GetOwnerPtr();
    float3 center   = ownerPtr->GetTranslate();

    //How many items. Spread them evenly around a circle, plus a little random, so they do not stack
    //(アイテムの数。円のまわりに均等に並べて少しランダムにする。重ならないように)
    int count = minCount_ + GetRand(maxCount_ - minCount_);
    for(int i = 0; i < count; ++i) {
        float angle = (2.0f * ITEMBOX_PI) * ((float)i / (float)count) + RandRange(-0.3f, 0.3f);
        SpawnOneItem(center, angle);
    }

    //Blast effect as its own small object, so the box can be deleted right now(爆発エフェクトは別の小さいオブジェクト。だから箱はすぐに消せる)
    auto fx = Scene::Object::Create<::Object>("Explosion");
    if(fx) {
        fx->SetTranslate(center);
        fx->AddComponent<ComponentExplosionFx>();
    }

    //Delete the box. Release only marks it, the scene deletes it at the end of the frame
    //(箱を消す。Releaseは印をつけるだけで、実際の削除はフレームの最後)
    Scene::Object::Release(ownerPtr);
}

void ComponentItemBox::SpawnOneItem(const float3& center, float angle)
{
    float3 dir = {cosf(angle), 0.0f, sinf(angle)};

    //Start a little above and away from the center, so items do not push each other at spawn
    //(中心から少し上・少し外から出す。出た瞬間にアイテム同士が押し合わないように)
    float3 startPos  = center + dir * 8.0f;
    startPos.y      += 15.0f;

    std::shared_ptr<::Object> item;
    if(GetRand(99) < consumableChance_)
        item = Scene::Object::Create<PoittersPoint::ConsumableItem>();    //Apple / golden apple / speed potion(リンゴ・金のリンゴ・スピードポーション)
    else
        item = Scene::Object::Create<PoittersPoint::Item>();    //Throwable rock(投げるアイテム)

    if(!item)
        return;

    item->SetTranslate(startPos);

    float3 vel = dir * RandRange(flySpeedMin_, flySpeedMax_);
    vel.y      = RandRange(flyUpMin_, flyUpMax_);
    item->AddComponent<ComponentScatterFly>()->SetVelocity(vel);
}

void ComponentItemBox::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("ItemBox")) {
            //Enable / disable(有効/無効)
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text(wasThrown_ ? "Flying" : "Waiting");

            //Test button: explode now(テスト用：今すぐ爆発)
            if(!exploded_ && ImGui::Button(u8"今すぐ爆発"))
                Explode();

            ImGui::DragFloat(u8"最大飛行秒数", &maxFlyTime_, 0.1f, 0.5f, 20.0f);
            ImGui::DragInt(u8"中身の最小数", &minCount_, 1, 0, 20);
            ImGui::DragInt(u8"中身の最大数", &maxCount_, 1, 0, 20);
            if(maxCount_ < minCount_)
                maxCount_ = minCount_;
            ImGui::DragInt(u8"消費アイテム確率%", &consumableChance_, 1, 0, 100);
            ImGui::DragFloat(u8"横の速さ 最小", &flySpeedMin_, 0.5f);
            ImGui::DragFloat(u8"横の速さ 最大", &flySpeedMax_, 0.5f);
            ImGui::DragFloat(u8"上の速さ 最小", &flyUpMin_, 0.5f);
            ImGui::DragFloat(u8"上の速さ 最大", &flyUpMax_, 0.5f);

            //Remove this component from the owner in the GUI(GUI上でオーナーからこのコンポーネントを削除する)
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentItemBox)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentItemBox)
