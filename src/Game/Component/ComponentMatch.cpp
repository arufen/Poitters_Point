#include "Game/Component/ComponentMatch.h"
#include "Game/Component/ComponentCharacterStats.h"
#include "Game/Component/ComponentPlayerPickUpTrigger.h"
#include "Game/StageConfig.h"
#include "Game/MatchResult.h"
#include "Game/Player.h"
#include "Game/Npc.h"
#include "Game/Result.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace SC = PoittersPoint::StageConfig;

namespace {
//The match running in the current scene. weak_ptr: it simply expires when the stage is released
//(今のシーンで動いている試合。weak_ptrなので、ステージが解放されると自然に消える)
std::weak_ptr<ComponentMatch> s_current;

const unsigned int COLOR_WHITE  = 0xFFFFFF;
const unsigned int COLOR_YELLOW = 0xFFD200;
const unsigned int COLOR_RED    = 0xFF3C3C;
const unsigned int COLOR_EDGE   = 0x000000;
}    // namespace

//---------------------------------------------------------------------------
// Setup and cleanup(準備と後片付け)
//---------------------------------------------------------------------------
void ComponentMatch::Init()
{
    Super::Init();

    s_current = std::dynamic_pointer_cast<ComponentMatch>(shared_from_this());

    state_      = State::Countdown;
    stateTimer_ = 0.0f;
    timeLeft_   = SC::MATCH_TIME;
    goTimer_    = 0.0f;
    collected_  = false;
    koCount_    = 0;
    fighters_.clear();

    //Own font handles, so we never change the global font size (the Title scene uses it)(自分のフォントハンドル。グローバルのフォントサイズは変えない(Titleシーンが使っている))
    fontHuge_  = CreateFontToHandle(nullptr, 160, 9, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 6);
    fontBig_   = CreateFontToHandle(nullptr, 96, 9, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 5);
    fontTimer_ = CreateFontToHandle(nullptr, 40, 7, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 3);
    fontSmall_ = CreateFontToHandle(nullptr, 18, 6, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 2);

    //Draw at the very last step, after the HP bars, HDR and filters(HPバー・HDR・フィルターの後、一番最後に描く)
    SetProc("MatchUI", [this]() { DrawUI(); }, ProcTiming::UI, ProcPriority::NORMAL);
}

void ComponentMatch::Exit()
{
    for(int* font : {&fontHuge_, &fontBig_, &fontTimer_, &fontSmall_}) {
        if(*font != -1) {
            DeleteFontToHandle(*font);
            *font = -1;
        }
    }

    Super::Exit();
}

bool ComponentMatch::CanAct(::Object* character)
{
    auto match = s_current.lock();
    if(!match)
        return true;    //No match in this scene(このシーンに試合はない)

    if(match->state_ != State::Playing)
        return false;    //Countdown or match over(カウントダウン中か試合終了)

    if(auto stats = character->GetComponent<ComponentCharacterStats>())
        return !stats->IsDead();

    return true;
}

//---------------------------------------------------------------------------
// Match flow(試合の流れ)
//---------------------------------------------------------------------------
void ComponentMatch::CollectFighters()
{
    fighters_.clear();

    //Same names and order as the HP bar in Player.cpp(Player.cppのHPバーと同じ名前と順番)
    for(auto& player : Scene::GetObjectsPtr<PoittersPoint::Player>()) {
        Fighter f;
        f.object   = player;
        f.label    = "PLAYER";
        f.isPlayer = true;
        fighters_.push_back(f);
    }

    int npcNumber = 1;
    for(auto& npc : Scene::GetObjectsPtr<PoittersPoint::Npc>()) {
        Fighter f;
        f.object = npc;
        f.label  = "NPC " + std::to_string(npcNumber++);
        fighters_.push_back(f);
    }

    collected_ = true;
}

void ComponentMatch::CheckKnockouts()
{
    for(auto& f : fighters_) {
        if(f.knockedOut)
            continue;

        auto obj = f.object.lock();
        if(!obj)
            continue;

        auto stats = obj->GetComponent<ComponentCharacterStats>();
        if(!stats || !stats->IsDead())
            continue;

        //New K.O.: remember the order, let go of the item(新しいK.O.：順番を覚えて、アイテムを離す)
        f.knockedOut = true;
        f.koOrder    = ++koCount_;

        if(auto trigger = obj->GetComponent<ComponentPlayerPickUpTrigger>())
            trigger->DropItem();
    }
}

int ComponentMatch::CountAlive() const
{
    int alive = 0;
    for(auto& f : fighters_)
        if(!f.knockedOut && !f.object.expired())
            alive++;
    return alive;
}

