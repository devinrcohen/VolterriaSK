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
    
    void update(float dt, float regrow_rate)
    {
        health += regrow_rate * dt;
        if (health > max_health) health = max_health;
        else if (health < 0.0) health = 0.0f;
    }
    
    float healthNormalized() const
    {
        //return max_health > 0.0f ? (health / max_health) : 0.0f;
        //if (max_health <= 0.0f) return 0.0f;
        //float h = health/max_health;
        //if (h < 0.0f) h = 0.0f;
        //if (h > 1.0f) h = 1.0f;
        //return h;
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
    //Vec2 center = {0.0f, 0.0f};
    // only store the indices to each object, since Field already contains
    // the master vectors for all creatures and patches
    std::vector<int> cell_creatures_indices;
    std::vector<int> cell_grassPatches_indices;
    //float radius;
    
//    FieldCell();
//    FieldCell(float cx, float cy, Settings& settings)
//    {
//        center = {cx, cy};
//        radius = settings.interaction_radius;
//    }
//    void Clear()
//    {
//        cell_creatures_indices.clear();
//        cell_grassPatches_indices.clear();
//    }

};

struct CreatureState
{
    float        x;
    float        y;
    SpeciesRole  role;
    Sex          sex;
    bool         alive;
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
    std::vector<Creature> creatures_;
    std::vector<GrassPatch> grassPatches_;
    std::vector<std::vector<FieldCell>> field_cells_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> x_dist_;
    std::uniform_real_distribution<float> y_dist_;
    std::uniform_real_distribution<float> v_dist_;
    std::bernoulli_distribution prey_female_dist_;
    std::bernoulli_distribution pred_female_dist_;

    void initializeFieldCells();
    void initializeCreatures();
    void handleGrass(float dt);
    void handleInteractions();
    void initializeGrass();
    void pairCheck();
    template <typename T> void ComputeCellLocation(const T&, int*, int*);
    
    // recalculated after determining # of cells
    int actual_cell_width_;
    int actual_cell_height_;
};


