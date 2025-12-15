//
//  creature.cpp
//  Volterria
//
//  Created by Devin R Cohen on 12/1/25.
//

#include "creature.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

//float lengthSquared(const Vec2& v)
//{
//    return v.x * v.x + v.y * v.y;
//}

Creature::Creature(const uint32_t id,
                   const Settings& settings,
                   SpeciesRole       role,
                   Sex               sex,
                   const Vec2&       initial_position,
                   const Vec2&       initial_velocity)
    : id_(id),
      species_(role),
      sex_(sex),
      position_(initial_position),
      velocity_(initial_velocity),
      rng_(std::random_device{}())
{
    if (species_ == SpeciesRole::Prey)
    {
        starve_rate_      = settings.prey_starve_rate;
        libido_rate_      = settings.prey_libido_rate;
        max_libido_       = settings.prey_libido_max;
        libido_threshold_ = settings.prey_libido_threshold;
        hunger_           = settings.prey_hunger_max;
    }
    else // Predator
    {
        starve_rate_      = settings.pred_starve_rate;
        libido_rate_      = settings.pred_libido_rate;
        max_libido_       = settings.predator_libido_max;
        libido_threshold_ = settings.pred_libido_threshold;
        hunger_           = settings.predator_hunger_max; // start reasonably full
    }
    
    std::uniform_real_distribution<float> libido_dist(0.0f, libido_threshold_);
    libido_ = libido_dist(rng_);
    
    // aging with variation
    float base_max_age =
        (species_ == SpeciesRole::Prey)
        ? settings.prey_max_age :
        settings.pred_max_age ;
    
    float frac = settings.age_variation_fraction;
    
    if(frac > 0.0f)
    {
        std::uniform_real_distribution<float> jitter(-frac, frac);
        max_age_ = base_max_age * (1 + jitter(rng_));
    }
    
    age_ = 0.0; // life begins
}

void Creature::update(float dt, const Settings& settings, const SteeringIntent& intent)
{
    if (!alive_) return;

    // Update hunger / libido timers.
    age_ += dt;
    if (age_ >= max_age_)
    {
        alive_ = false;
        return; // stop updating, Field will erase.
    }
    
    hunger_time_accumulator_ += dt;
    accel_time_accumulator_  += dt;
    
    // clear acceleration this tick
    acceleration_ = {0.f, 0.f};
    
    const bool veryHungry = (species_ == SpeciesRole::Predator) ?
        (hunger_ <= settings.pred_hunger_threshold) :
        (hunger_ <= settings.prey_hunger_threshold);
    
    const float libidoThresh = (species_ == SpeciesRole::Predator) ?
        (libido_ <= settings.pred_libido_threshold) :
        (libido_ <= settings.prey_libido_threshold);
    
    const bool wantsMate = libido_ >= libidoThresh;

    // hunt/mate priority
    if(veryHungry)
    {
        if (species_ == SpeciesRole::Prey && intent.has_target) forage(dt, settings, intent);
        else hunt(dt, settings, intent);
    } else if (wantsMate)
    {
        seekMate(dt, settings, intent);
    } else {
        wander(dt, settings);
    }
    
    // Starvation & libido growth.
    if (hunger_time_accumulator_ >= settings.hunger_tick_seconds)
    {
        hunger_time_accumulator_ -= settings.hunger_tick_seconds;

        // "Fullness" style hunger: predators lose fullness over time.
        hunger_ -= starve_rate_ * settings.hunger_tick_seconds;
        if (hunger_ < 0.0f)
        {
            hunger_ = 0.0f;
            alive_ = false;
            return; // no need to update() anymore, Field will erase.
        }
        
        // Libido grows for everyone up to their max.
        libido_ += libido_rate_ * settings.hunger_tick_seconds;
        if (libido_ > max_libido_)
            libido_ = max_libido_;
    }

    //wander(dt, settings); // this is where the wandering happens
    integrate(dt); // integrates velocity with dv (dv/dt)*dt, and position with dx (dx/dt)*dt
    applyWorldBounds(settings); // fix OOB objects AFTER repositioning to prevent clipping
}

