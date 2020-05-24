#ifndef NINJACLOWN_BOT_BOT_FACTORY_HPP
#define NINJACLOWN_BOT_BOT_FACTORY_HPP

#include "bot/bot_api.hpp"
#include "bot_interface/bot.h"

namespace bot {

struct commit_decision {
	decltype(bot_api::commit_decision) ptr;
};

template <typename... Funcs>
bot_api make_api(Funcs &&... funcs);

} // namespace bot

#include "bot_factory_impl.tpp"

#endif //NINJACLOWN_BOT_BOT_FACTORY_HPP
