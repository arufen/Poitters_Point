#include "Game/Component/ComponentSpawnPop.h"

namespace {
//Never use 0. A 0 scale breaks the matrix math when the engine reads rotation/scale back
//(0は使わない。スケール0だとエンジンが回転・スケールを読み戻す時に行列の計算が壊れる)
const float MIN_SCALE = 0.01f;
}    // namespace

void ComponentSpawnPop::Init()
{
    //Do not reset timer_ here. Start() may be called before Init()(ここでtimer_をリセットしない。Start()はInit()より先に呼ばれることがある)
    Super::Init();
}

ComponentSpawnPopPtr ComponentSpawnPop::Start(float targetScale, float duration)
{
    targetScale_ = targetScale;
    duration_    = (duration > 0.01f) ? duration : 0.01f;
    timer_       = 0.0f;
    finished_    = false;

    float3 tiny = {MIN_SCALE, MIN_SCALE, MIN_SCALE};
    GetOwner()->SetScaleAxisXYZ(tiny);

    return std::dynamic_pointer_cast<ComponentSpawnPop>(shared_from_this());
}

void ComponentSpawnPop::Update()
{
    Super::Update();

    if(finished_)
        return;

    timer_  += GetDeltaTime();
    float t  = timer_ / duration_;
    if(t > 1.0f)
        t = 1.0f;

    //Ease-out-back: goes a little bigger than 1, then settles. Looks like a "pop"
    //(イーズアウトバック：1より少し大きくなってから戻る。「ポン」と出る感じ)
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float       u  = t - 1.0f;
    float       k  = 1.0f + c3 * u * u * u + c1 * u * u;

    float s = targetScale_ * k;
    if(s < MIN_SCALE)
        s = MIN_SCALE;
    float3 size = {s, s, s};
    GetOwner()->SetScaleAxisXYZ(size);

    if(t >= 1.0f) {
        //Always end at exactly the real size, so pickup reads the right size(必ず本来の大きさで終わる。拾う処理が正しい大きさを読めるように)
        float3 finalSize = {targetScale_, targetScale_, targetScale_};
        GetOwner()->SetScaleAxisXYZ(finalSize);
        finished_ = true;
        GetOwner()->RemoveComponent(shared_from_this());
    }
}

void ComponentSpawnPop::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("SpawnPop")) {
            ImGui::Text("%.2f / %.2f", timer_, duration_);

            //Remove this component from the owner in the GUI(GUI上でオーナーからこのコンポーネントを削除する)
            if(ImGui::Button(u8"削除"))
                GetOwner()->RemoveComponent(shared_from_this());

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentSpawnPop)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentSpawnPop)