void Creature::integrate(float dt)
{
    velocity_.x += acceleration_.x * dt;
    velocity_.y += acceleration_.y * dt;
    position_.x += velocity_.x * dt;
    position_.y += velocity_.y * dt;
}

void Creature::wander(float dt, const Settings& settings)
{
    if (accel_time_accumulator_ < settings.accel_tick)
        return;

    accel_time_accumulator_ -= settings.accel_tick;

    // Simple random walk: pick a new acceleration vector with components
    // in the range [-vmax, vmax].
    std::uniform_real_distribution<float> accel_dist(-settings.vmax, settings.vmax);
    acceleration_.x = accel_dist(rng_);
    acceleration_.y = accel_dist(rng_);

    // Clamp velocity to avoid spiraling out of control.
    const float vlen2 = lengthSquared(velocity_);
    const float vmax2 = settings.vmax * settings.vmax;
    if (vlen2 > vmax2 && vlen2 > 0.0f)
    {
        const float scale = settings.vmax / std::sqrt(vlen2);
        velocity_.x *= scale;
        velocity_.y *= scale;
    }
}

void Creature::applyWorldBounds(const Settings& settings)
{
    bool bounce_x = false;
    bool bounce_y = false;

    if (position_.x < settings.x_min)
    {
        position_.x = settings.x_min;
        bounce_x = true;
    }
    else if (position_.x > settings.x_max)
    {
        position_.x = settings.x_max;
        bounce_x = true;
    }

    if (position_.y < settings.y_min)
    {
        position_.y = settings.y_min;
        bounce_y = true;
    }
    else if (position_.y > settings.y_max)
    {
        position_.y = settings.y_max;
        bounce_y = true;
    }

    if (bounce_x)
    {
        velocity_.x      *= -1.0f;
        acceleration_.x  *= -1.0f;
    }
    if (bounce_y)
    {
        velocity_.y      *= -1.0f;
        acceleration_.y  *= -1.0f;
    }
}

void Creature::applySeekSteering(const Vec2& desiredDir, float desiredSpeed, float maxAccel)
{
    // desired velocity vector
    Vec2 desiredVel = { desiredDir.x * desiredSpeed, desiredDir.y * desiredSpeed };
    
    // steer (accelerate) toward target
    Vec2 steer = desiredVel - velocity_;
    
    // limit acceleration / turning force
    steer = clampMagnitude(steer, maxAccel);
    
    // accumulate acceleration vector
    acceleration_ += steer;
}

void Creature::onEat(const Settings& settings)
{
    if (species_ != SpeciesRole::Predator) return;

    // Eating increases fullness; cap at the configured max.
    hunger_ += settings.predator_hunger_max * 0.5f; // half a "tank" per meal
    if (hunger_ > settings.predator_hunger_max)
        hunger_ = settings.predator_hunger_max;
}

void Creature::onMate(const Settings& /*settings*/)
{
    // Reset libido when a creature successfully reproduces.
    libido_ = 0.0f;
}

void Creature::add_hunger(float amount, float max_hunger)
{
    hunger_ += amount;
    if (hunger_ > max_hunger) hunger_ = max_hunger;
}

//void Creature::setCellLocation(float cx, float cy)
//{
//    cell_x_ = cx;
//    cell_y_ = cy;
//}

void Creature::setHunger(float h)
{
    hunger_ = h;
}

bool Creature::shouldHunt(const Settings& settings)
{
    float hunger_threshold = (species_ == SpeciesRole::Prey) ? settings.prey_hunger_threshold : settings.pred_hunger_threshold;
    return hunger_ <= hunger_threshold;
}

