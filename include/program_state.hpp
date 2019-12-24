#ifndef NINJACLOWN_PROGRAM_STATE_HPP
#define NINJACLOWN_PROGRAM_STATE_HPP

#include "view/viewer.hpp"

struct program_state {
    view::viewer& viewer;

    bool close_request{false};
    bool term_on_display{true};
};

#endif //NINJACLOWN_PROGRAM_STATE_HPP
