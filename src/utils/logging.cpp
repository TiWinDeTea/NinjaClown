#include "utils/logging.hpp"
#include "utils/optional.hpp"
#include "utils/resource_manager.hpp"

std::string_view utils::log::details::log_for(std::string_view key) noexcept {
	return utils::resource_manager::instance().log_for(key);
}
