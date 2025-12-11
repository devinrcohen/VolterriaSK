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
    initializeFieldCells();
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
    initializeFieldCells();
    initializeCreatures();
    initializeGrass();
}

void Field::initializeFieldCells()
{
    const int nx = settings_.num_cells_x;
    const int ny = settings_.num_cells_y;
    actual_cell_width_ = (settings_.x_max - settings_.x_min) / nx;
    actual_cell_height_ = (settings_.y_max - settings_.y_min) / ny;
    field_cells_.resize(nx);
    field_cells_.assign(nx, std::vector<FieldCell>(ny));
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
        //announceCreature(&creatures_.at(i));
    }

    // Spawn predators.
    for (std::size_t i = 0; i < settings_.numpred; ++i)
    {
        Vec2 pos{x_dist_(rng_), y_dist_(rng_)};
        Vec2 vel{v_dist_(rng_), v_dist_(rng_)};
        std::bernoulli_distribution is_female(pred_female_dist_);
        Sex sex = is_female(rng_) == 1 ? Sex::Female : Sex::Male;
        creatures_.emplace_back(settings_, SpeciesRole::Predator, sex, pos, vel);
        //announceCreature(&creatures_.at(i));
    }
}

void Field::step(float dt)
{
    // Update all existing creatures.
    for (Creature& c : creatures_)
    {
        if(c.isAlive())
            c.update(dt, settings_);
    }
    //int originalCount = static_cast<int>(creatures_.size());
    for (auto& col : field_cells_)
    {
        for (auto& cell : col)
        {
            cell.cell_creatures_indices.clear();
            // might need to clear grass too? those interactions are separate right now
        }
    }
    // Assign each creature to a cell (new)
    const int nx = settings_.num_cells_x;
    const int ny = settings_.num_cells_y;
    
    // Assign each grass patch to a cell
    for (int i = 0; i < grassPatches_.size(); ++i)
    {
        GrassPatch& g = grassPatches_[i];
        if (g.health <= 0.0f) continue;
        
        const float x = g.center.x;
        const float y = g.center.y;
        
        int cx = (x - settings_.x_min) / settings_.interaction_radius;
        int cy = (y - settings_.y_min) / settings_.interaction_radius;
        
        cx = std::clamp(cx, 0, nx-1);
        cy = std::clamp(cy, 0, ny-1);
        
        field_cells_[cx][cy].cell_grassPatches_indices.push_back(i);
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

static int pairChecks = 0;
static int pairChecks_previous = 0;
static int ticks = 0;

void Field::handleInteractions()
{
    const float interaction_radius2 = settings_.interaction_radius * settings_.interaction_radius;
    std::vector<Creature> newborns;
    newborns.reserve(creatures_.size() / 4); // approximation
    
    const int originalCount = static_cast<int>(creatures_.size()); // compare vs settings_.creature_threshold
    
    // Grid start if originalCount > settings_.creature_threshold
    for (auto& col : field_cells_)
    {
        for (auto& cell : col)
        {
            cell.cell_creatures_indices.clear();
            // might need to clear grass too? those interactions are separate right now
        }
    }
    // Assign each creature to a cell (new)
    for (int i = 0; i < originalCount; ++i)
    {
        Creature& c = creatures_[i];
        if (!c.isAlive()) continue;
        
        int cx, cy;

        ComputeCellLocation<Creature>(c, &cx, &cy);
        
        // copy the the index of the creature in the master vector into the cell's own vector
        field_cells_[cx][cy].cell_creatures_indices.push_back(i);
    }
    // consider making this a function pairCheck() -- no args because everything is taken from settings or private properties
    for (int i = 0; i < originalCount; ++i)
    {
        // loop over master vector
        Creature& A = creatures_[i];
        if(!A.isAlive()) continue;
        int cx, cy;
        ComputeCellLocation<Creature>(creatures_[i], &cx, &cy);
        // for each adjacent/cattycorner cell
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                // coordinates of nearby cell
                int nx = cx + dx;
                int ny = cy + dy;
                
                if (nx < 0 || nx >= settings_.num_cells_x) continue; // horizontally OOB
                if (ny < 0 || ny >= settings_.num_cells_y) continue; // vertically OOB
                
                FieldCell& cell = field_cells_[nx][ny];
                for (int idx : cell.cell_creatures_indices)
                {
                    // no self or duplicate interaction
                    if (idx <= i) continue;
                    Creature& B = creatures_[idx];
                    
                    // no dead interactions
                    if (!B.isAlive()) continue;
      
                    /* common: this is where the creatures are compared - something like Field::checkN2(Creature& A, Creature& B) */
                    // consider putting into a function like pairCheck(Creature&, Creature&, std::vector<Creature>& newborns)
                    // pairCheck(A, B, newborns);
                    float dist2 = distanceSquared(A.position(), B.position());
                    pairChecks++;
                    if (dist2 > interaction_radius2) continue;
                    if (dist2 <= interaction_radius2)
                    {
                        if (!A.isAlive() || !B.isAlive()) continue;
                        
                        // predator/prey interaction
                        if (A.species() != B.species()) // one of these is the hunter
                        {
                            Creature* predator = nullptr;
                            Creature* prey     = nullptr;
                            
                            if (A.species() == SpeciesRole::Predator)
                            {
                                predator = &A;
                                prey     = &B;
                            }
                            else
                            {
                                predator = &B;
                                prey     = &A;
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
                        } else { // both are the same species, mate logic
                            // Are they even alive?
                            if (!A.isAlive() || !B.isAlive())
                                continue;
                            
                            // Are they even compatible?
                            if (A.sex() == B.sex())
                                continue;
                            
                            const bool is_prey = (B.species() == SpeciesRole::Prey);
                            
                            const float libido_threshold =
                            is_prey ? settings_.prey_libido_threshold
                            : settings_.pred_libido_threshold;
                            if (A.libido() >= libido_threshold &&
                                B.libido() >= libido_threshold)
                            {
                                // Spawn a child at the midpoint of the parents' positions.
                                Vec2 child_pos{
                                    0.5f * (A.position().x + B.position().x),
                                    0.5f * (A.position().y + B.position().y)
                                };
                                Vec2 child_vel{
                                    v_dist_(rng_),
                                    v_dist_(rng_)
                                };
                                
                                std::bernoulli_distribution is_female(prey_female_dist_);
                                Sex sex = is_female(rng_) == 1 ? Sex::Female : Sex::Male;
                                Creature newborn(settings_, A.species(), sex, child_pos, child_vel);
                                // average the hunger of the parents so baby isn't magically full;
                                // prevents perpetual species growth if reproduction rate outpaces prey population decline
                                newborn.setHunger((A.hunger() + B.hunger())/2); // average the hunger of the parents so baby isn't magically full,
                                //creatures_.push_back(newborn);
                                newborns.emplace_back(std::move(newborn));
                                //announceCreature(&newborns.front());
                                //announceCreature(&newborn);
                                A.onMate(settings_);
                                B.onMate(settings_);
                            }
                        }
                    }
                }
            }
        }
    }
    
    for (Creature& c : newborns)
        creatures_.push_back(std::move(c));
    if (ticks > 60) {
        ticks = 0;
        size_t delta = pairChecks - pairChecks_previous;
        std::cerr << "creatures=" << creatures_.size() << " pairchecks=" << pairChecks << " delta=" << delta << std::endl;
        pairChecks_previous = pairChecks;
    }
    ticks++;
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
        for (int cx = 0; cx < cols; ++cx)
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

template <typename T> void Field::ComputeCellLocation(const T& obj, int *cx, int *cy)
{
    float world_x = obj.position().x;
    float world_y = obj.position().y;
    
    float x_min = settings_.x_min;
    float y_min = settings_.y_min;
    float cell_size = settings_.cell_size;
    int num_cells_x = settings_.num_cells_x;
    int num_cells_y = settings_.num_cells_y;
    
    
    int cx_unclamped = std::floor((world_x - x_min)/cell_size);
    int cy_unclamped = std::floor((world_y - y_min)/cell_size);
    
    *cx = std::clamp(cx_unclamped, 0, num_cells_x - 1);
    *cy = std::clamp(cy_unclamped, 0, num_cells_y - 1);
}

void GrassPatch::setCellLocation(float cx, float cy)
{
    cell_x_ = cx;
    cell_y_ = cy;
}

void Field::pairCheck()
{
    
}
