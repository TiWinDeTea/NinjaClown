#ifndef NINJACLOWN_BOT_BOT_FACTORY_HPP
#define NINJACLOWN_BOT_BOT_FACTORY_HPP

#include "ninja_clown/api.h"
#include "bot/bot_api.hpp"

namespace bot {

struct commit_decision {
	decltype(ninja_api::nnj_api::commit_decisions) ptr;
};

template <typename... Funcs>
ninja_api::nnj_api make_api(Funcs &&... funcs);

} // namespace bot

#include "bot_factory_impl.tpp"

#endif //NINJACLOWN_BOT_BOT_FACTORY_HPP
