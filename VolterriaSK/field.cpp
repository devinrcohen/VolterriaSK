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
      x_dist_uniform_(settings_.x_min, settings_.x_max),
      y_dist_uniform_(settings_.y_min, settings_.y_max),
      v_dist_uniform_(-settings_.vmax, settings_.vmax),
      x_prey_spawn_normal_(settings_.prey_spawn_mean_x, settings_.prey_spawn_stdev),
      y_prey_spawn_normal_(settings_.prey_spawn_mean_y, settings_.prey_spawn_stdev),
      x_predator_spawn_normal_(settings_.predator_spawn_mean_x, settings_.predator_spawn_stdev),
      y_predator_spawn_normal_(settings_.predator_spawn_mean_y, settings_.predator_spawn_stdev),
      prey_female_dist_(settings_.probability_female_prey),
      pred_female_dist_(settings_.probability_female_pred)
{
    //initializeFieldCells();
    //initializeCreatures(DistType::Uniform);
    //initializeCreatures(DistType::Normal);
    //initializeGrass();
}

void Field::ResetFromSettings()
{
    creatures_.clear();
    std::cerr << "creatures cleared\n";
    grassPatches_.clear();
    x_dist_uniform_ = std::uniform_real_distribution<float>(settings_.x_min, settings_.x_max);
    y_dist_uniform_ = std::uniform_real_distribution<float>(settings_.y_min, settings_.y_max);
    v_dist_uniform_ = std::uniform_real_distribution<float>(-settings_.vmax, settings_.vmax);
    x_prey_spawn_normal_ = std::normal_distribution<float>(settings_.prey_spawn_mean_x, settings_.prey_spawn_stdev);
    y_prey_spawn_normal_ = std::normal_distribution<float>(settings_.prey_spawn_mean_y, settings_.prey_spawn_stdev);
    x_predator_spawn_normal_ = std::normal_distribution<float>(settings_.predator_spawn_mean_x, settings_.predator_spawn_stdev);
    y_predator_spawn_normal_ = std::normal_distribution<float>(settings_.predator_spawn_mean_y, settings_.predator_spawn_stdev);
    initializeFieldCells();
    std::cerr << "field cells initialized\n";
    initializeCreatures(DistType::Normal);
    std::cerr << "creatures initialized\n";
    initializeGrass();
    std::cerr << "grass initialized\n";
}

void Field::initializeFieldCells()
{
    const int nx = settings_.num_cells_x;
    const int ny = settings_.num_cells_y;
    actual_cell_width_ = (settings_.x_max - settings_.x_min) / nx;
    actual_cell_height_ = (settings_.y_max - settings_.y_min) / ny;
    field_cells_.resize(nx);
    field_cells_.assign(nx, std::vector<FieldCell>(ny));
    std::cout << "cell size: " << settings_.cell_size << std::endl;
    std::cout << field_cells_.size() << " rows.\n";
    std::cout << field_cells_[0].size() << " cols.\n";
}

void Field::initializeCreatures(DistType spawnDistType)
{
    creatures_.clear();
    creatures_.reserve((settings_.numprey + settings_.numpred) * 4.0); // preallocate for higher population
    Vec2 pos, vel;
    
    // Spawn prey.
    for (std::size_t i = 0; i < settings_.numprey; ++i)
    {
        pos.x = (spawnDistType == DistType::Normal) ? x_prey_spawn_normal_(rng_) : x_dist_uniform_(rng_);
        pos.y = (spawnDistType == DistType::Normal) ? y_prey_spawn_normal_(rng_) : y_dist_uniform_(rng_);
        vel.x = v_dist_uniform_(rng_);
        vel.y = v_dist_uniform_(rng_);
        std::bernoulli_distribution is_female(prey_female_dist_);
        Sex sex = is_female(rng_) == 1 ? Sex::Female : Sex::Male;
        creatures_.emplace_back(next_creature_id++ ,settings_, SpeciesRole::Prey, sex, pos, vel);
        //announceCreature(&creatures_.at(i));
    }

    // Spawn predators.
    for (std::size_t i = 0; i < settings_.numpred; ++i)
    {
        pos.x = (spawnDistType == DistType::Normal) ? x_predator_spawn_normal_(rng_) : x_dist_uniform_(rng_);
        pos.y = (spawnDistType == DistType::Normal) ? y_predator_spawn_normal_(rng_) : y_dist_uniform_(rng_);
        vel.x = v_dist_uniform_(rng_);
        vel.y = v_dist_uniform_(rng_);
        std::bernoulli_distribution is_female(pred_female_dist_);
        Sex sex = is_female(rng_) == 1 ? Sex::Female : Sex::Male;
        creatures_.emplace_back(next_creature_id++, settings_, SpeciesRole::Predator, sex, pos, vel);
        //announceCreature(&creatures_.at(i));
    }
}

