//---------------------------------------------------------------------------
//!	@file	Player.cpp
//! @brief	Player
//---------------------------------------------------------------------------

#include "Player.h"
#include "Npc.h"
#include "MainStage.h"
#include "Component/ComponentCameraController.h"
#include "Component/ComponentCharacterStats.h"
#include "Component/ComponentPlayerState.h"
#include "Component/ComponentPlayerPickUpTrigger.h"
#include "Component/ComponentStateIdleWalk.h"
#include <cstring>
#include <string>
#include <vector>

namespace {
//One entry of the bottom HP row: who it is and where the HP comes from(下のHP列の1人分：誰のHPか)
struct HpEntry
{
    std::string                label;       //Name shown above the bar(バーの上に出す名前)
    ComponentCharacterStatsPtr stats;       //HP source(HPの元になるステータス)
    bool                       isPlayer;    //True = the human player(プレイヤー本人ならtrue)
};

//Draw text with a black shadow so it is easy to read on any ground(どんな地面の上でも読めるように、黒い影つきで文字を描く)
void DrawTextWithShadow(int x, int y, const char* text, unsigned int color)
{
    DrawString(x + 1, y + 1, text, GetColor(0, 0, 0));
    DrawString(x, y, text, color);
}

//Bar color changes with the HP ratio: green -> yellow -> red(HPの割合でバーの色を変える：緑→黄→赤)
unsigned int GetHpBarColor(float ratio)
{
    if(ratio > 0.5f)
        return GetColor(60, 200, 60);    //Green(緑)
    if(ratio > 0.25f)
        return GetColor(240, 200, 0);    //Yellow(黄)
    return GetColor(230, 40, 40);        //Red(赤)
}

//Draw one HP panel. (x, barY) is the top-left of the bar(HPパネルを1つ描く。(x, barY)がバーの左上)
void DrawHpPanel(int x, int barY, int w, int h, const HpEntry& entry)
{
    const ComponentCharacterStats& stats = *entry.stats;

    bool  dead  = stats.IsDead();
    float ratio = stats.GetHPRatio();

    //Text line sits right above the bar. Use the current font size, so we never change the global font(文字はバーのすぐ上。今のフォントサイズを使うので、全体のフォントは変えない)
    int textY = barY - GetFontSize() - 2;

    //Name on the left. The player is yellow like the PLAYER marker. Dead = gray(左に名前。プレイヤーは頭上のPLAYERと同じ黄色。死亡は灰色)
    unsigned int nameColor = entry.isPlayer ? GetColor(255, 220, 0) : GetColor(255, 255, 255);
    if(dead)
        nameColor = GetColor(130, 130, 130);

    DrawTextWithShadow(x, textY, entry.label.c_str(), nameColor);

    //HP numbers on the right, or K.O. when dead(右にHPの数字。死んでいたらK.O.)
    std::string hpText = dead ? "K.O." : std::to_string(stats.GetHP()) + "/" + std::to_string(stats.GetMaxHP());
    int         textW  = GetDrawStringWidth(hpText.c_str(), (int)hpText.size());

    DrawTextWithShadow(x + w - textW, textY, hpText.c_str(), dead ? GetColor(230, 40, 40) : GetColor(255, 255, 255));

    //Bar: background, HP fill, border. Same look as the old player bar(バー：背景・HP・枠。前のプレイヤーのバーと同じ見た目)
    DrawBox(x, barY, x + w, barY + h, GetColor(50, 50, 50), TRUE);

    int fillW = (int)(w * ratio);
    if(!dead && fillW < 2)
        fillW = 2;    //Still alive = always show a tiny bit(生きている間は少しだけ必ず見せる)

    if(!dead)
        DrawBox(x, barY, x + fillW, barY + h, GetHpBarColor(ratio), TRUE);

    DrawBox(x, barY, x + w, barY + h, GetColor(255, 255, 255), FALSE);

    //Speed potion active: cyan outline and a thin timer line under the bar(スピードポーション中：水色の外枠と、バーの下に残り時間の細い線)
    if(stats.HasSpeedBuff()) {
        unsigned int cyan = GetColor(0, 220, 255);
        DrawBox(x - 1, barY - 1, x + w + 1, barY + h + 1, cyan, FALSE);
        DrawBox(x, barY + h + 3, x + (int)(w * stats.GetSpeedBuffRatio()), barY + h + 6, cyan, TRUE);
    }
}
}    // namespace

