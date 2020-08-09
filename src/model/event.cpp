#include "event.hpp"

#include <adapter/adapter.hpp>
#include <algorithm>
#include <iterator>
#include <model/world.hpp>

void model::event_queue::update(model::world &world, adapter::adapter &adapter) {
	// This assume that our `std::deque` is ordered by `instant`. See `model::event_queue::add_event`.
	auto found_at = std::find_if(m_events.begin(), m_events.end(), [this](const event &ev) {
		return ev.instant <= m_tick;
	});
	std::vector<event> to_fire;
	std::move(found_at, m_events.end(), std::back_inserter(to_fire));
	m_events.erase(found_at, m_events.end());

	for (auto ev : to_fire) {
		world.fire_activator(adapter, ev.handle, ev.reason);
	}

	++m_tick;
}

void model::event_queue::add_event(handle_t activator_handle, tick_t delay, event_reason reason) {
	// We insert a new event in the queue making sure the `std::deque` stay ordered.
	// We could use an `std::priority_queue` but its API is not fun. Also we expect
	// only a few events, so performance-wise this is probably as good.
	tick_t instant   = m_tick + delay;
	auto lower_bound = std::lower_bound(m_events.begin(), m_events.end(), instant, [](const event &ev, const tick_t &instant) {
		return ev.instant > instant;
	});
	m_events.insert(lower_bound, {activator_handle, instant, reason});
}

void model::event_queue::clear_for_handle(handle_t activator_handle) {
	m_events.erase(std::remove_if(m_events.begin(), m_events.end(),
	                              [activator_handle](const event &ev) {
		                              return ev.handle == activator_handle;
	                              }),
	               m_events.end());
}
