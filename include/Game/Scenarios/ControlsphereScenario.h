#pragma once
#include "Game/Scenario.h"
#include "Engine/Graphics/Mesh.h"

//tracking scenario: one sphere circles all the way around you, speeding up, slowing down,
//changing direction and distance, and hopping up and down
//hold fire and keep the crosshair on it. score = seconds on target x 100, so 6000 is perfect
class ControlsphereScenario : public Scenario
{
public:
    ControlsphereScenario(Mesh* sphereMesh, Vec3 playerPosition);

    const char* id() const override { return "controlsphere"; }
    const char* name() const override { return "Controlsphere"; }
    const char* description() const override { return "Tracking - hold fire and stay on the sphere"; }

    void start() override;
    void update(float dt, const AimInput& input) override;
    void getDrawList(std::vector<const GameObject*>& out) const override;

    int score() const override;
    float accuracy() const override;
    std::string statsText() const override;

private:
    //tuning, all in metres and seconds
    static constexpr float RADIUS = .5f; //size (radius) of the target sphere
    static constexpr float MIN_SPEED = 2.0f;  //how fast it moves round you
    static constexpr float MAX_SPEED = 4.0f;   
    static constexpr float ACCELERATION = 4.0f;  //how quickly it gets to a new speed
    static constexpr float MIN_DISTANCE = 16.0f; //how far away the sphere is from you
    static constexpr float MAX_DISTANCE = 25.0f; 
    static constexpr float FLOOR_HEIGHT = 0.5f;  //lowest the centre of the sphere goes
    static constexpr float MAX_HEIGHT = 6.0f;    //highest point the sphere goes
    static constexpr float GRAVITY = 3.0f;  //gravity speed
    static constexpr float MIN_JUMP_GAP = 1.0f;  //seconds between random jumps
    static constexpr float MAX_JUMP_GAP = 4.0f;

    GameObject target;
    Vec3 centre; //the player, the sphere moves in a ring around this

    //movement, kept as an angle round the player rather than a position
    //so it never gets too close or drifts off into the distance
    float angle = 0.0f;
    float speed = 0.0f;       //current speed along the ring, negative = clockwise
    float targetSpeed = 0.0f; //what it's easing towards
    float distance = 0.0f;    //distance from the player
    float targetDistance = 0.0f; //target distance its heading towards
    float height = 0.0f;    //height of the sphere
    float verticalSpeed = 0.0f; //vertical speed
    float jumpTimer = 0.0f;   //counts down to the next jump, which can happen on the floor or in the air
    float changeTimer = 0.0f; //counts down to the next random change

    //stats
    double timeOnTarget = 0.0; //double so 60 seconds of tiny frame times add up to exactly 6000
    double timeFiring = 0.0; //time player spent firing
    bool onTarget = false; //if the player is on target that frame

    void pickNewMovement();
    void move(float dt);
};
