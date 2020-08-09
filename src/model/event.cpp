#include "event.hpp"

#include <adapter/adapter.hpp>
#include <algorithm>
#include <iterator>
#include <model/world.hpp>

void model::event_queue::update(model::world &world, adapter::adapter &adapter) {
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

void model::event_queue::add_event(handle_t handle, tick_t delay, event_reason reason) {
	tick_t instant   = m_tick + delay;
	auto lower_bound = std::lower_bound(m_events.begin(), m_events.end(), instant, [](const event &ev, const tick_t &instant) {
		return ev.instant > instant;
	});
	m_events.insert(lower_bound, {handle, instant, reason});
}

void model::event_queue::clear_for_handle(handle_t handle) {
	m_events.erase(std::remove_if(m_events.begin(), m_events.end(),
	                              [handle](const event &ev) {
		                              return ev.handle == handle;
	                              }),
	               m_events.end());
}
