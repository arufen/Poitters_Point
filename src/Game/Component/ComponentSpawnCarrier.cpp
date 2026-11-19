#include "Game/Component/ComponentSpawnCarrier.h"
#include "Game/Component/ComponentSpawnPop.h"
#include "Game/ItemBox.h"
#include <System/Component/ComponentModel.h>
#include <cmath>

namespace SC = PoittersPoint::StageConfig;

namespace {
//Own name, because DxLib already has a "PI" and the name clashes(DxLibにもう「PI」があって名前がぶつかるので、自分専用の名前にする)
const float CARRIER_PI = 3.14159265f;

//Smooth start and smooth stop, t is 0..1(ゆっくり始まってゆっくり止まる。tは0〜1)
float SmoothStep(float t)
{
    if(t < 0.0f)
        t = 0.0f;
    if(t > 1.0f)
        t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

float3 Lerp(const float3& a, const float3& b, float t)
{
    return a + (b - a) * t;
}
}    // namespace

ComponentSpawnCarrierPtr ComponentSpawnCarrier::Spawn(SC::ItemType type, const float3& target)
{
    auto obj = Scene::Object::Create<::Object>("SpawnCarrier");
    if(!obj)
        return nullptr;

    //Flat board look with the placeholder model(仮のモデルで平たい板の見た目にする)
    auto   model = obj->AddComponent<ComponentModel>(SC::CARRIER_MODEL);
    float3 scale = {SC::CARRIER_SCALE_XZ * SC::MODEL_BASE_SCALE, SC::CARRIER_SCALE_Y * SC::MODEL_BASE_SCALE, SC::CARRIER_SCALE_XZ * SC::MODEL_BASE_SCALE};
    model->SetScaleAxisXYZ(scale);

    auto carrier     = obj->AddComponent<ComponentSpawnCarrier>();
    carrier->type_   = type;
    carrier->target_ = target;

    //Hover point: right above the target(浮く場所：目標の真上)
    carrier->hover_   = target;
    carrier->hover_.y = SC::ARENA_FLOOR_Y + SC::CARRIER_HEIGHT;

    //Come from a random direction, from outside the arena, and leave to the other side
    //(ランダムな方向のアリーナの外から来て、反対側へ去る)
    float halfX = (SC::ARENA_MAX_X - SC::ARENA_MIN_X) * 0.5f;
    float halfZ = (SC::ARENA_MAX_Z - SC::ARENA_MIN_Z) * 0.5f;
    //(Not named "far": Windows headers have a macro with that name)(「far」という名前はWindowsのマクロとぶつかるので使わない)
    float  farDist = (halfX > halfZ ? halfX : halfZ) + SC::CARRIER_START_OUT;
    float  angle   = (float)GetRand(359) * (CARRIER_PI / 180.0f);
    float3 dir     = {cosf(angle), 0.0f, sinf(angle)};

    carrier->start_    = carrier->hover_ + dir * farDist;
    carrier->start_.y += 30.0f;
    carrier->leave_    = carrier->hover_ - dir * farDist;
    carrier->leave_.y += 60.0f;

    obj->SetTranslate(carrier->start_);
    return carrier;
}

void ComponentSpawnCarrier::Init()
{
    //Do not reset the state here. Spawn() sets it before Init() may run(ここで状態をリセットしない。Init()より先にSpawn()が設定する)
    Super::Init();
}

void ComponentSpawnCarrier::Update()
{
    Super::Update();

    if(state_ == State::Done)
        return;

    auto owner  = GetOwner();
    timer_     += GetDeltaTime();

    switch(state_) {
    case State::FlyIn:
        {
            float t = timer_ / SC::CARRIER_FLY_TIME;
            owner->SetTranslate(Lerp(start_, hover_, SmoothStep(t)));
            spin_ += 90.0f * GetDeltaTime();    //Slow turn while flying(飛んでいる間はゆっくり回る)
            if(t >= 1.0f) {
                state_ = State::SpawnAnim;
                timer_ = 0.0f;
            }
            break;
        }

    case State::SpawnAnim:
        {
            //Spin fast and bob up and down(速く回って上下にゆれる)
            float  t    = timer_ / SC::CARRIER_ANIM_TIME;
            float3 pos  = hover_;
            pos.y      += sinf(t * CARRIER_PI * 4.0f) * 3.0f;
            owner->SetTranslate(pos);
            spin_ += SC::CARRIER_SPIN_SPEED * GetDeltaTime();
            if(t >= 1.0f) {
                DropItem();
                state_ = State::Leave;
                timer_ = 0.0f;
            }
            break;
        }

    case State::Leave:
        {
            float t = timer_ / SC::CARRIER_LEAVE_TIME;
            owner->SetTranslate(Lerp(hover_, leave_, SmoothStep(t)));
            spin_ += 90.0f * GetDeltaTime();
            if(t >= 1.0f) {
                //Release only marks it, the scene deletes it at the end of the frame(Releaseは印をつけるだけで、実際の削除はフレームの最後)
                state_ = State::Done;
                Scene::Object::Release(GetOwnerPtr());
                return;
            }
            break;
        }

    default:
        break;
    }

    if(spin_ >= 360.0f)
        spin_ -= 360.0f;
    owner->SetRotationAxisXYZ(float3(0.0f, spin_, 0.0f));
}

void ComponentSpawnCarrier::DropItem()
{
    float3 pos  = GetOwner()->GetTranslate();
    pos.y      -= SC::CARRIER_DROP_BELOW;

    auto item = PoittersPoint::SpawnThrowable(type_, pos);
    if(!item)
        return;

    //Pop in from tiny to full size(とても小さい所から本来の大きさへポンと出る)
    item->AddComponent<ComponentSpawnPop>()->Start(item->GetSize(), SC::SPAWN_POP_TIME, SC::SPAWN_POP_EASE);

    spawnedItem_ = item;
}

void ComponentSpawnCarrier::Draw()
{
    Super::Draw();

    //Ground marker only before the drop(マーカーは落とす前だけ)
    if(state_ != State::FlyIn && state_ != State::SpawnAnim)
        return;

    //Pulsing circle on the ground, made of short lines(短い線でできた、地面の上で脈打つ円)
    const int SEGMENTS = 24;
    float     pulse    = 0.85f + 0.15f * sinf(timer_ * 10.0f);
    float     r        = SC::MARKER_RADIUS * pulse;
    float     y        = SC::ARENA_FLOOR_Y + 0.5f;
    const u32 color    = GetColor(255, 220, 60);

    for(int i = 0; i < SEGMENTS; ++i) {
        float  a0 = (2.0f * CARRIER_PI) * ((float)i / SEGMENTS);
        float  a1 = (2.0f * CARRIER_PI) * ((float)(i + 1) / SEGMENTS);
        VECTOR p0 = VGet((float)target_.x + cosf(a0) * r, y, (float)target_.z + sinf(a0) * r);
        VECTOR p1 = VGet((float)target_.x + cosf(a1) * r, y, (float)target_.z + sinf(a1) * r);
        DrawLine3D(p0, p1, color);
    }
    //Cross in the middle(真ん中に十字)
    DrawLine3D(VGet((float)target_.x - r * 0.4f, y, (float)target_.z), VGet((float)target_.x + r * 0.4f, y, (float)target_.z), color);
    DrawLine3D(VGet((float)target_.x, y, (float)target_.z - r * 0.4f), VGet((float)target_.x, y, (float)target_.z + r * 0.4f), color);
}

void ComponentSpawnCarrier::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("SpawnCarrier")) {
            static const char* STATE_NAMES[] = {"FlyIn", "SpawnAnim", "Leave", "Done"};
            ImGui::Text("Item : %s", SC::GetItemDef(type_).name);
            ImGui::Text("State: %s  (%.2f)", STATE_NAMES[(int)state_], timer_);
            ImGui::Text("Target: %.1f, %.1f", (float)target_.x, (float)target_.z);

            //Remove this component from the owner in the GUI(GUI上でオーナーからこのコンポーネントを削除する)
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentSpawnCarrier)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentSpawnCarrier)
