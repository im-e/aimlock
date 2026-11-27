#include "Game/Scenarios/ControlsphereScenario.h"
#include "Engine/Maths/MathUtils.h"
#include <cmath>
#include <cstdio>

static const Color IDLE_COLOUR = Color(0.9f, 0.25f, 0.2f, 1.0f);
static const Color TRACKED_COLOUR = Color(1.0f, 0.75f, 0.3f, 1.0f); //lights up while you're on it

ControlsphereScenario::ControlsphereScenario(Mesh* sphereMesh, Vec3 playerPosition)
    : centre(playerPosition)
{
    float r = RADIUS; //how big the radius of the sphere will be
    target = GameObject("Controlsphere", sphereMesh, Vec3(0, 0, 0), Vec3(r, r, r), Vec3(0, 0, 0), IDLE_COLOUR);
}

void ControlsphereScenario::start()
{
    //start straight in front of the player 
    angle = 0.0f;
    distance = targetDistance = 16.0f;
    height = centre.y;
    verticalSpeed = 0.0f;
    jumpTimer = randomRange(MIN_JUMP_GAP, MAX_JUMP_GAP);
    speed = 0.0f;
    pickNewMovement();

    timeOnTarget = 0.0;
    timeFiring = 0.0;
    onTarget = false;

    move(0.0f); //puts the target in its starting position
}

//picks a new movement for the sphere
void ControlsphereScenario::pickNewMovement() 
{
    //change speed
    float newSpeed = randomRange(MIN_SPEED, MAX_SPEED);

    //find the direction
    float direction = (speed < 0.0f) ? -1.0f : 1.0f;
    //if it hits chance threshold, change direction
    if (chance(0.7f))
        direction = -direction;

    //target speed then becomes the new speed with the direction change
    targetSpeed = newSpeed * direction;
    //change target distance to a new range
    targetDistance = randomRange(MIN_DISTANCE, MAX_DISTANCE);
    //set the change timer to a random range before next movement change
    changeTimer = randomRange(0.6f, 1.6f);
}

void ControlsphereScenario::update(float dt, const AimInput& input)
{
    //check hits first before moving so that its what the player was actually looking at the frame before
    onTarget = false;
    if (input.fireHeld) //if fire held
    {
        timeFiring += dt; //update time firing
        float t;
        if (MathUtils::raySphere(input.origin, input.direction, target.position, RADIUS, t))
        {
            //if hit 
            onTarget = true; 
            timeOnTarget += dt;
        }
    }
    if (onTarget) target.colour = TRACKED_COLOUR;
    else target.colour = IDLE_COLOUR;
    
    //reduce the change movement timer
    changeTimer -= dt;
    if (changeTimer <= 0.0f)
        pickNewMovement();

    move(dt); //move object by delta time
}

void ControlsphereScenario::move(float dt)
{
    //ease the speed towards the new one rather
    float speedStep = ACCELERATION * dt;
    if (speed < targetSpeed) speed = fminf(speed + speedStep, targetSpeed);
    else speed = fmaxf(speed - speedStep, targetSpeed);

    distance += (targetDistance - distance) * fminf(1.0f, dt * 1.5f);

    //speed is in metres per second along the ring, so divide by the distance to get radians per second
    //that way it looks just as fast whether it's near or far
    angle += (speed / distance) * dt;

    //gravity is always pulling it down
    verticalSpeed -= GRAVITY * dt;

    //pop up at random times, whether it's on the floor or still in the air
    //a timer rather than chance() every frame, because a per frame chance jumps way more often at high fps
    jumpTimer -= dt;
    if (jumpTimer <= 0.0f)
    {
        verticalSpeed = chance(0.5f) ? randomRange(3.5f, 5.5f) : randomRange(1.5f, 2.5f); //big jump or small hop
        jumpTimer = randomRange(MIN_JUMP_GAP, MAX_JUMP_GAP);
    }

    height += verticalSpeed * dt;
    if (height <= FLOOR_HEIGHT)
    {
        //land and stay on the floor until the next jump
        height = FLOOR_HEIGHT;
        verticalSpeed = 0.0f;
    }
    else if (height >= MAX_HEIGHT)
    {
        //hit the ceiling, so start falling straight away
        height = MAX_HEIGHT;
        verticalSpeed = fminf(verticalSpeed, 0.0f);
    }


    target.position = Vec3(centre.x + sinf(angle) * distance, height, centre.z + cosf(angle) * distance);
}

void ControlsphereScenario::getDrawList(std::vector<const GameObject*>& out) const
{
    out.push_back(&target);
}

int ControlsphereScenario::score() const
{
    return (int)(timeOnTarget * 100.0 + 0.5);
}

float ControlsphereScenario::accuracy() const
{
    return timeFiring > 0.0 ? (float)(timeOnTarget / timeFiring) : 0.0f;
}

std::string ControlsphereScenario::statsText() const
{
    char text[128];
    sprintf_s(text, "score %d | on target %.1fs | accuracy %d%%",
        score(), timeOnTarget, (int)(accuracy() * 100.0f + 0.5f));
    return text;
}
