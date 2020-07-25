#include "utils/logging.hpp"
#include "utils/optional.hpp"
#include "utils/resource_manager.hpp"

std::string_view utils::log::details::log_for(const resource_manager &rm, std::string_view key) noexcept {
	return rm.log_for(key);
}
