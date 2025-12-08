//
//  field.cpp
//  Volterria
//
//  Created by Devin R Cohen on 12/1/25.
//

#include "field.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstring>

namespace
{
    float distanceSquared(const Vec2& a, const Vec2& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }
}

Field::Field(const Settings& settings)
    : settings_(settings),
      rng_(std::random_device{}()),
      x_dist_(settings_.x_min, settings_.x_max),
      y_dist_(settings_.y_min, settings_.y_max),
      v_dist_(-settings_.vmax, settings_.vmax),
      prey_female_dist_(settings_.probability_female_prey),
      pred_female_dist_(settings_.probability_female_pred)
{
    initializeCreatures();
    initializeGrass();
}

void Field::ResetFromSettings()
{
    creatures_.clear();
    grassPatches_.clear();
    x_dist_ = std::uniform_real_distribution<float>(settings_.x_min, settings_.x_max);
    y_dist_ = std::uniform_real_distribution<float>(settings_.y_min, settings_.y_max);
    v_dist_ = std::uniform_real_distribution<float>(-settings_.vmax, settings_.vmax);
    initializeCreatures();
    initializeGrass();
}

void Field::initializeCreatures()
{
    creatures_.clear();
    creatures_.reserve((settings_.numprey + settings_.numpred) * 4.0); // preallocate for higher population

    // Spawn prey.
    for (std::size_t i = 0; i < settings_.numprey; ++i)
    {
        Vec2 pos{x_dist_(rng_), y_dist_(rng_)};
        Vec2 vel{v_dist_(rng_), v_dist_(rng_)};
        std::bernoulli_distribution is_female(prey_female_dist_);
        Sex sex = is_female(rng_) == 1 ? Sex::Female : Sex::Male;
        creatures_.emplace_back(settings_, SpeciesRole::Prey, sex, pos, vel);
        announceCreature(&creatures_.at(i));
    }

    // Spawn predators.
    for (std::size_t i = 0; i < settings_.numpred; ++i)
    {
        Vec2 pos{x_dist_(rng_), y_dist_(rng_)};
        Vec2 vel{v_dist_(rng_), v_dist_(rng_)};
        std::bernoulli_distribution is_female(pred_female_dist_);
        Sex sex = is_female(rng_) == 1 ? Sex::Female : Sex::Male;
        creatures_.emplace_back(settings_, SpeciesRole::Predator, sex, pos, vel);
        std::cout << (creatures_.at(i).sex() == Sex::Female ? "Female" : "Male") << " "
            << (creatures_.at(i).species() == SpeciesRole::Prey ? "Prey" : "Predator" ) << " was created."
            << std::endl;
    }
}

void Field::step(float dt)
{
    // Update all existing creatures.
    for (Creature& c : creatures_)
    {
        c.update(dt, settings_);
    }
    
    handleGrass(dt);
    // Apply interactions: eating, mating, and pruning of dead creatures.
    handleInteractions();

    // Remove any creatures that were killed this frame.
    creatures_.erase(
        std::remove_if(creatures_.begin(), creatures_.end(),
                       [](const Creature& c) { return !c.isAlive(); }),
        creatures_.end());
}

void Field::handleInteractions()
{
    const float interaction_radius2 = settings_.interaction_radius * settings_.interaction_radius;

    std::vector<Creature> newborns;
    newborns.reserve(creatures_.size() / 4); // rough guess

    const std::size_t n = creatures_.size();
    for (std::size_t i = 0; i < n; ++i)
    {
        Creature& a = creatures_[i];
        if (!a.isAlive()) continue;

        for (std::size_t j = i + 1; j < n; ++j)
        {
            Creature& b = creatures_[j];
            if (!b.isAlive()) continue;

            const float dist2 = distanceSquared(a.position(), b.position());
            if (dist2 > interaction_radius2)
                continue;

            // Predator / prey interaction.
            if (a.species() != b.species())
            {
                Creature* predator = nullptr;
                Creature* prey     = nullptr;

                if (a.species() == SpeciesRole::Predator)
                {
                    predator = &a;
                    prey     = &b;
                }
                else
                {
                    predator = &b;
                    prey     = &a;
                }

                if (!predator || !prey || !prey->isAlive())
                    continue;

                // Simple rule: predator eats prey if its fullness is below the
                // configured threshold.
                if (predator->hunger() <= settings_.pred_hunger_threshold)
                {
                    predator->onEat(settings_);
                    prey->kill();
                }
            }
            else
            {
                // Same species: check for mating.
                Creature& c1 = a;
                Creature& c2 = b;
                
                // Are they even alive?
                if (!c1.isAlive() || !c2.isAlive())
                    continue;
                
                // Are they even compatible?
                if (c1.sex() == c2.sex())
                    continue;

                const bool is_prey = (c1.species() == SpeciesRole::Prey);

                const float libido_threshold =
                    is_prey ? settings_.prey_libido_threshold
                            : settings_.pred_libido_threshold;

                if (c1.libido() >= libido_threshold &&
                    c2.libido() >= libido_threshold)
                {
                    // Spawn a child at the midpoint of the parents' positions.
                    Vec2 child_pos{
                        0.5f * (c1.position().x + c2.position().x),
                        0.5f * (c1.position().y + c2.position().y)
                    };
                    Vec2 child_vel{
                        v_dist_(rng_),
                        v_dist_(rng_)
                    };
                    
                    std::bernoulli_distribution is_female(prey_female_dist_);
                    Sex sex = is_female(rng_) == 1 ? Sex::Female : Sex::Male;
                    Creature newborn(settings_, c1.species(), sex, child_pos, child_vel);
                    // average the hunger of the parents so baby isn't magically full;
                    // prevents perpetual species growth if reproduction rate outpaces prey population decline
                    newborn.setHunger((c1.hunger() + c2.hunger())/2); // average the hunger of the parents so baby isn't magically full,
//                    newborns.emplace_back(
//                        settings_,
//                        c1.species(),
//                        sex,
//                        child_pos,
//                        child_vel
//                    );
                    newborns.emplace_back(c1);
                    announceCreature(&newborns.front());
                    c1.onMate(settings_);
                    c2.onMate(settings_);
                }
            }
        }
    }

    // Append any newborn creatures after all pairwise checks.
    for (Creature& c : newborns)
    {
        creatures_.push_back(std::move(c));
    }
}

