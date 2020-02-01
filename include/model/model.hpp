#ifndef NINJACLOWN_MODEL_MODEL_HPP
#define NINJACLOWN_MODEL_MODEL_HPP

#include "bot/bot_dll.hpp"
#include "model/world.hpp"

namespace model {
class model {
public:
	[[nodiscard]] bool load_dll(std::string dll_path) noexcept {
		return m_dll.load(std::move(dll_path));
	}

	void bot_init(bot::bot_api api) noexcept {
		api.ninja_descriptor = this;
		m_dll.bot_init(api);
	}

	void bot_think() noexcept {
		m_dll.bot_think();
	}

    ::model::world world{};
private:
	bot::bot_dll m_dll{};
};
} // namespace model
#endif //NINJACLOWN_MODEL_MODEL_HPP
