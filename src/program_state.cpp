#include "program_state.hpp"

empty_struct program_state::s_empty{};
program_state *program_state::global{nullptr};
