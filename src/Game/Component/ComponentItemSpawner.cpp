#include "ComponentItemSpawner.h"
#include <Game/Item.h>

#include <System/Object.h>
#include <System/Scene.h>
#include <System/Component/ComponentTransform.h>
#include <imgui.h>
#include <random>

namespace PoittersPoint {

void ComponentItemSpawner::SpawnItemAtRandomPosition()
{
    static std::random_device rd;
    static std::mt19937       gen(rd());

    // 1. Roll for overall spawn rate / probability
    std::uniform_real_distribution<float> rate_dist(0.0f, 1.0f);
    float                                 roll = rate_dist(gen);

    if(roll > spawn_chance_) {
        return;    // Failed spawn rate check
    }

    // 2. Calculate random position within the min/max boundary
    std::uniform_real_distribution<float> dist_x(spawn_area_min_.x, spawn_area_max_.x);
    std::uniform_real_distribution<float> dist_y(spawn_area_min_.y, spawn_area_max_.y);
    std::uniform_real_distribution<float> dist_z(spawn_area_min_.z, spawn_area_max_.z);

    float3 random_pos = {dist_x(gen), dist_y(gen), dist_z(gen)};

    // 3. Create single item at the randomized position
    auto item = Scene::Object::Create<PoittersPoint::Item>();
    if(item) {
        item->SetTranslate(random_pos);
    }
}

void ComponentItemSpawner::GUI()
{
    __super::GUI();

    if(ImGui::TreeNode(u8"Random Item Spawner")) {
        ImGui::SliderFloat(u8"Spawn Chance Rate", &spawn_chance_, 0.0f, 1.0f, "%.2f");

        // Convert float3 to temporary float arrays for ImGui
        float min_pos[3] = {spawn_area_min_.x, spawn_area_min_.y, spawn_area_min_.z};
        if(ImGui::DragFloat3(u8"Area Min Pos", min_pos, 1.0f)) {
            spawn_area_min_ = float3(min_pos[0], min_pos[1], min_pos[2]);
        }

        float max_pos[3] = {spawn_area_max_.x, spawn_area_max_.y, spawn_area_max_.z};
        if(ImGui::DragFloat3(u8"Area Max Pos", max_pos, 1.0f)) {
            spawn_area_max_ = float3(max_pos[0], max_pos[1], max_pos[2]);
        }

        if(ImGui::Button(u8"Test Random Spawn")) {
            SpawnItemAtRandomPosition();
        }

        ImGui::TreePop();
    }
}

}    // namespace PoittersPoint
