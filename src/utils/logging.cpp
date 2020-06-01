#include "utils/logging.hpp"
#include "utils/optional.hpp"
#include "utils/resource_manager.hpp"

utils::optional<std::string_view> utils::log::details::log_for(const resource_manager &rm, std::string_view key) noexcept {
	return rm.log_for(key);
}

std::string utils::log::get_or_gen(const resource_manager& rm, std::string_view key) {
	auto str = rm.log_for(key);
	if (str) {
		return std::string{*str};
	} else {
		return "MISSING TRANSLATION FOR KEY " + std::string{key};
	}
}