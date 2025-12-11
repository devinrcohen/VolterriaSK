//
//  constants.hpp
//  Volterria
//
//  Created by Devin R Cohen on 12/1/25.
//

#pragma once

// Core simulation constants and simple enums.
// All of the UI / rendering specific constants (sprite sizes, SFML types, etc.)
// have been removed so this header can be reused on any platform.

#include <cstddef>

enum class Direction : int { Side = 0, Down = 1, Up = 2 };
enum class Face : int { Left = -1, Right = 1 };

// Species role and sex used throughout the simulation core.
// Kept as a scoped enum so it is easy to bridge into Swift
enum class SpeciesRole : int { Prey = 0, Predator = 1 };
enum class Sex : int { Male = 0, Female = 1 };

// Default creature counts for a fresh field.
constexpr int numprey_default  = 150;
constexpr int numpred_default  = 15;

// Maximum recommended speed (units / second). This is used by the
// acceleration logic to keep velocities from exploding.
constexpr float vmax_default = 250.0f; // default 100.0

// Default period between hunger ticks (seconds).
constexpr float hunger_tick_seconds_default = 0.5f;

constexpr float default_length = 1000.f;
constexpr float golden_ratio = 1.f; // approx 1.162 ~= 1.1618033



