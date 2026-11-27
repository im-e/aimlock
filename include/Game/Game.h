#pragma once
#include "Game/GameState.h"
#include "Game/Camera.h"
#include "Game/GameObject.h"
#include "Engine/Platform/Window.h"
#include <vector>
#include <string>
#include <random>

//what happened during one run, for the hud and results screen
struct RunStats
{
    int shots = 0;
    int hits = 0;

    int score() const { return hits; } //just kills for now, each scenario will score itself later
    float accuracy() const { return shots > 0 ? (float)hits / (float)shots : 0.0f; }
};

//owns the game state machine and everything gameplay related
//main.cpp handles the window and the rendering, this decides what's on screen and what the input does
class Game
{
public:
    static constexpr float COUNTDOWN_LENGTH = 3.0f; //seconds
    static constexpr float RUN_LENGTH = 60.0f;
    static constexpr float TARGET_RADIUS = 0.3f;    //metres

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
    float stateTimer = 0.0f;  //seconds since we entered the current state
    bool quitRequested = false;

    Camera camera;
    std::vector<GameObject> room;
    GameObject target;

    RunStats stats;
    float timeLeft = 0.0f;  //only counts down while Playing, so pausing doesn't eat into the run
    int bestScore = 0;
    bool newBest = false;

    std::mt19937 rng;

    void changeState(GameState next, Window& win);

    //one per state
    void updateMainMenu(Window& win);
    void updateCountdown(Window& win);
    void updatePlaying(Window& win, float dt);
    void updatePaused(Window& win);
    void updateResults(Window& win);

    void buildRoom(Mesh* cubeMesh);
    void startRun(Window& win);
    void finishRun(Window& win);
    void shoot();
    void spawnTarget();
    float randomRange(float lo, float hi);
};
