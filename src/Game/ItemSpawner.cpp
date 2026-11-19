//---------------------------------------------------------------------------
//!	@file	ItemSpawner.cpp
//! @brief	Keeps the item pool full: fills the field at the start, and sends a carrier for every item that is missing
//!         (アイテムプールを満タンに保つ：開始時にフィールドを埋めて、なくなったアイテムごとに運び役を送る)
//---------------------------------------------------------------------------

#include "ItemSpawner.h"
#include "ItemBox.h"
#include "Player.h"
#include "Npc.h"
#include "Component/ComponentItemPickUp.h"
#include <cmath>

namespace PoittersPoint {

namespace {
//Random float between a and b(aとbの間のランダムな小数)
float RandRange(float a, float b)
{
    return a + (b - a) * (GetRand(1000) * 0.001f);
}

//Distance on the flat ground, height is ignored(平面だけの距離。高さは無視)
float FlatDistance(const float3& a, const float3& b)
{
    float dx = (float)a.x - (float)b.x;
    float dz = (float)a.z - (float)b.z;
    return sqrtf(dx * dx + dz * dz);
}
}    // namespace

ItemPtr ItemSpawner::GetAliveItem(const ItemWeakPtr& weak)
{
    auto item = weak.lock();
    if(item && item->GetStatus(::Object::StatusBit::Alive))
        return item;
    return nullptr;
}

void ItemSpawner::Init()
{
    slots_.clear();

    for(const auto& entry : StageConfig::ITEM_POOL) {
        for(int i = 0; i < entry.count; ++i) {
            Slot slot;
            slot.type         = entry.type;
            slot.respawnDelay = entry.respawnDelay;

            //Full field at the start: drop straight from the sky, no carrier(開始時は満タン：運び役なしで空から直接落とす)
            float3 pos = FindSpawnPoint();
            pos.y      = StageConfig::START_DROP_HEIGHT;
            slot.item  = SpawnThrowable(entry.type, pos);

            slots_.push_back(slot);
        }
    }
}

void ItemSpawner::Update(float deltaTime)
{
    for(auto& slot : slots_) {
        //1. Item is on the field(アイテムがフィールドにある)
        if(auto item = GetAliveItem(slot.item)) {
            //Safety net: it fell out of the stage. Delete it so it gets respawned(安全ネット：ステージから落ちた。消して出し直す)
            auto pickup = item->GetComponent<ComponentItemPickUp>();
            bool held   = pickup && pickup->IsHeld();
            if(!held && (float)item->GetTranslate().y < StageConfig::KILL_Y) {
                Scene::Object::Release(item);
                slot.item.reset();
            }
            continue;
        }

        //2. A carrier is on the way. When it drops the item, that item belongs to this slot
        //(運び役が向かっている。アイテムを落としたら、そのアイテムがこのスロットの物になる)
        if(auto carrier = slot.carrier.lock()) {
            if(carrier->HasDropped()) {
                slot.item = carrier->GetSpawnedItem();
                slot.carrier.reset();
            }
            continue;
        }

        //3. Nothing: the item is gone (exploded / fell out) or the carrier vanished. Count down, then send a carrier
        //(何もない：アイテムがなくなった(爆発・落下)か運び役が消えた。カウントダウンしてから運び役を送る)
        if(!slot.waiting) {
            slot.waiting = true;
            slot.timer   = slot.respawnDelay;
        }

        slot.timer -= deltaTime;
        if(slot.timer <= 0.0f) {
            slot.waiting = false;
            slot.carrier = ComponentSpawnCarrier::Spawn(slot.type, FindSpawnPoint());
        }
    }
}

float3 ItemSpawner::FindSpawnPoint() const
{
    float minX = StageConfig::ARENA_MIN_X + StageConfig::SPAWN_EDGE_MARGIN;
    float maxX = StageConfig::ARENA_MAX_X - StageConfig::SPAWN_EDGE_MARGIN;
    float minZ = StageConfig::ARENA_MIN_Z + StageConfig::SPAWN_EDGE_MARGIN;
    float maxZ = StageConfig::ARENA_MAX_Z - StageConfig::SPAWN_EDGE_MARGIN;

    auto players = Scene::GetObjectsPtr<Player>();
    auto npcs    = Scene::GetObjectsPtr<Npc>();
    auto items   = Scene::GetObjectsPtr<Item>();

    float3 pos = {0.0f, StageConfig::ARENA_FLOOR_Y, 0.0f};

    //Try some random spots. If none is free, use the last one (better than no item)
    //(ランダムな場所をいくつか試す。空いている所がなければ最後の場所を使う(アイテムが出ないよりまし))
    for(int tryCount = 0; tryCount < StageConfig::SPAWN_TRY_COUNT; ++tryCount) {
        pos.x = RandRange(minX, maxX);
        pos.z = RandRange(minZ, maxZ);

        bool ok = true;

        for(auto& p : players)
            if(FlatDistance(pos, p->GetTranslate()) < StageConfig::SPAWN_MIN_DIST_FROM_CHARACTER)
                ok = false;

        for(auto& n : npcs)
            if(FlatDistance(pos, n->GetTranslate()) < StageConfig::SPAWN_MIN_DIST_FROM_CHARACTER)
                ok = false;

        for(auto& it : items)
            if(FlatDistance(pos, it->GetTranslate()) < StageConfig::SPAWN_MIN_DIST_FROM_ITEM)
                ok = false;

        //Two carriers should not aim at the same spot(2つの運び役が同じ場所を狙わないように)
        for(auto& slot : slots_)
            if(auto carrier = slot.carrier.lock())
                if(FlatDistance(pos, carrier->GetTarget()) < StageConfig::SPAWN_MIN_DIST_FROM_ITEM)
                    ok = false;

        if(ok)
            break;
    }

    return pos;
}

void ItemSpawner::RespawnAll()
{
    for(auto& slot : slots_) {
        if(auto item = GetAliveItem(slot.item)) {
            auto pickup = item->GetComponent<ComponentItemPickUp>();
            if(pickup && pickup->IsHeld())
                continue;    //Do not delete what someone is holding(持たれている物は消さない)
            Scene::Object::Release(item);
            slot.item.reset();
        }
    }
}

void ItemSpawner::GUI()
{
    ImGui::Begin("ItemSpawner");
    {
        if(ImGui::Button(u8"全部出し直す"))
            RespawnAll();

        ImGui::Separator();

        for(size_t i = 0; i < slots_.size(); ++i) {
            const auto& slot = slots_[i];
            const char* name = StageConfig::GetItemDef(slot.type).name;

            if(auto item = GetAliveItem(slot.item))
                ImGui::Text("%2d %-12s  %s", (int)i, name, item->GetName().data());
            else if(slot.carrier.lock())
                ImGui::Text("%2d %-12s  carrier on the way", (int)i, name);
            else
                ImGui::Text("%2d %-12s  respawn in %.1fs", (int)i, name, slot.timer);
        }
    }
    ImGui::End();
}

}    // namespace PoittersPoint
