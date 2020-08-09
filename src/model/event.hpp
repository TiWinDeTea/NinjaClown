#ifndef NINJA_CLOWN_EVENT_QUEUE_HPP
#define NINJA_CLOWN_EVENT_QUEUE_HPP

#include <deque>
#include <model/types.hpp>
#include <variant>

namespace adapter {
class adapter;
}

namespace model {

class world;

enum class event_reason {
	NONE,
	REFIRE,
	DELAY,
};

struct event {
	handle_t handle;
	tick_t instant;
	event_reason reason;

	event(handle_t handle, tick_t instant, event_reason reason)
	    : handle{handle}
	    , instant{instant}
	    , reason{reason} {};
};

class event_queue {
public:
	void update(world &, adapter::adapter &);
	void add_event(handle_t, tick_t delay, event_reason);
	void clear_for_handle(handle_t);

private:
	tick_t m_tick{};
	std::deque<event> m_events{};
};

} // namespace model

#endif // NINJA_CLOWN_EVENT_QUEUE_HPP
