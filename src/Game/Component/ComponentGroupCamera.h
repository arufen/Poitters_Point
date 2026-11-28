#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>
#include <vector>

USING_PTR(ComponentGroupCamera);

//Poitter's Point style group camera. Put it on the same object as ComponentCamera.
//The angle never turns (fixed pitch, looking toward +Z). Every frame it:
//  1. finds the box around every character still alive (Player + NPC)
//  2. smoothly moves the look-at point to the middle of that box
//  3. zooms out when they spread, zooms in when they come together or fewer are left
//The match starts with the whole arena in view and then zooms in, and the last one standing gets a close-up.
//(ポイッターズポイント風のグループカメラ。ComponentCameraと同じオブジェクトにつける。
// アングルは回らない(ピッチ固定、+Z方向を見る)。毎フレーム：
//  1. まだ生きているキャラ全員(プレイヤー＋NPC)を囲む箱を求める
//  2. 注視点をその箱の真ん中へなめらかに動かす
//  3. 散らばるとズームアウト、集まる・人数が減るとズームイン
// 試合はアリーナ全体が見える所から始まってズームインし、最後の1人はアップになる)
class ComponentGroupCamera : public Component
{
public:
    BP_COMPONENT_DECL(ComponentGroupCamera, u8"グループ追従カメラ");

    void Init() override;

    void Update() override;

    void GUI() override;

    //Jump straight to the current target, no smoothing (e.g. after a scene reset)(なめらかに動かさず、今のターゲットへすぐ移動する(シーンのリセット後など))
    void SnapToTarget();

    //Start again from the whole-arena view and zoom in(アリーナ全体が見える所からもう一度ズームインし直す)
    void ResetToOverview();

private:
    //Feet positions of the characters to frame. Alive ones first; if nobody is alive, everyone
    //(映すキャラの足元の位置。まず生きているキャラ。誰も生きていなければ全員)
    void CollectTargets(std::vector<float3>& out) const;

    //Where the camera wants to be this frame (look-at point and distance)(このフレームでカメラが行きたい場所(注視点と距離))
    void CalcTarget(float3& lookAt, float& distance) const;

    //Best look-at point for these points, returns the distance needed to see them all (not clamped)
    //(これらの点を映す一番よい注視点を求めて、全部見える距離を返す(min/maxで制限しない))
    float FrameTargets(const std::vector<float3>& targets, float3& lookAt, bool useArenaClamp) const;

    //Smallest distance where every target (feet and head) is inside the safe area of the screen
    //(全員(足元と頭)が画面の安全な範囲に入る一番近い距離)
    float CalcFitDistance(const std::vector<float3>& targets, const float3& lookAt) const;

    //Keep the view from going too far outside the arena(アリーナの外を映しすぎないようにする)
    void ClampToArena(float3& lookAt, float distance) const;

    //Push lookAt/distance into the ComponentCamera(注視点と距離をComponentCameraに渡す)
    void ApplyToCamera();

    //---- Tuning (調整用。GUIでも変えられる) ----
    float pitch_        = 50.0f;     //Looking-down angle in degrees. 90 = straight down(見下ろす角度(度)。90＝真上から)
    float fov_          = 45.0f;     //Vertical field of view in degrees(縦の画角(度))
    float minDistance_  = 120.0f;    //Closest zoom (one character left)(一番寄った時の距離(残り1人の時))
    float maxDistance_  = 520.0f;    //Farthest zoom (whole arena)(一番引いた時の距離(アリーナ全体))
    float margin_       = 12.0f;     //Extra space around each character on the ground(地面の上で各キャラのまわりにとる余白)
    float charHeight_   = 30.0f;     //Character height + the PLAYER mark, so heads are not cut(キャラの高さ＋PLAYERマーク。頭が切れないように)
    float screenSide_   = 0.85f;     //Use this much of the screen to the left/right (1.0 = edge)(左右は画面のこの割合まで使う(1.0＝端))
    float screenTop_    = 0.80f;     //Top limit. Leaves room for the timer(上の限界。タイマーの場所をあける)
    float screenBottom_ = 0.68f;     //Bottom limit. Keeps feet above the HP bars(下の限界。足元がHPバーに重ならない)
    float lookHeight_   = 8.0f;      //Look at chest height, not the feet(足元ではなく胸の高さを見る)
    float followSpeed_  = 3.0f;      //How fast the look-at point follows. Bigger = snappier(注視点が追いかける速さ。大きいほどキビキビ)
    float zoomOutSpeed_ = 4.0f;      //Zoom out fast so nobody leaves the screen(誰も画面から出ないよう、引く時は速く)
    float zoomInSpeed_  = 1.5f;      //Zoom in slowly so it feels calm(寄る時はゆっくり落ち着いて)
    float overscan_     = 30.0f;     //How far outside the arena the view may show(アリーナの外をどれだけ映してよいか)
    bool  includeDead_  = false;     //Also frame K.O.'d characters(K.O.されたキャラも映す)
    bool  clampToArena_ = true;      //Use the arena limit above(上のアリーナ制限を使う)

    //---- Runtime state (実行中の状態。保存しない) ----
    float3 lookAt_     = {0.0f, 0.0f, 0.0f};    //Current smoothed look-at point(今のなめらかにした注視点)
    float  distance_   = 330.0f;                //Current smoothed distance(今のなめらかにした距離)
    float3 lastTarget_ = {0.0f, 0.0f, 0.0f};    //For the GUI(GUI表示用)
    float  lastDist_   = 0.0f;
    int    lastCount_  = 0;
    bool   started_    = false;

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver)
    {
        arc(CEREAL_NVP(pitch_),
            CEREAL_NVP(fov_),
            CEREAL_NVP(minDistance_),
            CEREAL_NVP(maxDistance_),
            CEREAL_NVP(margin_),
            CEREAL_NVP(charHeight_),
            CEREAL_NVP(screenSide_),
            CEREAL_NVP(screenTop_),
            CEREAL_NVP(screenBottom_),
            CEREAL_NVP(lookHeight_),
            CEREAL_NVP(followSpeed_),
            CEREAL_NVP(zoomOutSpeed_),
            CEREAL_NVP(zoomInSpeed_),
            CEREAL_NVP(overscan_),
            CEREAL_NVP(includeDead_),
            CEREAL_NVP(clampToArena_));

        arc(cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
    //@}
};

CEREAL_CLASS_VERSION(ComponentGroupCamera, 1);
