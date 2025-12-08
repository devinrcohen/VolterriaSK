//
//  VolterriaEngine.cpp
//  VolterriaCpp
//
//  Created by Devin R Cohen on 12/4/25.
//


// VolterriaEngine.cpp
#include "VolterriaEngine.hpp"

// You’ll adapt these calls to whatever Field actually exposes.
VolterriaEngine::VolterriaEngine()
    : field_(Settings{}) // maybe pass Settings, etc.
{
    // TODO: initialize grass, population, etc. as needed
}

void VolterriaEngine::ResetSimulation()
{
    field_.ResetFromSettings();
}

void VolterriaEngine::step(double dt) {
    field_.step(static_cast<float>(dt)); // or however your Field steps
}

std::vector<VCreatureSnapshot> VolterriaEngine::creatureSnapshot() const {
    std::vector<VCreatureSnapshot> out;
    const auto& creatures = field_.creatures(); // adjust to your API

    out.reserve(creatures.size());
    for (std::size_t i = 0; i < creatures.size(); ++i) {
        const Creature &c = creatures[i];
        if(!c.isAlive())
            continue;
        VCreatureSnapshot s;
        s.id = static_cast<int32_t>(i);
        s.x = c.position().x;
        s.y = c.position().y;
        s.role = (c.species() == SpeciesRole::Prey)
               ? VSpeciesRole::Prey
               : VSpeciesRole::Predator;
        s.sex = (c.sex() == Sex::Female ? VSex::Female : VSex::Male);
        s.alive = c.isAlive();
        out.push_back(s);
    }
    return out;
}

std::vector<VGrassPatchSnapshot> VolterriaEngine::grassSnapshot() const {
    std::vector<VGrassPatchSnapshot> out;
    const auto& patches = field_.grassPatches(); // adjust to your API

    out.reserve(patches.size());
    for (std::size_t i = 0; i < patches.size(); ++i) {
        const GrassPatch &g = patches[i];
        VGrassPatchSnapshot s;
        s.id = static_cast<int32_t>(i);
        s.x = g.center.x;
        s.y = g.center.y;
        s.radius = g.radius;
        s.normalizedHealth = g.healthNormalized();
        s.health = g.health;
        out.push_back(s);
    }
    return out;
}

//void VolterriaEngine::setDefaultPopulation(int prey, int pred) {
//    field_.resetPopulation(prey, pred); // you’ll hook this to whatever you have
//}


// Settings setters
void VolterriaEngine::SetDefaultPopulation(int prey, int pred)
{
    field_.SetNumPrey(prey);
    field_.SetNumPred(pred);
}

void VolterriaEngine::SetWorldDimensions(float width, float height)
{
    field_.SetFieldDimensions(width, height);
}

void VolterriaEngine::SetFieldWidth(float width)
{
    field_.SetFieldWidth(width);
}

void VolterriaEngine::SetFieldHeight(float height)
{
    field_.SetFieldHeight(height);
}
