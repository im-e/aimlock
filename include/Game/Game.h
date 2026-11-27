#pragma once
#include "Game/GameState.h"
#include "Game/Camera.h"
#include "Game/GameObject.h"
#include "Game/Scenario.h"
#include "Game/ScoreBoard.h"
#include "Game/Settings.h"
#include "Engine/Platform/Window.h"
#include <vector>
#include <string>
#include <memory>

//owns the game state machine and everything gameplay related
//main.cpp handles the window and the rendering, this decides what's on screen and what the input does
class Game
{
public:
    static constexpr float COUNTDOWN_LENGTH = 3.0f; //seconds
    static constexpr int LEADERBOARD_SIZE = 10;

    void init(Window& win, Mesh* cubeMesh, Mesh* sphereMesh);
    void update(Window& win, float dt);

    //fills the list with everything that should be drawn this frame
    void getDrawList(std::vector<const GameObject*>& out) const;

    GameState getState() const { return state; }
    const Camera& getCamera() const { return camera; }
    bool wantsToQuit() const { return quitRequested; }

    //one line describing the current screen, shown in the window title until the ui renderer exists
    std::string getStatusText() const;

private:
    GameState state = GameState::MainMenu;
    GameState returnState = GameState::MainMenu; //where Settings/Leaderboard go back to
    float stateTimer = 0.0f; //seconds since we entered the current state
    bool quitRequested = false;

    Camera camera;
    Settings settings;
    ScoreBoard scores;
    std::vector<GameObject> room;

    std::vector<std::unique_ptr<Scenario>> scenarios;
    int selectedScenario = 0;
    int leaderboardScenario = 0; //can flick through other scenarios' boards without changing the selection
    int selectedSetting = 0;

    //the run in progress
    float timeLeft = 0.0f; //only counts down while Playing, so pausing doesn't eat into the run
    int lastRank = 0;      //where the last run placed on the leaderboard
    bool newBest = false;

    Scenario& currentScenario() { return *scenarios[selectedScenario]; }
    const Scenario& currentScenario() const { return *scenarios[selectedScenario]; }

    void changeState(GameState next, Window& win);

    //one per state
    void updateMainMenu(Window& win);
    void updateScenarioSelect(Window& win);
    void updateLeaderboard(Window& win);
    void updateSettings(Window& win);
    void updateCountdown(Window& win);
    void updatePlaying(Window& win, float dt);
    void updatePaused(Window& win);
    void updateResults(Window& win);

    void buildRoom(Mesh* cubeMesh);
    void startRun(Window& win);
    void finishRun(Window& win);
    void changeSetting(int direction, bool bigStep);
    void printLeaderboard() const;
};