std::vector<CreatureState> Field::snapshot() const
{
    std::vector<CreatureState> out;
    out.reserve(creatures_.size());

    for (const Creature& c : creatures_)
    {
        CreatureState s;
        s.x     = c.position().x;
        s.y     = c.position().y;
        s.role  = c.species();
        s.alive = c.isAlive();
        out.push_back(s);
    }

    return out;
}

void Field::initializeGrass()
{
    grassPatches_.clear();
    
    const int rows = settings_.grass_patch_rows;
    const int cols = settings_.grass_patch_cols;
    
    const float wx_min = settings_.x_min;
    const float wx_max = settings_.x_max;
    const float wy_min = settings_.y_min;
    const float wy_max = settings_.y_max;
    
    const float cell_w = (wx_max - wx_min) / static_cast<float>(cols);
    const float cell_h = (wy_max - wy_min) / static_cast<float>(rows);
    const float r = settings_.grass_radius_frac * std::min(cell_w, cell_h); // radius is based on smaller of height and width
    
    for (int ry = 0; ry < rows; ++ry)
    {
        for(int cx = 0; cx < cols; ++cx)
        {
            GrassPatch g;
            g.center = Vec2{
                wx_min + (cx + 0.5f) * cell_w,
                wy_min + (ry + 0.5f) * cell_h
            };
            g.radius = r;
            g.max_health = settings_.grass_max_health;
            g.health = g.max_health;
            grassPatches_.push_back(g);
        }
    }
}

void Field::handleGrass(float dt)
{
    // Regrow all patches
    for (GrassPatch& g : grassPatches_)
    {
        g.update(dt, settings_.grass_regrow_rate);
    }
    
    float eatCapacity = settings_.grass_eat_rate * dt; // units of health per second x time

    for (GrassPatch& g : grassPatches_) {
        if (g.health <= 0.0f) continue;

        float remaining = eatCapacity;

        for (Creature& c : creatures_) {
            if (remaining <= 0.0f) break; // grass patch is "dead," must regrow
            if (!g.contains(c.position())) continue; // not even within the grass patch, skip
            if (!c.isAlive()) continue; // creature is dead lmao, skip
            if (c.species() == SpeciesRole::Predator) continue; // not a prey, skip
            if (c.hunger() >= settings_.prey_hunger_threshold) continue; // not hungry enough, skip
            //std::cout << "hunger: " << c.hunger() << std::endl;
            float bite = std::min(settings_.grass_eat_rate * dt, remaining);
            //g.health -= bite;
            g.health = std::max(g.health - bite, 0.0f);
            //remaining -= bite;
            remaining = std::max(remaining - bite, 0.0f);
            c.add_hunger(0.1f, settings_.prey_hunger_max);
        }
    }

    
}

void Field::SetNumPrey(int num) {
    settings_.numprey = num;
}

void Field::SetNumPred(int num) {
    settings_.numpred = num;
}

void Field::SetFieldDimensions(float width, float height)
{
    settings_.x_max = settings_.x_min + width;
    settings_.y_max = settings_.y_min + height;
}

void Field::SetFieldWidth(float width)
{
    settings_.x_max = settings_.x_min + width;
}

void Field::SetFieldHeight(float height)
{
    settings_.y_max = settings_.y_min + height;
}

void Field::SetPreyMaxAge(float age)
{
    settings_.prey_max_age = age;
}

void Field::SetPredMaxAge(float age)
{
    settings_.pred_max_age = age;
}

void Field::announceCreature(Creature *c)
{
    char sex[7];
    char role[9];
    if (c->sex() == Sex::Female)
        strcpy(sex, "female");
    else
        strcpy(sex, "male");
    
    if (c->species() == SpeciesRole::Prey)
        strcpy(role, "prey");
    else
        strcpy(role, "predator");
    std::cout << "A " << sex << " " << role << " was created." << std::endl;
}
