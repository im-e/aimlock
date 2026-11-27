#pragma once

//every screen the game can be on. Game::update() runs whichever one is current
//
//  MainMenu   Enter: start a run (-> Countdown)    Esc: quit the game
//  Countdown  after 3s: -> Playing                 Esc: back to the menu
//  Playing    after 60s: -> Results                Esc or alt+tab: -> Paused
//  Paused     Esc: back to Playing                 Q: quit to the menu
//  Results    Enter: retry (-> Countdown)          Esc: back to the menu
//
//scenario select and settings get added once the ui renderer can draw them
enum class GameState
{
    MainMenu,
    Countdown,
    Playing,
    Paused,
    Results
};

inline const char* toString(GameState state)
{
    switch (state)
    {
    case GameState::MainMenu:  return "Main menu";
    case GameState::Countdown: return "Countdown";
    case GameState::Playing:   return "Playing";
    case GameState::Paused:    return "Paused";
    case GameState::Results:   return "Results";
    }
    return "Unknown";
}