bool Creature::shouldSeekMate(const Settings& settings)
{
    float libido_threshold = (species_ == SpeciesRole::Prey) ? settings.prey_libido_threshold : settings.pred_libido_threshold;
    return libido_ >= libido_threshold;
}


// TO-DO: merge hunt and forage as seekFood, do species checks inside, and generalize
// Get rid of species checks in update() and simply call seekFood()
void Creature::hunt(float dt, const Settings& settings, const SteeringIntent&  intent)
{
    // no one to hunt, just dilly dally for now
    // may remove, hunt should not receive a Prey any longer, but keep as a guard
    if (species_ != SpeciesRole::Predator)
    {
        wander(dt, settings);
        return;
    }
    
    if(!intent.has_target)
    {
        wander(dt, settings);
        return;
    }
    
    // hunting urgency - map 0 to 1
    float urgency = 0.f;
    if (species_ == SpeciesRole::Predator)
    {
        const float hungry = settings.pred_hunger_threshold;
        urgency = (hungry - hunger_) / hungry;
        urgency = std::clamp(urgency, 0.f, 1.f);
    }
    
    //const float baseSpeed = 100.f; // verify if check for species
    const float desiredSpeed = settings.predator_hunt_speed_min + (settings.predator_hunt_speed_max - settings.predator_hunt_speed_min) * urgency;
    
    const float maxAccel = settings.predator_hunt_max_accel; // make const pred_max_accel in settings
    Vec2 dir = normalize(intent.desired_dir);
    applySeekSteering(dir, desiredSpeed, maxAccel);
}

void Creature::forage(float dt, const Settings& settings, const SteeringIntent& intent)
{
    if (species_ != SpeciesRole::Prey)
    {
        wander(dt, settings);
        return;
    }
    
    if(!intent.has_target)
    {
        wander(dt, settings);
        return;
    }
    
    // hunting urgency - map 0 to 1
    float urgency = 0.f;
    if (species_ == SpeciesRole::Prey)
    {
        const float hungry = settings.prey_hunger_threshold;
        urgency = (hungry - hunger_) / hungry;
        urgency = std::clamp(urgency, 0.f, 1.f);
    }
    
    //const float baseSpeed = 100.f; // verify if check for species
    const float desiredSpeed = settings.prey_forage_speed_min + (settings.prey_forage_speed_max - settings.prey_forage_speed_min) * urgency;
    
    const float maxAccel = settings.prey_forage_max_accel;
    Vec2 dir = normalize(intent.desired_dir);
    applySeekSteering(dir, desiredSpeed, maxAccel);
}

void Creature::seekMate(float dt, const Settings& settings, const SteeringIntent& intent)
{
    // no mate found, dilly dally
    if(!intent.has_target)
    {
        wander(dt, settings);
        return;
    }
    
    // Libido urgency
    const float libidoThresh = (species_ == SpeciesRole::Prey) ? settings.prey_libido_threshold : settings.pred_libido_threshold;
    float drive = (libido_ - libidoThresh) / std::max(1e-6f, (1.0f - libidoThresh));
    drive = std::clamp(drive, 0.f, 1.f);
    
    // mate seeking is less "full speed chase" than hunting
    const float speedMax = (species_ == SpeciesRole::Prey)
        ? settings.prey_mate_speed_max
        : settings.predator_mate_speed_max;

    const float speedMin = (species_ == SpeciesRole::Prey)
        ? settings.prey_mate_speed_min
        : settings.predator_mate_speed_min;

    const float accelMax = (species_ == SpeciesRole::Prey)
        ? settings.prey_mate_max_accel
        : settings.predator_mate_max_accel;

    // drive is 0..1 already
    const float desiredSpeed = speedMin + (speedMax - speedMin) * drive; // CHANGE
    const float maxAccel     = accelMax;
    
    Vec2 dir = normalize(intent.desired_dir);
    applySeekSteering(dir, desiredSpeed, maxAccel);
}

