#include "Game/Component/ComponentCharacterStats.h"

void ComponentCharacterStats::Init()
{
    Super::Init();
}

ComponentCharacterStatsPtr ComponentCharacterStats::SetMaxHP(int maxHp, bool fill)
{
    maxHp_ = maxHp < 1 ? 1 : maxHp;

    //HP can not be more than max HP(HPは最大HPを超えない)
    if(fill || hp_ > maxHp_)
        hp_ = maxHp_;

    return std::dynamic_pointer_cast<ComponentCharacterStats>(shared_from_this());
}

ComponentCharacterStatsPtr ComponentCharacterStats::SetStrength(float strength)
{
    strength_ = strength;
    return std::dynamic_pointer_cast<ComponentCharacterStats>(shared_from_this());
}

ComponentCharacterStatsPtr ComponentCharacterStats::SetSpeed(float speed)
{
    speed_ = speed;
    return std::dynamic_pointer_cast<ComponentCharacterStats>(shared_from_this());
}

bool ComponentCharacterStats::TakeDamage(int damage)
{
    if(IsDead() || damage <= 0)
        return false;

    hp_ -= damage;
    if(hp_ < 0)
        hp_ = 0;

    return true;
}

void ComponentCharacterStats::Heal(int amount)
{
    //Dead can not be healed(死んでいたら回復しない)
    if(IsDead() || amount <= 0)
        return;

    hp_ += amount;
    if(hp_ > maxHp_)
        hp_ = maxHp_;
}

float ComponentCharacterStats::GetLiftTime(float itemWeight) const
{
    //Never divide by a tiny number(小さい数で割らないようにする)
    float strength = strength_ < 0.1f ? 0.1f : strength_;

    //Heavy items grow fast (weight x weight), strong characters are faster(重い物は急に時間が増える。筋力が高いと早い)
    float time = itemWeight * itemWeight * liftTimeScale_ / strength;

    if(time > maxLiftTime_)
        time = maxLiftTime_;

    //Very short = instant, so light items do not feel laggy(とても短い時間は即時にして、軽い物でもたつかないようにする)
    if(time < 0.15f)
        time = 0.0f;

    return time;
}

void ComponentCharacterStats::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("CharacterStats")) {
            //Enable / disable(有効/無効)
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text("HP : %d / %d", hp_, maxHp_);
            ImGui::DragInt("HP", &hp_, 1, 0, maxHp_);
            ImGui::DragInt("Max HP", &maxHp_, 1, 1, 999);
            ImGui::DragFloat(u8"筋力", &strength_, 0.05f, 0.1f, 5.0f);
            ImGui::DragFloat(u8"速度", &speed_, 0.05f, 0.0f, 10.0f);
            ImGui::DragFloat(u8"持ち上げ時間の倍率", &liftTimeScale_, 0.01f, 0.0f, 2.0f);
            ImGui::DragFloat(u8"持ち上げ最大時間", &maxLiftTime_, 0.1f, 0.0f, 10.0f);

            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentCharacterStats)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentCharacterStats)
