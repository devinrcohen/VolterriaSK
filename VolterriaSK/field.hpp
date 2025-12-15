//
//  field.hpp
//  Volterria
//
//  Created by Devin R Cohen on 12/1/25.
//

#pragma once

// Field: owns the collection of Creatures and implements the
// interaction rules (predator eats prey, mating, etc.).
// This is the main entry point that your Swift / Obj-C bridge will
// talk to: create a Field, call step(), then call snapshot().

#include <vector>
#include <random>
#include <chrono>
#include <iostream>

#include "constants.hpp"
#include "settings.hpp"
#include "creature.hpp"


// POD snapshot used for bridging out to Swift / C++.
struct GrassPatch
{
    Vec2 center;
    float radius = 0.0f;
    float health = 0.0f;
    float max_health = 10.0f;
    
    void setCellLocation(float, float);
    
    int cx() const noexcept { return cell_x_; } // may not use
    int cy() const noexcept { return cell_y_; } // may not use
    
    Vec2 position() const
    {
        return center;
    }
    
    void update(float dt, float regrow_rate)
    {
        health += regrow_rate * dt;
        if (health > max_health) health = max_health;
        else if (health < 0.0) health = 0.0f;
    }
    
    float healthNormalized() const
    {
        return health/max_health;
    }
    
    bool contains(const Vec2& p) const
    {
        Vec2 d = p - center;
        return (d.x*d.x + d.y*d.y) <= radius * radius; // distance-from-center^2 <= grass radius^2
    }
    // what cell is it in?
    int cell_x_, cell_y_;  // may not use
};

struct FieldCell
{
    std::vector<int> cell_creatures_indices;
    std::vector<int> cell_grassPatches_indices;
};

struct CreatureState
{
    float        x;
    float        y;
    SpeciesRole  role;
    Sex          sex;
    bool         alive;
};


enum class IntentType { None, Hunt, SeekMate, SeekFood };
enum class DistType { Uniform, Normal };

struct Perception
{
    bool hasFoodTarget = false;
    Vec2 foodDirection{};
    bool hasMateTarget = false;
    Vec2 mateDirection{};
};

class Field
{
public:
    explicit Field(const Settings& settings);
    void ResetFromSettings();
    // Advance the simulation by dt seconds.
    void step(float dt);
    
    // Lightweight snapshot used by the UI layer. This intentionally
    // returns a POD-only copy so it is easy to bridge into Swift.
    std::vector<CreatureState> snapshot() const;
    
    const Settings&                            settings()  const noexcept { return settings_;  }
    const std::vector<Creature>&               creatures() const noexcept { return creatures_; }
    const std::vector<GrassPatch>&             grassPatches() const noexcept { return grassPatches_; }
    const std::vector<std::vector<FieldCell>>& field_cells() const noexcept { return field_cells_; }
    const int elapsedSimSeconds() const noexcept { return elapsed_sim_seconds_; }
    const int pairChecksPerFrame() const noexcept { return pair_checks_per_frame_; }
    const float framesPerSecond() const noexcept { return 1.0f / elapsed_sec_; }
    
    // Public settings-setters (lol that won't confuse anyone)
    void SetNumPrey(int);
    void SetNumPred(int);
    void SetPreyMaxAge(float);
    void SetPredMaxAge(float);
    void SetFieldDimensions(float, float);
    void SetFieldWidth(float);
    void SetFieldHeight(float);
    void announceCreature(Creature*);
    
private:
    Settings settings_;
    uint32_t next_creature_id = 1; // monotonic counter to prevent indexing sync issues
    
    std::vector<Creature> creatures_;
    std::vector<GrassPatch> grassPatches_;
    std::vector<std::vector<FieldCell>> field_cells_;
    std::vector<SteeringIntent> intents_;
    std::mt19937 rng_;
    
    std::uniform_real_distribution<float> x_dist_uniform_;
    std::uniform_real_distribution<float> y_dist_uniform_;
    std::uniform_real_distribution<float> v_dist_uniform_;
    std::normal_distribution<float> x_prey_spawn_normal_;
    std::normal_distribution<float> y_prey_spawn_normal_;
    std::normal_distribution<float> x_predator_spawn_normal_;
    std::normal_distribution<float> y_predator_spawn_normal_;
    std::bernoulli_distribution prey_female_dist_;
    std::bernoulli_distribution pred_female_dist_;

    void initializeFieldCells();
    void initializeCreatures(DistType);
    void handleGrass(float dt);
    void handleInteractions();
    void initializeGrass();
    void pairCheck();
    void computeIntents();
    template <typename T> void ComputeCellLocation(const T&, int*, int*);
    
    // recalculated after determining # of cells
    int actual_cell_width_;
    int actual_cell_height_;
    
    int elapsed_sim_seconds_ = 0;
    int pair_checks_per_frame_ = 0;
    
    std::chrono::steady_clock::time_point start_time_;
    float elapsed_sec_;
};