void ComponentMatch::Update()
{
    Super::Update();

    //Characters are created by MainStage before this object, so they all exist by the first Update
    //(キャラはMainStageがこのオブジェクトより先に作るので、最初のUpdateには全員そろっている)
    if(!collected_)
        CollectFighters();

    float delta  = GetDeltaTime();
    stateTimer_ += delta;

    switch(state_) {
    case State::Countdown:
        if(stateTimer_ >= (float)SC::COUNTDOWN_FROM) {
            state_      = State::Playing;
            stateTimer_ = 0.0f;
            goTimer_    = SC::GO_SHOW_TIME;
        }
        break;

    case State::Playing:
        if(goTimer_ > 0.0f)
            goTimer_ -= delta;

        timeLeft_ -= delta;
        CheckKnockouts();

        //Last one standing (or nobody, e.g. TNT took out the last two)(最後の1人(または0人。TNTで最後の2人が同時に倒れた時など))
        if(fighters_.size() >= 2 && CountAlive() <= 1)
            Finish(false);
        else if(timeLeft_ <= 0.0f) {
            timeLeft_ = 0.0f;
            Finish(true);
        }
        break;

    case State::Finished:
        if(stateTimer_ >= SC::FINISH_BANNER_TIME) {
            state_      = State::FadeOut;
            stateTimer_ = 0.0f;
        }
        break;

    case State::FadeOut:
        if(stateTimer_ >= SC::SCENE_FADE_TIME) {
            state_ = State::Done;

            //Scenes are cached by the engine. Release the old Result first, so a fresh one is built and its Init() runs again
            //(シーンはエンジンにキャッシュされる。古いResultを先に解放して、新しく作り直してInit()がまた動くようにする)
            //Note: use ReleaseScene(ptr), NOT ReleaseScene<T>(). The template version is declared in the engine but never defined (link error)
            //(注意：ReleaseScene<T>()ではなくReleaseScene(ptr)を使う。テンプレート版はエンジンで宣言だけされていて中身がない(リンクエラー))
            Scene::ReleaseScene(Scene::GetScene<PoittersPoint::Result>());
            Scene::Change(Scene::GetScene<PoittersPoint::Result>());
        }
        break;

    case State::Done:
        break;
    }
}

void ComponentMatch::Finish(bool timeUp)
{
    timeUp_     = timeUp;
    state_      = State::Finished;
    stateTimer_ = 0.0f;

    using PoittersPoint::MatchResult;

    //Snapshot of everyone(全員の今の状態)
    struct Row
    {
        MatchResult::Entry entry;
        int                koOrder = 0;
    };
    std::vector<Row> alive, out;

    for(auto& f : fighters_) {
        Row row;
        row.entry.label      = f.label;
        row.entry.isPlayer   = f.isPlayer;
        row.entry.knockedOut = f.knockedOut;
        row.koOrder          = f.koOrder;

        if(auto obj = f.object.lock()) {
            if(auto stats = obj->GetComponent<ComponentCharacterStats>()) {
                row.entry.hp    = stats->GetHP();
                row.entry.maxHp = stats->GetMaxHP();
            }
        }

        (f.knockedOut ? out : alive).push_back(row);
    }

    //Still standing: most HP first. K.O.'d: the one who lasted longest first
    //(残っている人：HPが多い順。K.O.された人：長く残った順)
    std::stable_sort(alive.begin(), alive.end(), [](const Row& a, const Row& b) { return a.entry.hp > b.entry.hp; });
    std::stable_sort(out.begin(), out.end(), [](const Row& a, const Row& b) { return a.koOrder > b.koOrder; });

    MatchResult& result = PoittersPoint::LastMatchResult();
    result              = MatchResult{};
    result.reason       = timeUp ? MatchResult::Reason::TimeUp : MatchResult::Reason::Knockout;

    for(auto& r : alive) result.ranking.push_back(r.entry);
    for(auto& r : out) result.ranking.push_back(r.entry);

    //Winner: nobody left = draw. Two or more tied on top HP at time up = draw
    //(勝者：誰も残っていない＝引き分け。時間切れでHPトップが同点＝引き分け)
    bool draw = alive.empty() || (alive.size() >= 2 && alive[0].entry.hp == alive[1].entry.hp);

    if(draw) {
        result.outcome = MatchResult::Outcome::Draw;
    }
    else {
        result.winner  = alive[0].entry.label;
        result.outcome = alive[0].entry.isPlayer ? MatchResult::Outcome::Win : MatchResult::Outcome::Lose;
    }
}

//---------------------------------------------------------------------------
// UI(UI)
//---------------------------------------------------------------------------
void ComponentMatch::DrawCentered(const char* text, int y, int font, unsigned int color, float scale)
{
    int len = (int)strlen(text);
    int w   = (int)(GetDrawStringWidthToHandle(text, len, font) * scale);
    int x   = (WINDOW_W - w) / 2;
    if(scale == 1.0f)
        DrawStringToHandle(x, y, text, color, font, COLOR_EDGE);
    else
        DrawExtendStringToHandle(x, y, scale, scale, text, color, font, COLOR_EDGE);
}

