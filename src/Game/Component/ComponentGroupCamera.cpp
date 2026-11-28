#include "Game/Component/ComponentGroupCamera.h"
#include "Game/Component/ComponentCharacterStats.h"
#include "Game/StageConfig.h"
#include "Game/Player.h"
#include "Game/Npc.h"
#include <System/Component/ComponentCamera.h>
#include <algorithm>
#include <cmath>

namespace SC = PoittersPoint::StageConfig;

namespace {
//Own name, because DxLib already has a "PI"(DxLibにもう「PI」があるので自分専用の名前にする)
const float CAM_DEG2RAD = 3.14159265f / 180.0f;

//The camera never gets closer to a point than this (stops dividing by ~0)(カメラはこれより点に近づかない(0に近い割り算を防ぐ))
const float CAM_MIN_DEPTH = 10.0f;

//Frame-rate independent smoothing. speed = how fast it closes the gap per second(フレームレートに左右されないなめらか補間。speed＝1秒あたりの寄り具合)
float SmoothFactor(float speed, float dt)
{
    return 1.0f - expf(-speed * dt);
}

float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float SignOf(float v)
{
    return v < 0.0f ? -1.0f : 1.0f;
}

float3 ArenaCenter()
{
    return float3((SC::ARENA_MIN_X + SC::ARENA_MAX_X) * 0.5f, SC::ARENA_FLOOR_Y, (SC::ARENA_MIN_Z + SC::ARENA_MAX_Z) * 0.5f);
}
}    // namespace

//---------------------------------------------------------------------------
// Setup(準備)
//---------------------------------------------------------------------------
void ComponentGroupCamera::Init()
{
    Super::Init();
    ResetToOverview();
}

void ComponentGroupCamera::ResetToOverview()
{
    //Far enough to see all 4 corners of the arena. The first Updates zoom in from here like a match intro
    //(アリーナの4つの角が全部見える距離。最初のUpdateからここからズームインして、試合開始の演出になる)
    std::vector<float3> corners = {
        float3(SC::ARENA_MIN_X, SC::ARENA_FLOOR_Y, SC::ARENA_MIN_Z),
        float3(SC::ARENA_MAX_X, SC::ARENA_FLOOR_Y, SC::ARENA_MIN_Z),
        float3(SC::ARENA_MIN_X, SC::ARENA_FLOOR_Y, SC::ARENA_MAX_Z),
        float3(SC::ARENA_MAX_X, SC::ARENA_FLOOR_Y, SC::ARENA_MAX_Z),
    };
    distance_ = (std::max)(FrameTargets(corners, lookAt_, false), minDistance_);
    started_  = true;
}

void ComponentGroupCamera::SnapToTarget()
{
    CalcTarget(lookAt_, distance_);
    ApplyToCamera();
}

//---------------------------------------------------------------------------
// Target calculation(ターゲットの計算)
//---------------------------------------------------------------------------
void ComponentGroupCamera::CollectTargets(std::vector<float3>& out) const
{
    //Two passes: first only the characters still alive. If nobody is alive (draw), frame everyone so the camera does not jump
    //(2回まわす：最初は生きているキャラだけ。誰も生きていない(引き分け)時は、カメラが飛ばないように全員を映す)
    for(int pass = 0; pass < 2; ++pass) {
        bool onlyAlive = (pass == 0) && !includeDead_;
        out.clear();

        auto visit = [&](::Object& obj) {
            if(onlyAlive) {
                if(auto stats = obj.GetComponent<ComponentCharacterStats>()) {
                    if(stats->IsDead())
                        return;
                }
            }
            out.push_back(obj.GetTranslate());
        };

        for(auto& player : Scene::GetObjectsPtr<PoittersPoint::Player>()) visit(*player);
        for(auto& npc : Scene::GetObjectsPtr<PoittersPoint::Npc>()) visit(*npc);

        if(!out.empty() || !onlyAlive)
            return;
    }
}

