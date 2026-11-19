//---------------------------------------------------------------------------
//!	@file	StageConfig.h
//! @brief	All tuning numbers for the stage: item pool, item stats, arena size, carrier, TNT
//!         (ステージの調整用の数字をすべてここに置く：アイテムプール、アイテムの性能、アリーナの広さ、運び役、TNT)
//---------------------------------------------------------------------------
#pragma once
#include "Item.h"
#include <System/EaseCurve.h>
#include <iterator>

namespace PoittersPoint::StageConfig {

//--------------------------------------------------------------------
//! @name Item types(アイテムの種類)
//--------------------------------------------------------------------
//@{

//Every throwable type. Keep Count last, it is the number of types(投げられる物の種類。Countは最後に置く。種類の数として使う)
//(Item.h has a forward declaration "enum class ItemType : int;", so keep ": int" here)
//(Item.hに「enum class ItemType : int;」の前方宣言があるので、ここの「: int」は消さない)
enum class ItemType : int
{
    Cobblestone,    //item0002 丸石
    IceBlock,       //item0004 氷ブロック
    Obsidian,       //item0003 黒曜石
    WoodPlank,      //item0001 木材の板
    Chest,          //item0007 チェスト
    TNT,            //item0005 TNT

    Count
};

//The engine's default model scale (ComponentModel starts at 0.1). modelScale below is multiplied by this
//(エンジンのモデルのデフォルトスケール(ComponentModelは0.1から始まる)。下のmodelScaleはこれに掛けられる)
inline constexpr float MODEL_BASE_SCALE = 0.1f;

//Stats of one item type(1種類のアイテムの性能)
struct ItemDef
{
    const char*      name;           //Name base. A number is added, like "Obsidian_3"(名前のもと。「Obsidian_3」のように番号がつく)
    const char*      model;          //Model file(モデルファイル)
    Item::WeightType weightType;     //Light / Medium / Heavy(軽い・普通・重い)
    float            weight;         //Damage and lift speed come from this(ダメージと持ち上げ速度はここから決まる)
    float            objectScale;    //Object size. Also used by the lift animation(オブジェクトの大きさ。持ち上げアニメにも使う)
    float
        modelScale;    //Model size. 1.0 = engine default (0.1). Use for models made at a different size(モデルの大きさ。1.0＝エンジンのデフォルト(0.1)。大きさが違うモデル用)
    float colRadius;     //Collision capsule radius(当たり判定カプセルの半径)
    float colHeight;     //Collision capsule height(当たり判定カプセルの高さ)
    float colOffsetY;    //Collision capsule up/down offset(当たり判定カプセルの上下のずれ)
};

//Must be in the same order as ItemType!(ItemTypeと同じ順番にすること！)
// clang-format off
inline constexpr ItemDef ITEM_DEFS[] = {
    //name           model                                          weightType                 weight objScale modelScale radius height offsetY
    { "Cobblestone", "data/Game/Models/Object/ThorwRock2.mv1",      Item::WeightType::Medium,  1.3f,  0.55f,   1.0f,      10.0f, 26.0f, -10.0f },
    { "IceBlock",    "data/Game/Models/Object/ThorwIce.mv1",        Item::WeightType::Medium,  1.3f,  0.55f,   1.0f,      10.0f, 26.0f, -10.0f },
    { "Obsidian",    "data/Game/Models/Object/Thorwobsidian.mv1",   Item::WeightType::Heavy,   2.5f,  0.75f,   1.0f,      10.0f, 26.0f, -10.0f },
    { "WoodPlank",   "data/Game/Models/Test/box2.mv1",              Item::WeightType::Light,   0.7f,  0.35f,   1.0f,      10.0f, 26.0f, -10.0f },    //No model yet, box2 for now(モデル未完成なので仮でbox2)
    { "Chest",       "data/Game/Models/Item/Chest.mv1",             Item::WeightType::Medium,  1.3f,  0.5f,    80.0f,     16.44f,33.24f, -2.0f },    //Chest model is tiny, same 0.1*80 as before(チェストのモデルは小さいので、前と同じ0.1×80)
    { "TNT",         "data/Game/Models/Object/ThorwTNT.mv1",        Item::WeightType::Medium,  1.3f,  0.55f,   1.0f,      10.0f, 26.0f, -10.0f },
};
// clang-format on
static_assert(std::size(ITEM_DEFS) == (size_t)ItemType::Count, "ITEM_DEFS must have one entry per ItemType(ITEM_DEFSはItemTypeごとに1つ必要)");

inline const ItemDef& GetItemDef(ItemType type)
{
    return ITEM_DEFS[(int)type];
}
//@}

//--------------------------------------------------------------------
//! @name Item pool(アイテムプール)
//--------------------------------------------------------------------
//@{

//How many of each type are on the field at the same time, and how long until a missing one comes back
//(各種類がフィールドに同時に何個あるか、なくなってから何秒で戻ってくるか)
struct PoolEntry
{
    ItemType type;
    int      count;           //How many on the field(フィールドに置く数)
    float    respawnDelay;    //Seconds after it disappears until the carrier is sent(消えてから運び役が来るまでの秒数)
};

//Chest and TNT break when thrown, so they get a shorter delay and come back more often
//(チェストとTNTは投げると壊れるので、待ち時間を短くして多く出す)
// clang-format off
inline constexpr PoolEntry ITEM_POOL[] = {
    { ItemType::Obsidian,    3, 6.0f },
    { ItemType::IceBlock,    3, 6.0f },
    { ItemType::Cobblestone, 3, 6.0f },
    { ItemType::WoodPlank,   3, 6.0f },
    { ItemType::Chest,       2, 3.0f },
    { ItemType::TNT,         3, 3.0f },
};
// clang-format on

//Height items fall from at the stage start(ステージ開始時にアイテムが落ちてくる高さ)
inline constexpr float START_DROP_HEIGHT = 60.0f;
//@}

//--------------------------------------------------------------------
//! @name Arena(アリーナ)
//--------------------------------------------------------------------
//@{

//Invisible wall. Players, NPCs and items can not go outside this box (x/z only)
//Turn on "draw bounds" in the ArenaBounds GUI to see it, and tune these to match the stage
//(見えない壁。プレイヤー・NPC・アイテムはこの範囲の外に出られない(x/zだけ)
// ArenaBoundsのGUIで「範囲を表示」をオンにすると見える。ステージに合わせて調整する)
inline constexpr float ARENA_MIN_X = -150.0f;
inline constexpr float ARENA_MAX_X = 150.0f;
inline constexpr float ARENA_MIN_Z = -150.0f;
inline constexpr float ARENA_MAX_Z = 150.0f;

//Height of the floor, used for the debug lines and the landing marker(床の高さ。デバッグ線と着地マーカーに使う)
inline constexpr float ARENA_FLOOR_Y = 0.0f;

//Safety net: an item below this height fell out of the stage, so it is deleted and respawned
//(安全ネット：この高さより下のアイテムはステージから落ちたとみなして、消してから出し直す)
inline constexpr float KILL_Y = -200.0f;

//Items do not spawn closer than this to the wall(壁からこの距離より内側にスポーンする)
inline constexpr float SPAWN_EDGE_MARGIN = 15.0f;
//Items do not spawn closer than this to a character(キャラからこの距離より離れてスポーンする)
inline constexpr float SPAWN_MIN_DIST_FROM_CHARACTER = 30.0f;
//Items do not spawn closer than this to another item(ほかのアイテムからこの距離より離れてスポーンする)
inline constexpr float SPAWN_MIN_DIST_FROM_ITEM = 15.0f;
//How many random spots to try before giving up and using the last one(あきらめて最後の場所を使うまでに試すランダムな場所の数)
inline constexpr int SPAWN_TRY_COUNT = 10;
//@}

//--------------------------------------------------------------------
//! @name Spawn carrier (the floating thing that brings items)(運び役(アイテムを運んでくる浮いている物))
//--------------------------------------------------------------------
//@{
inline constexpr const char* CARRIER_MODEL    = "data/Game/Models/Test/box.mv1";    //Placeholder(仮)
inline constexpr float       CARRIER_SCALE_XZ = 1.5f;    //Flat board look. 1.0 = engine default model size(平たい板の見た目。1.0＝エンジンのデフォルトの大きさ)
inline constexpr float       CARRIER_SCALE_Y  = 0.3f;
inline constexpr float       CARRIER_HEIGHT   = 70.0f;       //Height while hovering over the spot(場所の上で浮いている高さ)
inline constexpr float       CARRIER_START_OUT  = 80.0f;     //How far outside the arena it starts(アリーナの外どれだけ離れた所から来るか)
inline constexpr float       CARRIER_FLY_TIME   = 1.5f;      //Seconds to fly to the spot(場所まで飛ぶ秒数)
inline constexpr float       CARRIER_ANIM_TIME  = 0.8f;      //Seconds of spin before the item appears(アイテムが出る前に回る秒数)
inline constexpr float       CARRIER_LEAVE_TIME = 1.0f;      //Seconds to fly away(飛び去る秒数)
inline constexpr float       CARRIER_SPIN_SPEED = 720.0f;    //Spin speed in degrees per second during the animation(アニメ中の回転速度(度/秒))
inline constexpr float       CARRIER_DROP_BELOW = 15.0f;     //The item appears this far under the carrier(アイテムは運び役のこれだけ下に出る)
inline constexpr float       SPAWN_POP_TIME     = 0.25f;     //Seconds for the item to grow from 0 to full size(アイテムが0から本来の大きさになる秒数)
//Shape of the pop. Any EaseType from System/EaseCurve.h. Try OutElastic (wobble) or OutBounce (bounce). Elastic looks better with ~0.6s
//(ポンと出るカーブ。System/EaseCurve.hのEaseTypeならなんでもOK。OutElastic(ぷるぷる)やOutBounce(バウンド)も試せる。Elasticは0.6秒くらいが合う)
inline constexpr EaseType SPAWN_POP_EASE = EaseType::OutBack;
inline constexpr float    MARKER_RADIUS  = 14.0f;    //Ground marker size(地面のマーカーの大きさ)
//@}

//--------------------------------------------------------------------
//! @name TNT
//--------------------------------------------------------------------
//@{
inline constexpr float TNT_BLAST_RADIUS = 40.0f;    //Characters inside this distance get hurt(この距離の中のキャラがダメージを受ける)
inline constexpr int   TNT_BLAST_DAMAGE = 4;        //Damage of the blast(爆風のダメージ)
//@}

//--------------------------------------------------------------------
//! @name Chest
//--------------------------------------------------------------------
//@{
inline constexpr int CHEST_MIN_CONTENT = 3;    //Least consumables inside(中身の消費アイテムの最小数)
inline constexpr int CHEST_MAX_CONTENT = 5;    //Most consumables inside(中身の消費アイテムの最大数)
//@}

}    // namespace PoittersPoint::StageConfig
