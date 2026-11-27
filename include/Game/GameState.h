#pragma once

//every screen the game can be on. Game::update() runs whichever one is current
//
//  MainMenu        Enter: -> ScenarioSelect      S: -> Settings          Esc: quit the game
//  ScenarioSelect  Up/Down: pick a scenario      Enter: -> Countdown     L: -> Leaderboard     Esc: -> MainMenu
//  Leaderboard     Left/Right: other scenario                             Esc: back to where you came from
//  Settings        Up/Down: pick a setting       Left/Right: change it   Esc: back (saved as you go)
//  Countdown       after 3s: -> Playing                                   Esc: -> ScenarioSelect
//  Playing         when the timer runs out: -> Results                    Esc or alt+tab: -> Paused
//  Paused          Esc: back to Playing          S: -> Settings          Q: -> ScenarioSelect
//  Results         Enter: retry (-> Countdown)   L: -> Leaderboard       Esc: -> ScenarioSelect
enum class GameState
{
    MainMenu,
    ScenarioSelect,
    Leaderboard,
    Settings,
    Countdown,
    Playing,
    Paused,
    Results
};

inline const char* toString(GameState state)
{
    switch (state)
    {
    case GameState::MainMenu:       return "Main menu";
    case GameState::ScenarioSelect: return "Scenario select";
    case GameState::Leaderboard:    return "Leaderboard";
    case GameState::Settings:       return "Settings";
    case GameState::Countdown:      return "Countdown";
    case GameState::Playing:        return "Playing";
    case GameState::Paused:         return "Paused";
    case GameState::Results:        return "Results";
    }
    return "Unknown";
}