float ComponentGroupCamera::CalcFitDistance(const std::vector<float3>& targets, const float3& lookAt) const
{
    //The camera sits at lookAt + (0, sin(pitch), -cos(pitch)) * d. For a point at (dx, dy, dz) from lookAt:
    //  depth (along the view)   = d + K,  K = dz*cos(pitch) - dy*sin(pitch)
    //  up    (on the screen)    = dy*cos(pitch) + dz*sin(pitch)
    //  right (on the screen)    = dx
    //The point is on screen when |up| <= limit * tan(fov/2) * depth, so we can solve d for each point directly
    //(カメラは lookAt + (0, sin(pitch), -cos(pitch)) * d にいる。lookAtから(dx, dy, dz)の点は：
    //  奥行き = d + K、 画面の上方向 = dy*cos + dz*sin、 画面の右方向 = dx
    // |上方向| <= 限界 * tan(画角/2) * 奥行き なら画面に入るので、点ごとに d を直接求められる)
    float pitch    = pitch_ * CAM_DEG2RAD;
    float sinP     = sinf(pitch);
    float cosP     = cosf(pitch);
    float tanHalfY = tanf(fov_ * 0.5f * CAM_DEG2RAD);
    float tanHalfX = tanHalfY * ((float)WINDOW_W / (float)WINDOW_H);

    float need = 0.0f;
    for(auto& t : targets) {
        //Feet and head, so tall characters are not cut(足元と頭。背の高いキャラが切れないように)
        for(float h : {0.0f, charHeight_}) {
            float dx = (float)t.x - (float)lookAt.x;
            float dy = (float)t.y + h - (float)lookAt.y;
            float dz = (float)t.z - (float)lookAt.z;

            //Push the point a little away from the middle = margin(点を真ん中から少し外へ押す＝余白)
            dx += SignOf(dx) * margin_;
            dz += SignOf(dz) * margin_;

            float K     = dz * cosP - dy * sinP;
            float up    = dy * cosP + dz * sinP;
            float limit = (up >= 0.0f) ? screenTop_ : screenBottom_;

            need = (std::max)(need, fabsf(up) / (limit * tanHalfY) - K);
            need = (std::max)(need, fabsf(dx) / (screenSide_ * tanHalfX) - K);
            need = (std::max)(need, CAM_MIN_DEPTH - K);
        }
    }
    return need;
}

void ComponentGroupCamera::CalcTarget(float3& lookAt, float& distance) const
{
    std::vector<float3> targets;
    CollectTargets(targets);

    if(targets.empty()) {
        //Nobody in the scene: keep looking where we are(シーンに誰もいない：今の場所を見続ける)
        lookAt   = lookAt_;
        distance = distance_;
        return;
    }

    distance = std::clamp(FrameTargets(targets, lookAt, clampToArena_), minDistance_, maxDistance_);
}

