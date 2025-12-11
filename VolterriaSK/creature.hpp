//
//  creature.hpp
//  Volterria
//
//  Created by Devin R Cohen on 12/1/25.
//

#pragma once

// Simulation-only Creature type.
// All rendering concerns (sprites, textures, SFML types) have been removed.
// The only responsibility here is updating position / velocity / acceleration
// and internal state like hunger and libido.

#include <random>
#include "constants.hpp"
#include "settings.hpp"

// declaring as a struct instead of class makes all members public by default
// equivalent to class Vec2{ public: ... };
struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float xx, float yy) : x(xx), y(yy) {}

    // Compound operators - inside the struct because they modify the struct itself
    // Vec2 addition (compound)
    // Vec2& is the return type
    // const Vec2& is the parameter, avoids copying "other" into function
    // and still allows binding to temporaries and lvalues
    // can do explicit statements like a += Vec2{3, 4} (a temporary).
    // without const, that would be invalid
    Vec2& operator+=(const Vec2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    // Vec2 subtraction (compound)
    Vec2& operator-=(const Vec2& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
};

// Binary operators are non-member functions and reuse the compound functions
// Doesn't require left operand to be a Vec2, just needs to be explicitly convertible to one
// Such as if we need to add Vec2i or Vec2f at a later time and they need to be addable
inline Vec2 operator+(Vec2 lhs, const Vec2& rhs)
{
    lhs += rhs;
    return lhs;
}

inline Vec2 operator-(Vec2 lhs, const Vec2& rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Vec2 operator*(const Vec2& v, float s)
{
    return Vec2(v.x * s, v.y * s);
}

inline Vec2 operator*(float s, const Vec2& v)
{
    return Vec2(v.x * s, v.y * s);
}

// Squared length helper used for distance checks.
float lengthSquared(const Vec2& v);

class Creature
{
public:
    Creature(const Settings& settings,
             SpeciesRole       role,
             Sex               sex,
             const Vec2&       initial_position,
             const Vec2&       initial_velocity);

    // Per-frame update entry point used by Field.
    void update(float dt, const Settings& settings);
    void setHunger(float);
    void setCellLocation(float, float);

    // Simple getters used by the Field / snapshot layer.
    SpeciesRole species()     const noexcept { return species_; }
    Sex sex()                 const noexcept { return sex_; }
    const Vec2& position()    const noexcept { return position_; }
    const Vec2& velocity()    const noexcept { return velocity_; }
    bool        isAlive()     const noexcept { return alive_;   }
    float       hunger()      const noexcept { return hunger_;  }
    float       libido()      const noexcept { return libido_;  }
    float       libidoThreshold() const noexcept { return libido_threshold_; }
//    int cx() const noexcept { return cell_x_; }
//    int cy() const noexcept { return cell_y_; }

    void kill() noexcept { alive_ = false; }

    // Hooks called by the Field when this creature eats prey or mates.
    void onEat(const Settings& settings);
    void onMate(const Settings& settings);
    void add_hunger(float amount, float max_hunger);

private:
    void integrate(float dt);
    void wander(float dt, const Settings& settings);
    void applyWorldBounds(const Settings& settings);

    SpeciesRole species_;
    Sex sex_;

    Vec2 position_;
    Vec2 velocity_;
    Vec2 acceleration_;

    bool  alive_            = true;
    float hunger_           = 0.0f;  // "fullness" style hunger: 0 = starving
    float libido_           = 0.0f;
    float starve_rate_      = 1.0f;  // units of fullness lost per second
    float libido_rate_      = 1.0f;  // units of libido gained per second
    float max_libido_       = 10.0f;
    float libido_threshold_ = 3.0f;  // must exceed this before mating

    float accel_time_accumulator_  = 0.0f;
    float hunger_time_accumulator_ = 0.0f;
    
    // what cell is it in?
//    int cell_x_, cell_y_;
    
    // Aging state
    float age_ = 0.0f; // current age in simulation seconds
    float max_age_ = 60.f; // assigned in constructorfrom Settings

    std::mt19937 rng_;
};


