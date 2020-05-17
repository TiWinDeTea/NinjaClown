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
	static void NINJACLOWN_CALLCONV log(const char *text);

	static size_t NINJACLOWN_CALLCONV map_width(void *ninja_data);
	static size_t NINJACLOWN_CALLCONV map_height(void *ninja_data);
	static void NINJACLOWN_CALLCONV map_scan(void *ninja_data, bot::cell *map_view);
	static void NINJACLOWN_CALLCONV map_update(void *ninja_data, bot::cell *map_view);
	static size_t NINJACLOWN_CALLCONV max_entities();
	static void NINJACLOWN_CALLCONV entities_update(void *ninja_data, bot::entity *entities);

	static float NINJACLOWN_CALLCONV get_angle(void *ninja_data);
	static float NINJACLOWN_CALLCONV get_x_position(void *ninja_data);
	static float NINJACLOWN_CALLCONV get_y_position(void *ninja_data);

	static void NINJACLOWN_CALLCONV turn_right(void *ninja_data);
	static void NINJACLOWN_CALLCONV turn_left(void *ninja_data);
	static void NINJACLOWN_CALLCONV move_forward(void *ninja_data);
	static void NINJACLOWN_CALLCONV move_backward(void *ninja_data);
	static void NINJACLOWN_CALLCONV move_backward_dummy(void *ninja_data);

	static void NINJACLOWN_CALLCONV activate_button(void *ninja_data);

private:
	static model::model *get_model(void *ninja_data);
	static model::world *get_world(void *ninja_data);
	static adapter::adapter *get_adapter(void *ninja_data);
};

} // namespace bot

#endif //NINJACLOWN_BOT_BOT_API_HPP
