#ifndef NINJACLOWN_PROGRAM_STATE_HPP
#define NINJACLOWN_PROGRAM_STATE_HPP

#include "view/viewer.hpp"
#include "utils/resource_manager.hpp"

struct program_state {
    view::viewer viewer;

    utils::resource_manager resource_manager;

    bool close_request{false};
    bool term_on_display{true};

    static program_state* global;
};

#endif //NINJACLOWN_PROGRAM_STATE_HPP
