#pragma once
#include "Game/GameObject.h"
#include "Engine/Maths/Vec3.h"
#include <vector>
#include <string>
#include <random>

//what the scenario needs to know about the player each frame
struct AimInput
{
    Vec3 origin; //camera position
    Vec3 direction; //where the crosshair points (normalised)
    bool fireHeld; //left mouse held down, for tracking
    int clicks; //left clicks this frame, for clicking scenarios
};

//base class for every scenario
//scenarios move its targets, decide what counts as a hit, and say what the score is
class Scenario
{
public:
    Scenario() : rng(std::random_device{}()) {}
    virtual ~Scenario() = default;

    virtual const char* id() const = 0; //no spaces, used in scores.txt
    virtual const char* name() const = 0; //shown to the player
    virtual const char* description() const = 0;
    virtual float duration() const { return 60.0f; }

    virtual void start() = 0; //reset everything for a new run
    virtual void update(float dt, const AimInput& input) = 0; //only called while Playing
    virtual void getDrawList(std::vector<const GameObject*>& out) const = 0; //gets the list of game objects to draw each frame

    virtual int score() const = 0; //calculates the score once scenario ends
    virtual float accuracy() const = 0; //calculates the accuracy of the player (0-1)
    virtual std::string statsText() const = 0; //one line for the hud, e.g. "kills 12 | accuracy 85%"

protected:
    std::mt19937 rng; //rng seed

    float randomRange(float lo, float hi) //helper for finding a random value between a range
    {
        std::uniform_real_distribution<float> dist(lo, hi);
        return dist(rng);
    }

    bool chance(float probability) //helper for a true/false action based on a probability
    { 
        return randomRange(0.0f, 1.0f) < probability; 
    }
};