float ComponentGroupCamera::FrameTargets(const std::vector<float3>& targets, float3& lookAt, bool useArenaClamp) const
{
    //Box around everyone(全員を囲む箱)
    float minX = 1.0e9f, maxX = -1.0e9f, minZ = 1.0e9f, maxZ = -1.0e9f, sumY = 0.0f;
    for(auto& t : targets) {
        minX  = (std::min)(minX, (float)t.x);
        maxX  = (std::max)(maxX, (float)t.x);
        minZ  = (std::min)(minZ, (float)t.z);
        maxZ  = (std::max)(maxZ, (float)t.z);
        sumY += (float)t.y;
    }
    float x = (minX + maxX) * 0.5f;
    float y = sumY / (float)targets.size() + lookHeight_;

    //Left/right is the middle of the box. Front/back is NOT: the near side looks bigger (perspective),
    //so we search the depth where the needed distance is smallest. The needed distance is a "bowl" shape, so a ternary search works
    //(左右は箱の真ん中。前後は違う：手前は大きく見える(遠近法)ので、必要な距離が一番小さくなる奥行きを探す。
    // 必要な距離は「おわん型」なので三分探索で見つかる)
    auto  fitAtZ = [&](float z) { return CalcFitDistance(targets, float3(x, y, z)); };
    float lo     = minZ - 150.0f;
    float hi     = maxZ + 150.0f;
    for(int i = 0; i < 40; ++i) {
        float m1 = lo + (hi - lo) / 3.0f;
        float m2 = hi - (hi - lo) / 3.0f;
        if(fitAtZ(m1) < fitAtZ(m2))
            hi = m2;
        else
            lo = m1;
    }
    lookAt       = float3(x, y, (lo + hi) * 0.5f);
    float needed = CalcFitDistance(targets, lookAt);

    //Pull the view toward the arena, but only as far as it does not need to zoom out for it
    //(画面をアリーナの方へ寄せる。ただし、そのためにズームアウトが必要にならない所まで)
    if(useArenaClamp) {
        float3 clamped = lookAt;
        ClampToArena(clamped, (std::max)(needed, minDistance_));

        float allowed = (std::max)(needed, minDistance_) * 1.02f;
        float ok = 0.0f, ng = 1.0f;
        if(CalcFitDistance(targets, clamped) <= allowed)
            ok = 1.0f;
        else {
            for(int i = 0; i < 12; ++i) {
                float  mid = (ok + ng) * 0.5f;
                float3 p   = lookAt + (clamped - lookAt) * mid;
                if(CalcFitDistance(targets, p) <= allowed)
                    ok = mid;
                else
                    ng = mid;
            }
        }
        lookAt = lookAt + (clamped - lookAt) * ok;
        needed = CalcFitDistance(targets, lookAt);
    }

    return needed;
}

void ComponentGroupCamera::ClampToArena(float3& lookAt, float distance) const
{
    //How much ground the view covers from the middle, roughly(画面が真ん中からどれだけの地面を映すか(だいたい))
    float pitch     = pitch_ * CAM_DEG2RAD;
    float tanHalfY  = tanf(fov_ * 0.5f * CAM_DEG2RAD);
    float aspect    = (float)WINDOW_W / (float)WINDOW_H;
    float viewHalfW = distance * tanHalfY * aspect;
    float viewHalfD = distance * tanHalfY / (std::max)(sinf(pitch), 0.1f);

    //The middle may move only as far as the view still stays inside arena + overscan.
    //If the view is bigger than the arena, stay in the middle of the arena
    //(画面がアリーナ＋はみ出し分の中に収まる範囲でだけ真ん中を動かせる。画面がアリーナより大きければアリーナの真ん中にいる)
    float3 c      = ArenaCenter();
    float  limitX = (std::max)(0.0f, (SC::ARENA_MAX_X - SC::ARENA_MIN_X) * 0.5f + overscan_ - viewHalfW);
    float  limitZ = (std::max)(0.0f, (SC::ARENA_MAX_Z - SC::ARENA_MIN_Z) * 0.5f + overscan_ - viewHalfD);

    float x = std::clamp((float)lookAt.x, (float)c.x - limitX, (float)c.x + limitX);
    float z = std::clamp((float)lookAt.z, (float)c.z - limitZ, (float)c.z + limitZ);
    lookAt  = float3(x, (float)lookAt.y, z);
}

//---------------------------------------------------------------------------
// Update(更新)
//---------------------------------------------------------------------------
void ComponentGroupCamera::Update()
{
    Super::Update();

    if(!started_)
        ResetToOverview();

    float3 targetLook;
    float  targetDist;
    CalcTarget(targetLook, targetDist);

    float dt = GetDeltaTime();

    //Look-at point follows smoothly(注視点はなめらかに追いかける)
    float tf = SmoothFactor(followSpeed_, dt);
    lookAt_  = float3(
        Lerp((float)lookAt_.x, (float)targetLook.x, tf), Lerp((float)lookAt_.y, (float)targetLook.y, tf), Lerp((float)lookAt_.z, (float)targetLook.z, tf));

    //Zoom out quickly, zoom in slowly(引く時は速く、寄る時はゆっくり)
    float zoomSpeed = (targetDist > distance_) ? zoomOutSpeed_ : zoomInSpeed_;
    distance_       = Lerp(distance_, targetDist, SmoothFactor(zoomSpeed, dt));

    //For the GUI(GUI表示用)
    lastTarget_ = targetLook;
    lastDist_   = targetDist;

    ApplyToCamera();
}