void ComponentMatch::DrawUI()
{
    DrawTimer();
    DrawCountdown();
    DrawBanner();
    DrawFade();

    //Always put the blend mode back(ブレンドモードは必ず戻す)
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void ComponentMatch::DrawCountdown()
{
    int fontH = GetFontSizeToHandle(fontHuge_);

    if(state_ == State::Countdown) {
        //3, 2, 1. Each number starts big and shrinks into place (a "pop")(3、2、1。数字は大きく出て、元の大きさに縮む(「ポン」と出る))
        int   number = SC::COUNTDOWN_FROM - (int)stateTimer_;
        float t      = stateTimer_ - floorf(stateTimer_);    //0..1 inside this second(この1秒の中の0〜1)
        float pop    = t * 5.0f;
        if(pop > 1.0f)
            pop = 1.0f;
        float scale = 1.6f - 0.6f * pop;

        std::string text = std::to_string(number);
        int         y    = (int)(WINDOW_H * 0.5f - fontH * scale * 0.5f);
        DrawCentered(text.c_str(), y, fontHuge_, COLOR_WHITE, scale);
        return;
    }

    if(state_ == State::Playing && goTimer_ > 0.0f) {
        //GO! fades out while the players already move(GO!はプレイヤーがもう動いている間に消えていく)
        float a = goTimer_ / SC::GO_SHOW_TIME;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(255 * a));
        int y = (int)(WINDOW_H * 0.5f - fontH * 0.5f);
        DrawCentered("GO!", y, fontHuge_, COLOR_YELLOW);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void ComponentMatch::DrawTimer()
{
    //Top-left box: "TIME" and mm:ss, like the reference(左上の箱：「TIME」とmm:ss。参考画像と同じ)
    int total   = (int)ceilf(timeLeft_);
    int minutes = total / 60;
    int seconds = total % 60;

    char text[16];
    snprintf(text, sizeof(text), "%02d:%02d", minutes, seconds);

    const int x = 20, y = 16, w = 150, h = 74;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
    DrawBox(x, y, x + w, y + h, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    //Red in the last seconds(残り数秒は赤)
    bool         warning = state_ == State::Playing && timeLeft_ <= SC::TIMER_WARNING_TIME;
    unsigned int color   = warning ? COLOR_RED : COLOR_WHITE;

    DrawStringToHandle(x + 12, y + 6, "TIME", COLOR_YELLOW, fontSmall_, COLOR_EDGE);
    DrawStringToHandle(x + 12, y + 26, text, color, fontTimer_, COLOR_EDGE);
}

void ComponentMatch::DrawBanner()
{
    if(state_ != State::Finished && state_ != State::FadeOut && state_ != State::Done)
        return;

    //Pops in like the countdown(カウントダウンと同じようにポンと出る)
    float pop = state_ == State::Finished ? stateTimer_ * 5.0f : 1.0f;
    if(pop > 1.0f)
        pop = 1.0f;
    float scale = 1.5f - 0.5f * pop;

    int         fontH = GetFontSizeToHandle(fontBig_);
    int         y     = (int)(WINDOW_H * 0.5f - fontH * scale * 0.5f);
    const char* text  = timeUp_ ? "TIME UP!" : "FINISH!";
    DrawCentered(text, y, fontBig_, timeUp_ ? COLOR_RED : COLOR_YELLOW, scale);
}

void ComponentMatch::DrawFade()
{
    if(state_ != State::FadeOut && state_ != State::Done)
        return;

    float a = (state_ == State::Done) ? 1.0f : stateTimer_ / SC::SCENE_FADE_TIME;
    if(a > 1.0f)
        a = 1.0f;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(255 * a));
    DrawBox(0, 0, WINDOW_W, WINDOW_H, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void ComponentMatch::GUI()
{
    __super::GUI();

    //Show inside the GUI(GUI内に出現させる)
    ImGui::Begin(GetOwner()->GetName().data());
    {
        ImGui::Separator();
        if(ImGui::TreeNode("Match")) {
            static const char* STATE_NAMES[] = {"Countdown", "Playing", "Finished", "FadeOut", "Done"};
            ImGui::Text("State: %s  (%.1f)", STATE_NAMES[(int)state_], stateTimer_);
            ImGui::Text("Time left: %.1f", timeLeft_);
            ImGui::Text("Alive: %d / %d", CountAlive(), (int)fighters_.size());

            //Test buttons(テスト用ボタン)
            if(state_ == State::Playing) {
                if(ImGui::Button(u8"10秒にする"))
                    timeLeft_ = 10.0f;
                ImGui::SameLine();
                if(ImGui::Button(u8"今すぐ終了(TIME UP)"))
                    timeLeft_ = 0.0f;
            }

            for(auto& f : fighters_)
                ImGui::Text("%-8s %s %s", f.label.c_str(), f.knockedOut ? "K.O." : "", f.knockedOut ? std::to_string(f.koOrder).c_str() : "");

            ImGui::TreePop();
        }
    }
    ImGui::End();
}

CEREAL_REGISTER_TYPE(ComponentMatch)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ComponentMatch)
