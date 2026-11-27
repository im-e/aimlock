#include "Game/Game.h"
#include "Game/Scenarios/ControlsphereScenario.h"
#include "Engine/Maths/MathUtils.h"
#include <cmath>
#include <cstdio>

static const char* SETTINGS_PATH = "settings.txt";
static const char* SCORES_PATH = "scores.txt";

//the rows on the settings screen, in order
enum SettingRow { ROW_SENSITIVITY, ROW_YAW, ROW_DPI, ROW_FOV, ROW_COUNT };

void Game::init(Window& win, Mesh* cubeMesh, Mesh* sphereMesh)
{
    settings.load(SETTINGS_PATH);
    camera.applySettings(settings);
    scores.load(SCORES_PATH);

    buildRoom(cubeMesh);

    //every scenario gets made once here, adding a new one is just another push_back
    scenarios.push_back(std::make_unique<ControlsphereScenario>(sphereMesh, camera.position));

    changeState(GameState::MainMenu, win);
}

void Game::buildRoom(Mesh* cubeMesh)
{
    //placeholder room out of stretched cubes
    room.clear();
    room.push_back(GameObject("Floor", cubeMesh, Vec3(0.0f, -0.1f, 0.0f), Vec3(30.0f, 0.1f, 30.0f),
        Vec3(0, 0, 0), Color(0.35f, 0.35f, 0.38f, 1.0f)));
    room.push_back(GameObject("BackWall", cubeMesh, Vec3(0.0f, 4.0f, 30.0f), Vec3(15.0f, 5.0f, 0.1f),
        Vec3(0, 0, 0), Color(0.45f, 0.45f, 0.5f, 1.0f)));
}

//---------------------------------------------------------------------------------------------
//state machine
//---------------------------------------------------------------------------------------------

void Game::update(Window& win, float dt)
{
    stateTimer += dt;

    switch (state)
    {
    case GameState::MainMenu:       updateMainMenu(win);       break;
    case GameState::ScenarioSelect: updateScenarioSelect(win); break;
    case GameState::Leaderboard:    updateLeaderboard(win);    break;
    case GameState::Settings:       updateSettings(win);       break;
    case GameState::Countdown:      updateCountdown(win);      break;
    case GameState::Playing:        updatePlaying(win, dt);    break;
    case GameState::Paused:         updatePaused(win);         break;
    case GameState::Results:        updateResults(win);        break;
    }
}

void Game::changeState(GameState next, Window& win)
{
    char msg[64];
    sprintf_s(msg, "Game: %s -> %s\n", toString(state), toString(next));
    OutputDebugStringA(msg);

    //settings and the leaderboard can be opened from more than one place, so remember where to go back to
    if (next == GameState::Settings || next == GameState::Leaderboard)
        returnState = state;

    state = next;
    stateTimer = 0.0f;

    //the mouse only gets captured while you're actually aiming, menus get a normal cursor
    bool aiming = (next == GameState::Countdown || next == GameState::Playing);
    win.allowMouseLock = aiming;
    win.lockMouse(aiming && win.focused);
}

void Game::updateMainMenu(Window& win)
{
    if (win.keyPressed(VK_RETURN) || win.keyPressed(VK_SPACE))
        changeState(GameState::ScenarioSelect, win);
    else if (win.keyPressed('S'))
        changeState(GameState::Settings, win);
    else if (win.keyPressed(VK_ESCAPE))
        quitRequested = true;
}

void Game::updateScenarioSelect(Window& win)
{
    int count = (int)scenarios.size();
    if (win.keyPressed(VK_UP))   selectedScenario = (selectedScenario + count - 1) % count;
    if (win.keyPressed(VK_DOWN)) selectedScenario = (selectedScenario + 1) % count;

    if (win.keyPressed(VK_RETURN) || win.keyPressed(VK_SPACE))
        startRun(win);
    else if (win.keyPressed('L'))
    {
        leaderboardScenario = selectedScenario;
        changeState(GameState::Leaderboard, win);
        printLeaderboard();
    }
    else if (win.keyPressed(VK_ESCAPE))
        changeState(GameState::MainMenu, win);
}

void Game::updateLeaderboard(Window& win)
{
    int count = (int)scenarios.size();
    int before = leaderboardScenario;
    if (win.keyPressed(VK_LEFT))  leaderboardScenario = (leaderboardScenario + count - 1) % count;
    if (win.keyPressed(VK_RIGHT)) leaderboardScenario = (leaderboardScenario + 1) % count;
    if (leaderboardScenario != before)
        printLeaderboard();

    if (win.keyPressed(VK_ESCAPE))
        changeState(returnState, win);
}

