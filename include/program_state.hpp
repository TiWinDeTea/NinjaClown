#ifndef NINJACLOWN_PROGRAM_STATE_HPP
#define NINJACLOWN_PROGRAM_STATE_HPP

#include "view/viewer.hpp"
#include "utils/resource_manager.hpp"
#include "terminal_commands.hpp"

#include <imterm/terminal.hpp>

struct program_state {
    static struct empty_struct s_empty;

    ImTerm::terminal<terminal_commands> terminal{s_empty, "terminal"};
    view::viewer viewer{};

    utils::resource_manager resource_manager{};

    bool close_request{false};
    bool term_on_display{true};

    static program_state* global;
};

#endif //NINJACLOWN_PROGRAM_STATE_HPP
