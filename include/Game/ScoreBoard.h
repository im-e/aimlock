#pragma once
#include <string>
#include <vector>

//one finished run
struct ScoreEntry
{
    std::string scenario;  //scenario id, e.g. "controlsphere"
    int score = 0;
    float accuracy = 0.0f; //0-1
    float cmPer360 = 0.0f; //what sens you were on, handy when looking back at old scores
    std::string date;      //"2026-11-27 14:02"
};

//every run gets added to scores.txt, the leaderboard is just those runs sorted by score
//keeping every run (not only the top 10) means there's a history to look back on later
class ScoreBoard
{
public:
    void load(const std::string& path);

    //saves the run to the file straight away and returns where it placed on the leaderboard (1 = top)
    int add(const ScoreEntry& entry);

    std::vector<ScoreEntry> top(const std::string& scenario, size_t count) const;
    int best(const std::string& scenario) const; //0 if it's never been played
    int runCount(const std::string& scenario) const;

    static std::string currentDate();

private:
    std::string path;
    std::vector<ScoreEntry> entries;
};