void Game::updateSettings(Window& win)
{
    if (win.keyPressed(VK_UP))   selectedSetting = (selectedSetting + ROW_COUNT - 1) % ROW_COUNT;
    if (win.keyPressed(VK_DOWN)) selectedSetting = (selectedSetting + 1) % ROW_COUNT;

    bool bigStep = win.keys[VK_SHIFT];
    if (win.keyPressed(VK_LEFT))  changeSetting(-1, bigStep);
    if (win.keyPressed(VK_RIGHT)) changeSetting(+1, bigStep);

    if (win.keyPressed(VK_ESCAPE))
        changeState(returnState, win);
}

void Game::changeSetting(int direction, bool bigStep)
{
    switch (selectedSetting)
    {
    case ROW_SENSITIVITY:
        settings.sensitivity += direction * (bigStep ? 0.5f : 0.05f);
        if (settings.sensitivity < 0.05f) settings.sensitivity = 0.05f;
        break;
    case ROW_YAW:
        settings.yawPreset = (settings.yawPreset + direction + YAW_PRESET_COUNT) % YAW_PRESET_COUNT;
        break;
    case ROW_DPI:
        settings.dpi += direction * (bigStep ? 400 : 50);
        if (settings.dpi < 100) settings.dpi = 100;
        break;
    case ROW_FOV:
        settings.horizontalFov += direction * (bigStep ? 10.0f : 1.0f);
        if (settings.horizontalFov < 60.0f) settings.horizontalFov = 60.0f;
        if (settings.horizontalFov > 130.0f) settings.horizontalFov = 130.0f;
        break;
    }
    camera.applySettings(settings); //takes effect straight away, even mid run from the pause menu
    settings.save(SETTINGS_PATH);   //save every change so nothing's lost if the game gets closed from here
}

void Game::updateCountdown(Window& win)
{
    camera.update(win); //you can already look around and line up while it counts down

    if (win.keyPressed(VK_ESCAPE))
        changeState(GameState::ScenarioSelect, win);
    else if (stateTimer >= COUNTDOWN_LENGTH)
        changeState(GameState::Playing, win);
}

void Game::updatePlaying(Window& win, float dt)
{
    //losing focus mid run pauses it, so alt+tabbing doesn't throw away the score
    if (win.keyPressed(VK_ESCAPE) || !win.focused)
    {
        changeState(GameState::Paused, win);
        return;
    }

    camera.update(win);

    AimInput input;
    input.origin = camera.position;
    input.direction = camera.forward();
    input.fireHeld = win.mouseButtons[0];
    input.clicks = win.leftClicks;
    currentScenario().update(dt, input);

    timeLeft -= dt;
    if (timeLeft <= 0.0f)
        finishRun(win);
}

void Game::updatePaused(Window& win)
{
    if (win.keyPressed(VK_ESCAPE))
        changeState(GameState::Playing, win); //timeLeft wasn't touched while paused, so it carries on
    else if (win.keyPressed('S'))
        changeState(GameState::Settings, win);
    else if (win.keyPressed('Q'))
        changeState(GameState::ScenarioSelect, win); //abandoned runs aren't saved
}

void Game::updateResults(Window& win)
{
    if (win.keyPressed(VK_RETURN) || win.keyPressed(VK_SPACE))
        startRun(win);
    else if (win.keyPressed('L'))
    {
        leaderboardScenario = selectedScenario;
        changeState(GameState::Leaderboard, win);
        printLeaderboard();
    }
    else if (win.keyPressed(VK_ESCAPE))
        changeState(GameState::ScenarioSelect, win);
}

//---------------------------------------------------------------------------------------------
//runs
//---------------------------------------------------------------------------------------------

void Game::startRun(Window& win)
{
    timeLeft = currentScenario().duration();
    newBest = false;
    lastRank = 0;
    camera.resetView();
    currentScenario().start();
    changeState(GameState::Countdown, win);
}

void Game::finishRun(Window& win)
{
    timeLeft = 0.0f;
    Scenario& scenario = currentScenario();

    ScoreEntry entry;
    entry.scenario = scenario.id();
    entry.score = scenario.score();
    entry.accuracy = scenario.accuracy();
    entry.cmPer360 = settings.cmPer360();
    entry.date = ScoreBoard::currentDate();

    //check before adding, otherwise the new run would always be compared against itself
    newBest = entry.score > scores.best(entry.scenario);
    lastRank = scores.add(entry);

    changeState(GameState::Results, win);
}

