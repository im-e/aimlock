#include "Game/ScoreBoard.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

//scores.txt, one run per line:
//  scenario score accuracy cm360 date time
//  controlsphere 4520 0.913 51.95 2026-11-27 14:02

void ScoreBoard::load(const std::string& _path)
{
    path = _path;
    entries.clear();

    std::ifstream file(path);
    if (!file)
        return; //first launch, nothing saved yet

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream in(line);
        ScoreEntry entry;
        std::string day, time;
        if (in >> entry.scenario >> entry.score >> entry.accuracy >> entry.cmPer360 >> day >> time)
        {
            entry.date = day + " " + time;
            entries.push_back(entry);
        }
        //a broken line just gets skipped rather than losing the whole file
    }
}

int ScoreBoard::add(const ScoreEntry& entry)
{
    //rank = 1 + how many runs beat it, so equal scores share a place
    int rank = 1;
    for (const ScoreEntry& e : entries)
        if (e.scenario == entry.scenario && e.score > entry.score)
            rank++;

    entries.push_back(entry);

    //append rather than rewrite, so a crash halfway through saving can't wipe old scores
    bool newFile = !std::ifstream(path).good();
    std::ofstream file(path, std::ios::app);
    if (newFile)
        file << "# scenario score accuracy cm360 date time\n";
    file << entry.scenario << " " << entry.score << " " << entry.accuracy << " "
         << entry.cmPer360 << " " << entry.date << "\n";

    return rank;
}

std::vector<ScoreEntry> ScoreBoard::top(const std::string& scenario, size_t count) const
{
    std::vector<ScoreEntry> result;
    for (const ScoreEntry& e : entries)
        if (e.scenario == scenario)
            result.push_back(e);

    //stable so that on a tie the older run stays above the newer one
    std::stable_sort(result.begin(), result.end(),
        [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });

    if (result.size() > count)
        result.resize(count);
    return result;
}

int ScoreBoard::best(const std::string& scenario) const
{
    int best = 0;
    for (const ScoreEntry& e : entries)
        if (e.scenario == scenario && e.score > best)
            best = e.score;
    return best;
}

int ScoreBoard::runCount(const std::string& scenario) const
{
    int count = 0;
    for (const ScoreEntry& e : entries)
        if (e.scenario == scenario)
            count++;
    return count;
}

std::string ScoreBoard::currentDate()
{
    std::time_t now = std::time(nullptr);
    std::tm local = {};
    localtime_s(&local, &now);
    char text[32];
    std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M", &local);
    return text;
}
