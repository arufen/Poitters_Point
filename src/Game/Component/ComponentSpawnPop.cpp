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

ComponentSpawnPopPtr ComponentSpawnPop::Start(float targetScale, float duration, EaseType ease)
{
    targetScale_ = targetScale;
    duration_    = (duration > 0.01f) ? duration : 0.01f;
    ease_        = ease;
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

    //Engine ease curve: 0 at the start, 1 at the end. OutBack goes a little over 1 then settles, which looks like a "pop"
    //(エンジンのイージングカーブ：最初は0、最後は1。OutBackは1を少し超えてから戻るので「ポン」と出る感じになる)
    float k = GetEaseFunction(ease_)(t);

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