//until there's a ui the full leaderboard goes to the Output window, the title bar only fits the top few
void Game::printLeaderboard() const
{
    const Scenario& scenario = *scenarios[leaderboardScenario];
    std::vector<ScoreEntry> board = scores.top(scenario.id(), LEADERBOARD_SIZE);

    char line[160];
    sprintf_s(line, "---- %s leaderboard (%d runs) ----\n", scenario.name(), scores.runCount(scenario.id()));
    OutputDebugStringA(line);
    for (size_t i = 0; i < board.size(); i++)
    {
        const ScoreEntry& e = board[i];
        sprintf_s(line, "%2d. %6d   %3d%%   %5.1f cm/360   %s\n",
            (int)i + 1, e.score, (int)(e.accuracy * 100.0f + 0.5f), e.cmPer360, e.date.c_str());
        OutputDebugStringA(line);
    }
    if (board.empty())
        OutputDebugStringA("no runs yet\n");
}

//---------------------------------------------------------------------------------------------
//output
//---------------------------------------------------------------------------------------------

void Game::getDrawList(std::vector<const GameObject*>& out) const
{
    for (const GameObject& object : room)
        out.push_back(&object);

    //show the targets from the countdown on so you can see where it starts
    if (state == GameState::Countdown || state == GameState::Playing || state == GameState::Paused)
        currentScenario().getDrawList(out);
}

std::string Game::getStatusText() const
{
    std::string text = "Aimlock | ";
    char buffer[256];

    switch (state)
    {
    case GameState::MainMenu:
        text += "Main menu | Enter: play   S: settings   Esc: quit";
        break;

    case GameState::ScenarioSelect:
    {
        const Scenario& s = currentScenario();
        sprintf_s(buffer, "Scenario %d/%d: %s (%s) | best %d | Up/Down: choose   Enter: start   L: leaderboard   Esc: back",
            selectedScenario + 1, (int)scenarios.size(), s.name(), s.description(), scores.best(s.id()));
        text += buffer;
        break;
    }

    case GameState::Leaderboard:
    {
        const Scenario& s = *scenarios[leaderboardScenario];
        std::vector<ScoreEntry> board = scores.top(s.id(), 5);
        text += "Leaderboard: ";
        text += s.name();
        text += " |";
        for (size_t i = 0; i < board.size(); i++)
        {
            sprintf_s(buffer, "  %d. %d (%d%%)", (int)i + 1, board[i].score, (int)(board[i].accuracy * 100.0f + 0.5f));
            text += buffer;
        }
        if (board.empty())
            text += "  no runs yet";
        text += " | Left/Right: scenario   Esc: back";
        break;
    }

    case GameState::Settings:
    {
        //mark the selected row with > <
        auto row = [&](int index, const char* label, const std::string& value)
        {
            bool selected = (index == selectedSetting);
            return std::string(selected ? "> " : "") + label + " " + value + (selected ? " <" : "") + "   ";
        };
        char value[32];
        text += "Settings | ";
        sprintf_s(value, "%.2f", settings.sensitivity);   text += row(ROW_SENSITIVITY, "Sens", value);
        text += row(ROW_YAW, "Game", settings.yawName());
        sprintf_s(value, "%d", settings.dpi);             text += row(ROW_DPI, "DPI", value);
        sprintf_s(value, "%.0f", settings.horizontalFov); text += row(ROW_FOV, "FOV", value);
        sprintf_s(buffer, "= %.1f cm/360 | Up/Down Left/Right (Shift = bigger steps)   Esc: back", settings.cmPer360());
        text += buffer;
        break;
    }

    case GameState::Countdown:
        sprintf_s(buffer, "%s | Starting in %d", currentScenario().name(), (int)ceilf(COUNTDOWN_LENGTH - stateTimer));
        text += buffer;
        break;

    case GameState::Playing:
    {
        int seconds = (int)ceilf(timeLeft);
        sprintf_s(buffer, "%s | %d:%02d | %s", currentScenario().name(), seconds / 60, seconds % 60,
            currentScenario().statsText().c_str());
        text += buffer;
        break;
    }

    case GameState::Paused:
        text += "Paused | Esc: resume   S: settings   Q: quit run";
        break;

    case GameState::Results:
    {
        const Scenario& s = currentScenario();
        sprintf_s(buffer, "Results: %s | %s | #%d on the leaderboard%s | Enter: retry   L: leaderboard   Esc: back",
            s.name(), s.statsText().c_str(), lastRank, newBest ? " (new best!)" : "");
        text += buffer;
        break;
    }
    }
    return text;
}
