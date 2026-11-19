#include "Game/Component/ComponentGroundShadow.h"
#include "Game/StageConfig.h"
#include "Game/Ground.h"
#include <System/Component/ComponentModel.h>
#include <cmath>

namespace SC = PoittersPoint::StageConfig;

namespace {
//Own name, because DxLib already has a "PI" and the name clashes(DxLibにもう「PI」があって名前がぶつかるので、自分専用の名前にする)
const float SHADOW_PI = 3.14159265f;
}    // namespace

void ComponentGroundShadow::Init()
{
    Super::Init();
    alpha_ = SC::SHADOW_ALPHA;
}

ComponentGroundShadowPtr ComponentGroundShadow::SetRadius(float radius)
{
    radius_ = radius;
    return std::dynamic_pointer_cast<ComponentGroundShadow>(shared_from_this());
}

bool ComponentGroundShadow::CastToGround()
{
    //Find the Ground once and remember it(Groundを1回だけ探して覚える)
    auto ground = ground_.lock();
    if(!ground) {
        auto grounds = Scene::GetObjectsPtr<PoittersPoint::Ground>();
        if(grounds.empty())
            return false;
        ground  = grounds.front();
        ground_ = ground;
    }

    auto model = ground->GetComponent<ComponentModel>();
    if(!model)
        return false;

    //Line from a bit above the owner straight down. Starting above means it still works if the owner sinks into the floor a little
    //(オーナーの少し上から真下への線。上から始めるので、オーナーが床に少しめり込んでいても動く)
    float3 pos    = GetOwner()->GetTranslate();
    float3 top    = pos + float3(0.0f, SC::SHADOW_RAY_START_UP, 0.0f);
    float3 bottom = pos - float3(0.0f, SC::SHADOW_RAY_LENGTH, 0.0f);

    //Same check the engine's ComponentCollisionModel uses on the Ground model(エンジンのComponentCollisionModelがGroundモデルに使うのと同じチェック)
    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(model->GetModel(), -1, cast(top), cast(bottom));
    if(!hit.HitFlag)
        return false;

    groundPos_    = float3(hit.HitPosition.x, hit.HitPosition.y, hit.HitPosition.z);
    groundNormal_ = normalize(float3(hit.Normal.x, hit.Normal.y, hit.Normal.z));

    //A floor seen from below would have a downward normal. Flip it so the circle faces up(下から見た床は法線が下向きになる。円が上を向くように反転する)
    if((float)groundNormal_.y < 0.0f)
        groundNormal_ = -groundNormal_;

    return true;
}

void ComponentGroundShadow::Update()
{
    Super::Update();
    hasGround_ = CastToGround();
}

void ComponentGroundShadow::Draw()
{
    Super::Draw();

    if(!hasGround_)
        return;    //Off the stage: no floor, no shadow(ステージの外：床がないので影もない)

    //Two arrows lying ON the floor (tangent and bitangent), so the circle lies flat on ramps too
    //(床の上に寝ている2本の矢印(接線と従接線)。坂でも円が床にぴったり寝るように)
    float3 n       = groundNormal_;
    float3 helper  = (fabsf((float)n.x) < 0.9f) ? float3(1.0f, 0.0f, 0.0f) : float3(0.0f, 0.0f, 1.0f);
    float3 tangent = normalize(cross(helper, n));
    float3 bitan   = cross(n, tangent);

    //Lift a little along the normal so it does not flicker inside the floor (z-fighting)(床の中でちらつかないように(Zファイティング)、法線方向に少し浮かせる)
    float3 center = groundPos_ + n * SC::SHADOW_LIFT;
    float  r      = radius_ * sizeScale_;

    //A filled circle = a fan of thin triangles around the center(塗りつぶした円＝中心のまわりの細い三角形の扇)
    const int SEGMENTS = 20;
    const u32 color    = GetColor(0, 0, 0);

    SetUseLighting(FALSE);    //Shadows are not lit(影はライトの影響を受けない)
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha_);

    VECTOR c = cast(center);
    for(int i = 0; i < SEGMENTS; ++i) {
        float  a0 = (2.0f * SHADOW_PI) * ((float)i / SEGMENTS);
        float  a1 = (2.0f * SHADOW_PI) * ((float)(i + 1) / SEGMENTS);
        float3 p0 = center + (tangent * cosf(a0) + bitan * sinf(a0)) * r;
        float3 p1 = center + (tangent * cosf(a1) + bitan * sinf(a1)) * r;
        DrawTriangle3D(c, cast(p1), cast(p0), color, TRUE);
    }

    //Always put the draw settings back(描画設定は必ず元に戻す)
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseLighting(TRUE);
}

void ComponentGroundShadow::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("GroundShadow")) {
            //Enable / disable(有効/無効)
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            ImGui::Text(hasGround_ ? "Floor: %.1f, %.1f, %.1f" : "No floor below", (float)groundPos_.x, (float)groundPos_.y, (float)groundPos_.z);
            ImGui::DragFloat(u8"半径", &radius_, 0.1f, 0.0f, 100.0f);
            ImGui::DragInt(u8"濃さ", &alpha_, 1, 0, 255);

            //Remove this component from the owner in the GUI(GUI上でオーナーからこのコンポーネントを削除する)
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentGroundShadow)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentGroundShadow)