void Field::step(float dt)
{
    // Update all existing creatures.
    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time_);
    start_time_ = std::chrono::steady_clock::now(); // reassign start_time_
    std::chrono::duration<float> sec = elapsed;
    elapsed_sec_ = elapsed.count();

    //intents_.assign(creatures_.size(), SteeringIntent{});
    for (auto& col : field_cells_)
    {
        for (auto& cell : col)
        {
            cell.cell_creatures_indices.clear();
            cell.cell_grassPatches_indices.clear();
            // might need to clear grass too? those interactions are separate right now
        }
    }
    // Assign each creature to a cell (new)
//    const int nx = settings_.num_cells_x;
//    const int ny = settings_.num_cells_y;
    
    for (int i = 0; i < creatures_.size(); ++i)
    {
        Creature& c = creatures_[i];
        if (!c.isAlive()) continue;

        int cx, cy;
        ComputeCellLocation<Creature>(c, &cx, &cy);
        // copy the the index of the creature in the master vector into the cell's own vector
        field_cells_[cx][cy].cell_creatures_indices.push_back(i);
    }
    
    // Assign each grass patch to a cell
    for (int i = 0; i < grassPatches_.size(); ++i)
    {
        GrassPatch& g = grassPatches_[i];
        if (g.health <= 0.0f) continue;

        int cx, cy;
        ComputeCellLocation<GrassPatch>(g, &cx, &cy);
        
        field_cells_[cx][cy].cell_grassPatches_indices.push_back(i);
    }
    
    // fill intents_
    computeIntents();
    
    for (int i = 0; i < (int)creatures_.size(); ++i)
    {
        if (creatures_[i].isAlive())
        {
            creatures_[i].update(dt, settings_, intents_[i]);
        }
    }
    

    handleGrass(dt);
    // Apply interactions: eating, mating, and pruning of dead creatures.
    //computeIntent();
    handleInteractions();
    // Remove any creatures that were killed this frame.
    creatures_.erase(
        std::remove_if(creatures_.begin(), creatures_.end(),
                       [](const Creature& c) { return !c.isAlive(); }),
        creatures_.end());
}