namespace PoittersPoint {
//! @brief 初期化 / Initialize
//! @return 初期化終了 / Initialization finished
bool Player::Init()
{
    Super::Init();

    SetName("Player");

    AddComponent<ComponentModel>("data/Game/Models/Player/Salaryman.mv1")
        ->SetAnimation({
            { "idle",          "data/Game/Models/Player/Anims/Idle.mv1", 0, 1.0f}, // Idle
            { "walk",       "data/Game/Models/Player/Anims/Walking.mv1", 0, 3.3f}, // Walk
            {"throw", "data/Game/Models/Player/Anims/Carry_Walking.mv1", 1, 1.0f}  // Throw/Carry related
    });

    SetTranslate({0, 5, 0});

    //Random stats: HP 10-20, strength 0.8-1.3(ランダムなステータス：HP10〜20、筋力0.8〜1.3)
    AddComponent<ComponentCharacterStats>()->SetMaxHP(10 + GetRand(10))->SetStrength(0.8f + (GetRand(5) * 0.1f))->SetSpeed(2.0f);

    AddComponent<ComponentPlayerState>();
    AddComponent<ComponentCameraController>();

    auto col = AddComponent<ComponentCollisionCapsule>()    //
                   ->SetRadius(3.0f)
                   ->SetHeight(23.4f);

    col->UseGravity();
    col->SetCollisionGroup(ComponentCollision::CollisionGroup::PLAYER);

    AddComponent<ComponentPlayerPickUpTrigger>();

    //UI
    UI_key_e = LoadGraph("data/Game/UI/E_Key.png");

    return true;
}

void Player::Update()
{
    Super::Update();

    auto pickUpTrigger = GetComponent<ComponentPlayerPickUpTrigger>();
    if(!pickUpTrigger)
        return;

    //Can not move while lifting an item. Turn the walk component off and on(アイテムを持ち上げている間は動けない。歩くコンポーネントをオフ/オンする)
    bool lifting = pickUpTrigger->IsLifting();
    if(auto walk = GetComponent<ComponentStateIdleWalk>())
        walk->SetStatus(Component::StatusBit::Enable, !lifting);

    if(lifting) {
        //Stand still until the lift is done(持ち上げが終わるまで立ち止まる)
        if(auto mdl = GetComponent<ComponentModel>())
            mdl->PlayAnimationNoSame("idle", true);

        return;
    }

    //E key: throw if holding an item, otherwise pick up(Eキー：持っていれば投げる、持っていなければ拾う)
    if(!Input::IsKeyOn(KEY_INPUT_E))
        return;

    if(pickUpTrigger->HasItem())
        pickUpTrigger->ThrowItem();
    else
        pickUpTrigger->PickUpItem();
}

void Player::PostDraw()
{
    //Always show who the player is(いつもプレイヤーが誰か分かるように表示する)
    DrawPlayerMarker();
    DrawPickUpIcon();
    DrawHpHud();
}

void Player::DrawPickUpIcon()
{
    auto pickUpTrigger = GetComponent<ComponentPlayerPickUpTrigger>();
    if(!pickUpTrigger)
        return;

    auto item = pickUpTrigger->GetNearbyItem();
    if(!item)
        return;

    //Item world position -> screen position, so the icon floats right above the item(アイテムのワールド座標を画面座標に変換して、アイテムの真上にアイコンを出す)
    float3        pos        = item->GetTranslate();
    DxLib::VECTOR world_pos  = {pos.x, pos.y + 5.0f, pos.z};
    DxLib::VECTOR screen_pos = ConvWorldPosToScreenPos(world_pos);

    DrawRotaGraphF(screen_pos.x, screen_pos.y, 0.05f, 0.0f, UI_key_e, 1);
}

void Player::DrawHpHud()
{
    //Collect everyone who has HP. The player is always first (left), then the NPCs(HPを持つ全員を集める。プレイヤーが一番左、そのあとNPC)
    std::vector<HpEntry> entries;

    if(auto myStats = GetComponent<ComponentCharacterStats>())
        entries.push_back({"PLAYER", myStats, true});

    int npcNumber = 1;    //Shown as NPC 1, NPC 2... (NPCの名前は増え続けるので、表示は1から数え直す)
    for(auto& npc : Scene::GetObjectsPtr<PoittersPoint::Npc>()) {
        if(auto npcStats = npc->GetComponent<ComponentCharacterStats>())
            entries.push_back({"NPC " + std::to_string(npcNumber), npcStats, false});

        npcNumber++;
    }

    if(entries.empty())
        return;

    //Layout in pixels(レイアウト(ピクセル))
    const int barH   = 18;    //Bar height(バーの高さ)
    const int gap    = 30;    //Space between panels(パネルの間隔)
    const int margin = 20;    //Space from the screen edge(画面の端からの余白)
    int       count  = (int)entries.size();

    //Each panel is 200px like before, but gets smaller if the window is too narrow(1パネル200pxだが、画面が狭い時は小さくする)
    int panelW = 200;
    int maxW   = (WINDOW_W - margin * 2 - gap * (count - 1)) / count;
    if(panelW > maxW)
        panelW = maxW;

    //Center the whole row, put it at the bottom(列全体を中央にそろえて、画面の下に置く)
    int totalW = panelW * count + gap * (count - 1);
    int startX = (WINDOW_W - totalW) / 2;
    int barY   = WINDOW_H - margin - barH;

    for(int i = 0; i < count; ++i) DrawHpPanel(startX + i * (panelW + gap), barY, panelW, barH, entries[i]);
}

void Player::DrawPlayerMarker()
{
    //Head position in the world, then convert to the screen position(頭の位置をワールド座標で決めて、画面の位置に変換する)
    float3        pos        = GetTranslate();
    DxLib::VECTOR world_pos  = {pos.x, pos.y + markerHeight_, pos.z};
    DxLib::VECTOR screen_pos = ConvWorldPosToScreenPos(world_pos);

    //Do not draw when the player is behind the camera(カメラの後ろにいる時は描かない)
    if(screen_pos.z <= 0.0f || screen_pos.z >= 1.0f)
        return;

    int x = (int)screen_pos.x;
    int y = (int)screen_pos.y;

    unsigned int markColor    = GetColor(255, 220, 0);    //Yellow(黄色)
    unsigned int outlineColor = GetColor(0, 0, 0);        //Black outline so it is easy to read(読みやすいように黒い縁取り)

    //Triangle that points down at the head. The tip is at (x, y)(頭を指す下向きの三角形。先端が(x, y))
    DrawTriangle(x, y + 1, x - 10, y - 15, x + 10, y - 15, outlineColor, TRUE);
    DrawTriangle(x, y - 2, x - 7, y - 14, x + 7, y - 14, markColor, TRUE);

    //Text above the triangle, centered(三角形の上に文字を中央ぞろえで描く)
    const char* text = "PLAYER";
    int         tx   = x - GetDrawStringWidth(text, (int)strlen(text)) / 2;
    int         ty   = y - 34;

    //Draw 4 black copies around the text to make an outline(まわりに黒い文字を4つ描いて縁取りにする)
    DrawString(tx - 1, ty, text, outlineColor);
    DrawString(tx + 1, ty, text, outlineColor);
    DrawString(tx, ty - 1, text, outlineColor);
    DrawString(tx, ty + 1, text, outlineColor);
    DrawString(tx, ty, text, markColor);
}

}    // namespace PoittersPoint
