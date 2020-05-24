#ifndef NINJACLOWN_BOT_BOT_API_HPP
#define NINJACLOWN_BOT_BOT_API_HPP

#include <bot_interface/bot.h>

namespace model {
class model;
struct world;
} // namespace model

namespace adapter {
class adapter;
}

namespace bot {

struct ffi {
	static void NINJACLOWN_CALLCONV log(bot::nnj_log_level level, const char *text);

	static size_t NINJACLOWN_CALLCONV map_width(void *ninja_data);
	static size_t NINJACLOWN_CALLCONV map_height(void *ninja_data);
	static void NINJACLOWN_CALLCONV map_scan(void *ninja_data, bot::nnj_cell *map_view);
	static size_t NINJACLOWN_CALLCONV map_update(void *ninja_data, bot::nnj_cell *map_view, bot::nnj_cell_pos *changed_cells,
	                                             size_t changed_size);

	static size_t NINJACLOWN_CALLCONV max_entities();
	static void NINJACLOWN_CALLCONV entities_scan(void *ninja_data, bot::nnj_entity *entities);
	static size_t NINJACLOWN_CALLCONV entities_update(void *ninja_data, bot::nnj_entity *entities);

	static void NINJACLOWN_CALLCONV commit_decision(void *ninja_data, bot::nnj_decision_commit *commits, size_t num_commits);

private:
	static model::model *get_model(void *ninja_data);
	static model::world *get_world(void *ninja_data);
	static adapter::adapter *get_adapter(void *ninja_data);
};

} // namespace bot

#endif //NINJACLOWN_BOT_BOT_API_HPP
