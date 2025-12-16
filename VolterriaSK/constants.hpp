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
//enum class SpeciesRole : int { Prey = 0, Predator = 1 };
//enum class Sex : int { Male = 0, Female = 1 };





