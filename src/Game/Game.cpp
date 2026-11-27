#include "Game/Game.h"
#include "Engine/Maths/MathUtils.h"
#include <cmath>
#include <cstdio>

void Game::init(Window& win, Mesh* cubeMesh, Mesh* sphereMesh)
{
    rng.seed(std::random_device{}());

    buildRoom(cubeMesh);

    float r = TARGET_RADIUS; //the sphere mesh has radius 1, so scale = radius
    target = GameObject("Target", sphereMesh, Vec3(0, 0, 0), Vec3(r, r, r), Vec3(0, 0, 0), Color(0.9f, 0.25f, 0.2f, 1.0f));

    changeState(GameState::MainMenu, win);
}

void Game::buildRoom(Mesh* cubeMesh)
{
    //placeholder room out of stretched cubes
    room.clear();
    room.push_back(GameObject("Floor", cubeMesh, Vec3(0.0f, -0.1f, 0.0f), Vec3(30.0f, 0.1f, 30.0f),
        Vec3(0, 0, 0), Color(0.35f, 0.35f, 0.38f, 1.0f)));
    room.push_back(GameObject("BackWall", cubeMesh, Vec3(0.0f, 4.0f, 14.0f), Vec3(15.0f, 5.0f, 0.1f),
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
    case GameState::MainMenu:  updateMainMenu(win);     break;
    case GameState::Countdown: updateCountdown(win);    break;
    case GameState::Playing:   updatePlaying(win, dt);  break;
    case GameState::Paused:    updatePaused(win);       break;
    case GameState::Results:   updateResults(win);      break;
    }
}

void Game::changeState(GameState next, Window& win)
{
    char msg[64];
    sprintf_s(msg, "Game: %s -> %s\n", toString(state), toString(next));
    OutputDebugStringA(msg);

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
        startRun(win);
    else if (win.keyPressed(VK_ESCAPE))
        quitRequested = true;
}

void Game::updateCountdown(Window& win)
{
    camera.update(win); //you can already look around and line up while it counts down

    if (win.keyPressed(VK_ESCAPE))
        changeState(GameState::MainMenu, win);
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

    //go through every click this frame, not just "is the button down", otherwise fast clicks get dropped
    for (int i = 0; i < win.leftClicks; i++)
        shoot();

    timeLeft -= dt;
    if (timeLeft <= 0.0f)
        finishRun(win);
}

void Game::updatePaused(Window& win)
{
    if (win.keyPressed(VK_ESCAPE))
        changeState(GameState::Playing, win); //timeLeft wasn't touched while paused, so it carries on
    else if (win.keyPressed('Q'))
        changeState(GameState::MainMenu, win);
}

void Game::updateResults(Window& win)
{
    if (win.keyPressed(VK_RETURN) || win.keyPressed(VK_SPACE))
        startRun(win);
    else if (win.keyPressed(VK_ESCAPE))
        changeState(GameState::MainMenu, win);
}

//---------------------------------------------------------------------------------------------
//gameplay
//---------------------------------------------------------------------------------------------

void Game::startRun(Window& win)
{
    stats = RunStats();
    timeLeft = RUN_LENGTH;
    newBest = false;
    camera.resetView();
    spawnTarget();
    changeState(GameState::Countdown, win);
}

void Game::finishRun(Window& win)
{
    timeLeft = 0.0f;
    if (stats.score() > bestScore)
    {
        bestScore = stats.score();
        newBest = true;
    }
    changeState(GameState::Results, win);
}

void Game::shoot()
{
    stats.shots++;

    //hitscan: fire a ray straight out of the middle of the screen
    float t;
    if (MathUtils::raySphere(camera.position, camera.forward(), target.position, TARGET_RADIUS, t))
    {
        stats.hits++;
        spawnTarget();
    }
}

void Game::spawnTarget()
{
    //somewhere in front of the player: up to ~40 degrees either side, a few metres away, roughly head height
    float angle = randomRange(-0.7f, 0.7f);
    float distance = randomRange(7.0f, 10.0f);
    float height = randomRange(1.0f, 3.0f);
    target.position = Vec3(sinf(angle) * distance, height, cosf(angle) * distance);
}

float Game::randomRange(float lo, float hi)
{
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(rng);
}

//---------------------------------------------------------------------------------------------
//output
//---------------------------------------------------------------------------------------------

void Game::getDrawList(std::vector<const GameObject*>& out) const
{
    for (const GameObject& object : room)
        out.push_back(&object);

    if (state == GameState::Playing || state == GameState::Paused)
        out.push_back(&target);
}

std::string Game::getStatusText() const
{
    char text[256];
    int accuracy = (int)(stats.accuracy() * 100.0f + 0.5f);

    switch (state)
    {
    case GameState::MainMenu:
        sprintf_s(text, "Aimlock | Main menu | best %d | Enter: start   Esc: quit", bestScore);
        break;
    case GameState::Countdown:
        sprintf_s(text, "Aimlock | Starting in %d", (int)ceilf(COUNTDOWN_LENGTH - stateTimer));
        break;
    case GameState::Playing:
    {
        int seconds = (int)ceilf(timeLeft);
        sprintf_s(text, "Aimlock | %d:%02d | kills %d | accuracy %d%%", seconds / 60, seconds % 60, stats.hits, accuracy);
        break;
    }
    case GameState::Paused:
        sprintf_s(text, "Aimlock | Paused | Esc: resume   Q: quit to menu");
        break;
    case GameState::Results:
        sprintf_s(text, "Aimlock | Results | score %d | accuracy %d%% | best %d%s | Enter: retry   Esc: menu",
            stats.score(), accuracy, bestScore, newBest ? " (new best!)" : "");
        break;
    default:
        sprintf_s(text, "Aimlock");
        break;
    }
    return text;
}
