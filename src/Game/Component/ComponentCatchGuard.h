//---------------------------------------------------------------------------
//!	@file	ComponentCatchGuard.h
//! @brief	[ADD] cha0003 警察官(POLICE): catch or guard items thrown at this character
//!         ([追加] cha0003 警察官(POLICE)：投げられた物をキャッチ・ガードする)
//!
//! Item::OnHit asks this component first, before the damage. Only items coming from the FRONT can be stopped:
//!   - CATCH : hands are free -> the item goes into the hands, no damage. The NPC throws it right back at the thrower
//!   - GUARD : hands are busy (carrying / lifting / throwing) or the item is too heavy to catch
//!             -> much less damage, no fall-down, only a small push back
//!   - from the side/back, while down or after K.O. -> a normal hit
//! The front is wide (FRONT_ANGLE), so it is "easy to catch/guard" as the task list says.
//! (Item::OnHitはダメージの前にまずこのコンポーネントに聞く。止められるのは「前」から来た物だけ：
//!   - キャッチ：手が空いている → 物が手の中に入る。ダメージなし。NPCは投げた相手にすぐ投げ返す
//!   - ガード  ：手がふさがっている(運び中・持ち上げ中・投げ中)か、重すぎてキャッチできない
//!               → ダメージが大きく減る。倒れない。少し後ろに押されるだけ
//!   - 横・後ろから、ダウン中、K.O.後 → 普通に当たる
//!  前の範囲は広い(FRONT_ANGLE)ので、タスクリストのとおり「キャッチ・ガードしやすい」)
//---------------------------------------------------------------------------
#pragma once
#include <System/Scene.h>
#include <System/Component/Component.h>

USING_PTR(ComponentCatchGuard);

namespace PoittersPoint {
USING_PTR(Item);
}
class ComponentItemPickUp;

class ComponentCatchGuard : public Component
{
public:
    BP_COMPONENT_DECL(ComponentCatchGuard, u8"キャッチ・ガード");

    //What happened to the item that hit us(当たったアイテムがどうなったか)
    enum class Result
    {
        None,     //Normal hit(普通に当たる)
        Catch,    //Caught: the item is in our hands now(キャッチ：物はもう手の中)
        Guard,    //Guarded: less damage, no fall-down(ガード：ダメージが減る、倒れない)
    };

    void Init() override;

    //Small push back after a guard, and the timers(ガード後の小さな押し戻しと、タイマー)
    void Update() override;

    //"CATCH!" / "GUARD!" above the head(頭の上に「CATCH!」/「GUARD!」)
    void PostDraw() override;

    void GUI() override;

    //Item::OnHit calls this BEFORE giving damage. On Catch the item is already in our hands
    //(Item::OnHitがダメージを与える「前」に呼ぶ。Catchなら物はもう手の中にある)
    Result TryBlock(const PoittersPoint::ItemPtr& item, ComponentItemPickUp& pickup);

    //Damage after a guard (rounded down, can be 0)(ガードした後のダメージ(切り捨て。0もある))
    int GetGuardDamage(int damage) const;

    //The AI asks this every frame: true once after a new catch, and outThrower = who threw the item (can be nullptr).
    //Only compare outThrower with live objects, never use it as a pointer
    //(AIが毎フレーム聞く：新しくキャッチした後に1回だけtrue。outThrower＝その物を投げた人(nullptrもある)。
    // outThrowerは生きているオブジェクトと比べるだけで、ポインタとして使わない)
    bool TakeCatch(const Object*& outThrower);

    //[ADD] The AI asks this every frame: is a thrown item flying at us right now? outDir = flat direction from us to that item.
    //The AI then turns toward it, so it comes from the FRONT and can be caught/guarded
    //([追加] AIが毎フレーム聞く：今、投げられた物がこちらへ飛んできているか？outDir＝自分からその物への水平な方向。
    // AIはその方へ振り向くので、物が「前」から来てキャッチ・ガードできる)
    bool FindIncomingItem(float3& outDir) const;

private:
    //Tuning (ImGui "CatchGuard")(調整用(ImGuiの「CatchGuard」))
    float frontAngleDeg_ =
        80.0f;    //Items within this angle from the front can be stopped (90 = the whole front half)(正面からこの角度以内の物を止められる(90で前半分すべて))
    float catchMaxWeight_   = 2.0f;     //Heavier items can not be caught, only guarded (Obsidian etc.)(これより重い物はキャッチできずガードだけ(黒曜石など))
    float guardDamageScale_ = 0.34f;    //Damage x this on a guard: light/medium = 0, heavy = 1(ガード時のダメージ×これ：軽い・普通＝0、重い＝1)
    float guardPushDist_    = 6.0f;     //Push back distance on a guard(ガード時に押される距離)
    float cooldown_         = 0.5f;     //Seconds after a catch/guard before the next one(キャッチ・ガードの後、次ができるまでの秒数)
    float reactRange_       = 90.0f;    //[ADD] Turn toward items flying at us within this distance([追加] この距離以内でこちらへ飛んでくる物の方へ振り向く)
    float reactWidth_       = 18.0f;    //[ADD] "At us" = the item passes closer than this(「こちらへ」＝物がこれより近くを通る)

    float  cooldownTimer_ = 0.0f;
    float3 pushVel_       = {0.0f, 0.0f, 0.0f};    //Push back speed, slows down every frame(押し戻しの速さ。毎フレーム遅くなる)

    //Text above the head(頭の上の文字)
    Result lastResult_ = Result::None;
    float  textTimer_  = 0.0f;
    float  textTime_   = 0.7f;

    //Counter-throw target for the AI(AI用の投げ返す相手)
    const Object* catchThrower_ = nullptr;
    bool          caughtNew_    = false;    //A catch the AI has not read yet(AIがまだ読んでいないキャッチ)

    //Count of catches/guards, only for the GUI(キャッチ・ガードの回数。GUI表示用)
    int catchCount_ = 0;
    int guardCount_ = 0;

    //True if the item flies toward our front(物が自分の前から飛んできているか)
    bool IsFromFront(const float3& itemVelocity) const;

    //Hands are free: not carrying, not lifting, not in a throw animation(手が空いている：運んでいない・持ち上げていない・投げアニメ中でない)
    bool HandsFree() const;

    //--------------------------------------------------------------------
    //! @name Cereal処理
    //--------------------------------------------------------------------
    //@{
    CEREAL_SAVELOAD(arc, ver) { arc(cereal::make_nvp("Component", cereal::base_class<Component>(this))); }
};

CEREAL_CLASS_VERSION(ComponentCatchGuard, 1);