void Field::handleInteractions()
{
    int pairChecks = 0;
    const float interaction_radius2 = settings_.interaction_radius * settings_.interaction_radius;
    const int maxOffset = std::ceil(settings_.interaction_radius / settings_.cell_size); // use predator vision since it's largest in current implementation, this will probably change though.
    std::vector<Creature> newborns;
    newborns.reserve(creatures_.size() / 4); // approximation
    
    const int originalCount = static_cast<int>(creatures_.size()); // compare vs settings_.creature_threshold
    // consider making this a function pairCheck() -- no args because everything is taken from settings or private properties
    for (int i = 0; i < originalCount; ++i)
    {
        // loop over master vector
        Creature& A = creatures_[i];
        if(!A.isAlive()) continue;
        int cx, cy;
        ComputeCellLocation<Creature>(creatures_[i], &cx, &cy);
        // for each adjacent/cattycorner cell
        //for (int dx = -1; dx <= 1; ++dx)
        for (int dx = -maxOffset; dx <= maxOffset; ++dx)
        {
            //for (int dy = -1; dy <= 1; ++dy)
            for (int dy = -maxOffset; dy <= maxOffset; ++dy)
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
//                                Vec2 child_pos{
//                                    0.5f * (A.position().x + B.position().x),
//                                    0.5f * (A.position().y + B.position().y)
//                                };
                                Vec2 child_vel{
                                    v_dist_uniform_(rng_),
                                    v_dist_uniform_(rng_)
                                };
                                Vec2 child_pos = 0.5 * (A.position() + B.position());
                                
                                std::bernoulli_distribution is_female(prey_female_dist_);
                                Sex sex = is_female(rng_) == 1 ? Sex::Female : Sex::Male;
                                Creature newborn(next_creature_id++,settings_, A.species(), sex, child_pos, child_vel);
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
    
    pair_checks_per_frame_ = pairChecks;
    for (Creature& c : newborns)
        creatures_.push_back(std::move(c));
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
            if (remaining <= 0.f) break; // grass patch is "dead," must regrow
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
            //c.add_hunger(0.25f, settings_.prey_hunger_max); // 0.1f  -adjust to remove population crash
            c.add_hunger(settings_.prey_hunger_restore_rate * dt, settings_.prey_hunger_max);
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

void Field::computeIntents()
{
    intents_.assign(creatures_.size(), {});
    
    for (int i = 0; i < (int) creatures_.size(); ++i)
    {
        Creature& A = creatures_[i];
        if (!A.isAlive()) continue; // he's DEAD! he's LIFELESS!
        
        const float visionR = (A.species() == SpeciesRole::Predator)
        ? settings_.predator_vision_radius
        : settings_.prey_vision_radius;
        const float visionR2 = visionR * visionR;
        const int maxOffset = (int) std::ceil(visionR / settings_.cell_size);
        
        int cx, cy;
        ComputeCellLocation<Creature>(A, &cx, &cy);
        
        int bestIdx = -1;
        float bestD2 = visionR2;
        
        // NEW for grass grid
        int bestGrassIdx = -1;
        float bestGrassD2 = visionR2;
        
        // neighboring cell loops
        for(int dx = -maxOffset; dx <= maxOffset; ++dx)
        {
            for(int dy = -maxOffset; dy <= maxOffset; ++dy)
            {
                int nx = cx + dx;
                int ny = cy + dy;
                
                // bounds checking
                if (nx < 0 || nx >= settings_.num_cells_x) continue;
                if (ny < 0 || ny >= settings_.num_cells_y) continue;
                
                FieldCell& cell = field_cells_[nx][ny];
                
                // Prey Seeks Grass
                // This relies on grass being assigned to cells before computeIntents()
                if (A.species() == SpeciesRole::Prey && A.shouldHunt(settings_))
                {
                    for (int gi : cell.cell_grassPatches_indices)
                    {
                        const GrassPatch& g = grassPatches_[gi];
                        if (g.health <= 0.0f) continue;
                        
                        const float d2g = distanceSquared(A.position(), g.center);
                        if (d2g < bestGrassD2)
                        {
                            bestGrassD2 = d2g;
                            bestGrassIdx = gi;
                        }
                    }
                }
                
                for (int idx : cell.cell_creatures_indices)
                {
                    if (idx == i) continue;
                    Creature& B = creatures_[idx];
                    if(!B.isAlive()) continue;
                    
                    const float d2 = distanceSquared(A.position(), B.position());
                    if (d2 >= bestD2) continue;
                    
                    // Decide what A is looking for
                    if (A.shouldHunt(settings_))
                    {
//                        if (A.species() == SpeciesRole::Predator && B.species() == SpeciesRole::Prey)
//                        {
//                            bestIdx = idx;
//                            bestD2 = d2;
//                        }
                    } else if (A.shouldSeekMate(settings_))
                    {
                        if (settings_.prevent_spirals && A.sex() != Sex::Male) continue; // only males pursue; removes spiral chases
                        if (!B.shouldSeekMate(settings_)) continue; // only pursue females who are ready to go
//                        if (A.species() == B.species())
//                        {
//                            const bool compatible = (A.sex() == Sex::Female && B.sex() == Sex::Male) ||
//                            (A.sex() == Sex::Male && B.sex() == Sex::Female);
//                            if (compatible)
//                            {
//                                bestIdx = idx;
//                                bestD2 = d2;
//                            }
//                        }
                        // effectively, male A's only chase female B's for mating, both must be "full enough"
                        if (A.species() == B.species() && B.sex() == Sex::Female
                            && 0.5*(A.normalizedHunger()+B.normalizedHunger()) >= settings_.min_normalized_hunger_to_mate)
                        {
                            bestIdx = idx;
                            bestD2 = d2;
                        }
                    }
                }
            }
        } // end neighbor groups
        
        // If hungry prey found grass, prefer that over mate seeking
        if (A.species() == SpeciesRole::Prey && A.shouldHunt(settings_) && bestGrassIdx != -1)
        {
            Vec2 dir = grassPatches_[bestGrassIdx].center - A.position();
            intents_[i].desired_dir = 1.f/std::sqrt(lengthSquared(dir)) * dir;
            intents_[i].has_target = true;
            continue; // don't target a mate too
        }
        
        if (bestIdx != -1)
        {
            Vec2 dir = creatures_[bestIdx].position() - A.position();
            intents_[i].desired_dir = 1.f/std::sqrt(lengthSquared(dir)) * dir;
            intents_[i].has_target = true;
        }
    }
}
