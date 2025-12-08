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
    // World bounds in "field" coordinates.
    // Swift / SFML can map these to pixels however they like.
    float x_min = 0.0f;
    float x_max = 1000.0f;
    float y_min = 0.0f;
    float y_max = 1000.0f;

    // Initial population sizes.
    std::size_t numprey = numprey_default;
    std::size_t numpred = numpred_default;

    // Velocity / acceleration tuning.
    // vmax is the soft cap for creature speed (units / second).
    float vmax = vmax_default;

    // How often (seconds) each creature is allowed to "re-roll" its
    // acceleration vector. This keeps motion from changing direction
    // on every single frame.
    float accel_tick = 0.125f;

    // Predator / prey hunger and libido tuning.
    // These are intentionally fairly high-level so you can experiment
    // from Swift without touching the C++.
    float pred_hunger_threshold = 8.0f;  // below this, predator will eat on contact
    float prey_hunger_threshold = 5.0f;  // below this, prey will eat grass on contact
    float pred_libido_threshold = 3.0f;  // must be above this to reproduce
    float prey_libido_threshold = 3.0f;

    float pred_hunger_max   = 10.0f;
    float prey_hunger_max   = 10.0f;
    float pred_libido_max   = 10.0f;
    float prey_libido_max   = 10.0f;

    // How often hunger is updated (seconds).
    float hunger_tick_seconds = hunger_tick_seconds_default;

    // Per-second libido growth rates.
    float prey_libido_rate = 0.5f;
    float pred_libido_rate = 0.2f;

    // Per-second starvation rates.
    float prey_starve_rate = 0.1f;  // one "fullness" unit per second
    float pred_starve_rate = 0.25f;

    // Distance within which creatures can interact (eat / mate), in
    // field units. Rendering code can pick whatever scale makes sense.
    float interaction_radius = 10.0f;
    
    // Aging parameters
    float prey_max_age = 22.f; // seconds in-game time
    float pred_max_age = 66.f; // seconds in-game time
    
    // max age randomness, +/-25% by default
    float age_variation_fraction = 0.25f;
    
    // Grass stuff
    int grass_patch_rows = 3;
    int grass_patch_cols = 3;
    float grass_max_health = 7.0f;
    float grass_regrow_rate = 0.2f; // health units per second
    float grass_radius_frac = 0.5f; // fraction of cell size to be used as radius
    float grass_eat_rate = 0.3f;
};
