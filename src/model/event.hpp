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
	/// Need to be called at each game tick
	void update(world &, adapter::adapter &);
	/// Register activator to be activated in future
	void add_event(handle_t, tick_t delay, event_reason);
	/// Unregister all events related to a specific activator
	void clear_for_handle(handle_t);

private:
	tick_t m_tick{};
	std::deque<event> m_events{};
};

} // namespace model

#endif // NINJA_CLOWN_EVENT_QUEUE_HPP
