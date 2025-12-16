//
//  settings.hpp
//  Volterria
//
//  Created by Devin R Cohen on 12/1/25.
//

#pragma once

// Global simulation settings shared by the Field and all Creatures.
// This is intentionally free of any rendering library dependencies so that
// the same code can be used on desktop (SFML) and on iOS (SwiftUI/SpriteKit).

#include "constants.hpp"

struct Settings
{
    float default_length = 500.f;
    float height_ratio = 1.682f; // approx 1.162 ~= 1.1618033
    // World bounds in "field" coordinates.
    // Swift / SFML can map these to pixels however they like.
    float x_min = 0.0f;
    float x_max = x_min + default_length/height_ratio;
    float y_min = 0.0f;
    float y_max = y_min + default_length;
    const float x_center = (x_min + x_max)/2.0;
    const float y_center = (y_min + y_max)/2.0;
    
    const float field_width = x_max - x_min;
    const float field_height = y_max - y_min;
    
    // spawn locations
    float prey_spawn_mean_x = x_center;
    float prey_spawn_mean_y = (y_min + y_center) / 2.0; // half-way between bottom and center
    float prey_spawn_stdev_n = 5.0; // number of std.dvs between center field and walls
    float prey_spawn_stdev = std::min(field_height/2.0, field_height/2.0) / (2 * prey_spawn_stdev_n);
    
    float predator_spawn_mean_x = x_center;
    float predator_spawn_mean_y = (y_center + y_max) / 2.0; // half-way between center and top
    float predator_spawn_stdev_n = 5.0;
    float predator_spawn_stdev = std::min(field_height/2.0, field_height/2.0) / (2 * predator_spawn_stdev_n);
    
    // Initial population sizes.
    int numprey = 80;
    int numpred = 16;
    
    // Velocity / acceleration tuning.
    // vmax is the soft cap for creature speed (units / second).
    float vmax_default = 100.f;
    
    // How often (seconds) each creature is allowed to "re-roll" its
    // acceleration vector. This keeps motion from changing direction
    // on every single frame.
    float accel_tick = 0.125f;
    
    // Predator / prey hunger and libido tuning.
    // These are intentionally fairly high-level so you can experiment
    // from Swift without touching the C++.
    float pred_hunger_threshold = 3.0f;  // below this, predator will eat on contact
    float prey_hunger_threshold = 5.0f;  // below this, prey will eat grass on contact
    float prey_hunger_restore_rate = 8.f; // how much health to add when eating per second
    float pred_libido_threshold = 4.0f;  // must be above this to reproduce
    float prey_libido_threshold = 3.0f;
    
    // Intent-based motion tuning
    float predator_hunt_speed_min = 100.f;
    float predator_hunt_speed_max = 250.f;
    float predator_hunt_max_accel = 200.f;
    
    float prey_forage_speed_min = 100.f;
    float prey_forage_speed_max = 250.f;
    float prey_forage_max_accel = 200.f;
    
    float prey_mate_speed_min = 120.0f; // 120
    float prey_mate_speed_max = 320.0f; // 320
    float prey_mate_max_accel = 200.f; // 300

    float predator_mate_speed_min = 100.0f; // 160
    float predator_mate_speed_max = 300.0f; // 420
    float predator_mate_max_accel = 200.0f; // 350
    
    float predator_hunger_max   = 10.0f;
    float prey_hunger_max   = 10.0f;
    float predator_libido_max   = 10.0f;
    float prey_libido_max   = 10.0f;
    
    // How often hunger is updated (seconds).
    float hunger_tick_seconds = 0.2f;
    
    // Per-second libido growth rates (per second).
    float prey_libido_rate = 0.25f; // 0.3f
    float pred_libido_rate = 1.0f; // 1.f
    
    // Per-second starvation rates.
    float prey_starve_rate = 0.5f;  // 0.5 "fullness" unit per second
    
    float pred_starve_rate = 1.0f;
    
    // Distance within which creatures can interact (eat / mate), in
    // field units. Rendering code can pick whatever scale makes sense.
    float interaction_radius = 30.0f;
    float prey_vision_radius = 300.0f;
    float predator_vision_radius = 400.0f; // 300
    float interaction_multiplier = 2.f;
    float min_normalized_hunger_to_mate = 0.3f;
    //float cell_size = interaction_multiplier * interaction_radius;
    const float cell_size = interaction_radius * interaction_multiplier;
    
    const float prey_max_speed = vmax_default / 1.0f; // 20% the speed of a predator so predator always wins, tune the denominator.
    const float predator_max_speed = vmax_default;
    bool prevent_spirals = false; // causes females not to chase a mate if set true
    // Aging parameters
    float prey_max_age = 30.f; // seconds in-game time: 60f latest default
    //float prey_age_tolerance = 2.f; // +/- seconds
    float pred_max_age = 20.f; // seconds in-game time: 45f latest default
    //float predator_age_tolerance = 2.f; // +/- seconds
    
    // max age randomness, +/-25% by default
    float age_variation_fraction = 0.25f;
    
    // Grass stuff
    int grass_patch_rows = 5;
    int grass_patch_cols = 3;
    float grass_max_health = 10.0f;
    float grass_regrow_rate = 1.0f; // health units per second
    float grass_radius_frac = 0.5f; // fraction of cell size to be used as radius
    float grass_eat_rate = 4.0f;
    float min_grass_edible_health = 0.f; // prey can't eat until patch is this healthy
    
    
    // Sex
    float probability_female_prey = 0.5f;
    float probability_female_pred = 0.5f;
    
    const int num_cells_x = std::ceil((x_max - x_min) / cell_size);
    const int num_cells_y = std::ceil((y_max - y_min) / cell_size);
    
    
};
