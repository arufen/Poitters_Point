//---------------------------------------------------------------------------
//!	@file	MatchResult.h
//! @brief	The result of the last match. MainStage fills it, the Result scene reads it
//!         (最後の試合の結果。MainStageが書いて、Resultシーンが読む)
//---------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>

namespace PoittersPoint {

struct MatchResult
{
    //From the human player's point of view(人間のプレイヤーから見た結果)
    enum class Outcome
    {
        Win,
        Lose,
        Draw,
    };

    //Why the match ended(試合が終わった理由)
    enum class Reason
    {
        Knockout,    //Only one (or nobody) left(1人だけ(または誰も)残っていない)
        TimeUp,      //The timer reached 0:00(タイマーが0:00になった)
    };

    //One character in the ranking(ランキングの1人分)
    struct Entry
    {
        std::string label;    //"PLAYER", "NPC 1"... same as the HP bar(HPバーと同じ名前)
        int         hp         = 0;
        int         maxHp      = 0;
        bool        isPlayer   = false;
        bool        knockedOut = false;
    };

    Outcome            outcome = Outcome::Draw;
    Reason             reason  = Reason::TimeUp;
    std::string        winner;     //Label of the winner, empty on a draw(勝者の名前。引き分けなら空)
    std::vector<Entry> ranking;    //1st place first(1位が最初)
};

//The one shared result. Simple on purpose: only one match runs at a time
//(共有の結果は1つだけ。試合は同時に1つしかないので、わざとシンプルにしている)
inline MatchResult& LastMatchResult()
{
    static MatchResult result;
    return result;
}

}    // namespace PoittersPoint