void ComponentGroupCamera::ApplyToCamera()
{
    auto camera = GetOwner()->GetComponent<ComponentCamera>();
    if(!camera)
        return;

    //The camera sits behind (-Z) and above the look-at point. The angle never turns
    //(カメラは注視点の後ろ(-Z)の上にいる。アングルは回らない)
    float  pitch = pitch_ * CAM_DEG2RAD;
    float3 offset(0.0f, sinf(pitch) * distance_, -cosf(pitch) * distance_);

    camera->SetPerspective(fov_, (float)WINDOW_W / (float)WINDOW_H);
    camera->SetPositionAndTarget(lookAt_ + offset, lookAt_);
}

//---------------------------------------------------------------------------
// GUI
//---------------------------------------------------------------------------
void ComponentGroupCamera::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode(u8"GroupCamera")) {
            //Enable / disable(有効/無効)
            bool enable = GetStatus(StatusBit::Enable);
            if(ImGui::Checkbox(u8"有効", &enable))
                SetStatus(StatusBit::Enable, enable);

            std::vector<float3> targets;
            CollectTargets(targets);
            ImGui::Text(u8"映している人数: %d", (int)targets.size());
            ImGui::Text(u8"距離: %.1f  (目標 %.1f)", distance_, lastDist_);
            ImGui::Text(u8"注視点: %.1f, %.1f, %.1f", (float)lookAt_.x, (float)lookAt_.y, (float)lookAt_.z);

            ImGui::SliderFloat(u8"見下ろす角度", &pitch_, 20.0f, 89.0f);
            ImGui::SliderFloat(u8"画角", &fov_, 20.0f, 90.0f);
            ImGui::DragFloat(u8"最小距離(寄り)", &minDistance_, 1.0f, 30.0f, maxDistance_);
            ImGui::DragFloat(u8"最大距離(引き)", &maxDistance_, 1.0f, minDistance_, 2000.0f);
            ImGui::DragFloat(u8"余白", &margin_, 0.5f, 0.0f, 200.0f);
            ImGui::DragFloat(u8"キャラの高さ", &charHeight_, 0.5f, 0.0f, 200.0f);
            ImGui::SliderFloat(u8"画面の左右の限界", &screenSide_, 0.3f, 1.0f);
            ImGui::SliderFloat(u8"画面の上の限界", &screenTop_, 0.3f, 1.0f);
            ImGui::SliderFloat(u8"画面の下の限界", &screenBottom_, 0.3f, 1.0f);
            ImGui::DragFloat(u8"注視点の高さ", &lookHeight_, 0.5f, -50.0f, 100.0f);
            ImGui::DragFloat(u8"追従の速さ", &followSpeed_, 0.05f, 0.1f, 20.0f);
            ImGui::DragFloat(u8"引く速さ", &zoomOutSpeed_, 0.05f, 0.1f, 20.0f);
            ImGui::DragFloat(u8"寄る速さ", &zoomInSpeed_, 0.05f, 0.1f, 20.0f);
            ImGui::Checkbox(u8"アリーナ外を映しすぎない", &clampToArena_);
            ImGui::DragFloat(u8"はみ出してよい幅", &overscan_, 0.5f, 0.0f, 300.0f);
            ImGui::Checkbox(u8"K.O.されたキャラも映す", &includeDead_);

            if(ImGui::Button(u8"全体表示からやり直す"))
                ResetToOverview();
            ImGui::SameLine();
            if(ImGui::Button(u8"今すぐ合わせる"))
                SnapToTarget();

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentGroupCamera)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentGroupCamera)
