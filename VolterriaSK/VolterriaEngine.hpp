//
//  VolterriaEngine.hpp
//  VolterriaCpp
//
//  Created by Devin R Cohen on 12/4/25.
//

#pragma once

#include <vector>
#include <cstdint>
#include "field.hpp"
#include "creature.hpp"

// Match your existing Swift roles
enum class VSpeciesRole : int {
    Prey = 0, // Swift: .Prey
    Predator = 1, // Swift: .Predator
};

enum class VSex : int {
    Male = 0, // Swift: .Male
    Female = 1, // Swift: .Female
};

struct VGrassPatchSnapshot {
    int32_t id;
    float x;
    float y;
    float radius;
    float normalizedHealth; // 0.0 – 1.0
    float health;
};

struct VCreatureSnapshot {
    int32_t id;
    float x;
    float y;
    VSpeciesRole role;
    VSex sex;
    bool alive;
};

// High-level C++ engine wrapper around Field
class VolterriaEngine {
public:
    VolterriaEngine();
    void ResetSimulation();

    // Keep the old name so Swift calls can stay almost identical
    void step(double dt);

    // Return std::vector – Swift can interop with std::vector specializations.:contentReference[oaicite:3]{index=3}
    std::vector<VCreatureSnapshot> creatureSnapshot() const;
    std::vector<VGrassPatchSnapshot> grassSnapshot() const;

    // Settings setters
    void SetDefaultPopulation(int prey, int pred);
    void SetWorldDimensions(float width, float height);
    void SetFieldWidth(float width);
    void SetFieldHeight(float height);

    // Settings getters
    std::size_t defaultPreyPop() const noexcept { return field_.settings().numprey; }
    std::size_t defaultPredatorPop() const noexcept { return field_.settings().numpred; }
    float maxPreyAge() const noexcept { return field_.settings().prey_max_age; }
    float maxPredatorAge() const { return field_.settings().pred_max_age; }
    float worldXMin() const noexcept { return field_.settings().x_min; }
    float worldXMax() const noexcept { return field_.settings().x_max; }
    float worldYMin() const noexcept { return field_.settings().y_min; }
    float worldYMax() const noexcept { return field_.settings().y_max; }
    float worldWidth() const noexcept { return worldXMax() - worldXMin(); }
    float worldHeight() const noexcept { return worldYMax() - worldYMin(); }    

private:
    Field field_;
};
