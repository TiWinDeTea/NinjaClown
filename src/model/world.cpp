#include <array>
#include <cmath>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/collision.hpp"
#include "model/components.hpp"
#include "model/event.hpp"
#include "model/systems.hpp"
#include "model/world.hpp"
#include "ninja_clown/api.h"

void model::world::update(adapter::adapter &adapter) {
	for (handle_t handle = cst::max_entities; (handle--) != 0u;) {
		switch (m_components.metadata[handle].kind) {
			case ninja_api::nnj_entity_kind::EK_PROJECTILE:
				projectile_behaviour_system(m_components.properties[handle], m_components.movement[handle]);
				break;
			case ninja_api::nnj_entity_kind::EK_PATROL:
				patrol_behaviour_system(m_components.properties[handle], m_components.movement[handle]);
				break;
			case ninja_api::nnj_entity_kind::EK_AGGRESSIVE:
				aggressive_behaviour_system(m_components.properties[handle], m_components.movement[handle]);
				break;
			default:
				break;
		}
	}

	for (handle_t handle = cst::max_entities; (handle--) != 0u;) {
		auto &hitbox = m_components.hitbox[handle];
		if (hitbox.has_value()) {
			decision_system(*this, adapter, handle, m_components.properties[handle], hitbox.value(), m_components.decision[handle],
			                m_components.movement[handle], m_components.state[handle]);
		}
	}

	for (handle_t handle = cst::max_entities; (handle--) != 0u;) {
		auto &hitbox = m_components.hitbox[handle];
		if (hitbox.has_value()) {
			action_system(*this, adapter, handle, m_components.properties[handle], hitbox.value(), m_components.state[handle]);
		}
	}

	for (handle_t handle = cst::max_entities; (handle--) != 0u;) {
		auto &hitbox = m_components.hitbox[handle];
		if (hitbox.has_value()) {
			movement_system(*this, adapter, handle, m_components.properties[handle], hitbox.value(), m_components.movement[handle],
			                m_components.metadata[handle]);
		}
	}

	m_event_queue.update(*this, adapter);
}

void model::world::reset() {
	m_map.resize(0, 0);
	m_interactions.clear();
	m_activators.clear();
	m_actionables.clear();

	for (unsigned int i = 0; i < cst::max_entities; ++i) {
		reset_entity(i);
	}
}

void model::world::reset_entity(const handle_t handle) {
	m_components.state[handle]      = {};
	m_components.metadata[handle]   = {};
	m_components.properties[handle] = {};
	m_components.movement[handle].reset();
	m_components.decision[handle].reset();
	m_components.health[handle].reset();
	m_components.hitbox[handle].reset();
}

void model::world::move_entity(adapter::adapter &adapter, const handle_t entity_handle, const vec2 displacement) {
	std::optional<component::hitbox> &hitbox_opt = m_components.hitbox[entity_handle];
	if (!hitbox_opt.has_value()) {
		return;
	}
	component::hitbox &hitbox = hitbox_opt.value();

	float old_x = hitbox.center.x;
	hitbox.center.x += displacement.x;
	if (entity_check_collision(entity_handle)) {
		hitbox.center.x = old_x;
	}

	float old_y = hitbox.center.y;
	hitbox.center.y += displacement.y;
	if (entity_check_collision(entity_handle)) {
		hitbox.center.y = old_y;
	}

	adapter.move_entity(adapter::model_handle{entity_handle, adapter::model_handle::ENTITY}, hitbox.center.x, hitbox.center.y);
}

void model::world::rotate_entity(adapter::adapter &adapter, const handle_t entity_handle, const float rotation_rad) {
	std::optional<component::hitbox> &hitbox_opt = m_components.hitbox[entity_handle];
	if (!hitbox_opt.has_value()) {
		return;
	}
	component::hitbox &hitbox = hitbox_opt.value();

	float old_rad = hitbox.rad;

	hitbox.rad += rotation_rad;
	if (hitbox.rad >= uni::math::pi<float>) {
		hitbox.rad -= 2 * uni::math::pi<float>;
	} else if (hitbox.rad <= -uni::math::pi<float>) {
		hitbox.rad += 2 * uni::math::pi<float>;
	}

	if (entity_check_collision(entity_handle)) {
		hitbox.rad = old_rad;
	} else {
		adapter.rotate_entity(adapter::model_handle{entity_handle, adapter::model_handle::ENTITY}, hitbox.rad);
	}
}

bool model::world::entity_check_collision(const handle_t entity_handle) {
	std::optional<component::hitbox> &hitbox_opt = m_components.hitbox[entity_handle];
	if (!hitbox_opt.has_value()) {
		return false;
	}
	component::hitbox &hitbox = hitbox_opt.value();

	obb this_box{hitbox};

	// Against grid cells
	bounding_circle this_circle{hitbox};
	for (const cell_view &cell : m_map.subgrid(this_box)) {
		if (cell.type != cell_type::GROUND) {
			aabb cell_box{cell.pos};
			if (circle_aabb_test(this_circle, cell_box)) {
				return true;
			}
		}
	}

	// Against other entities
	for (handle_t other_handle = cst::max_entities; (other_handle--) != 0u;) {
		if (other_handle != entity_handle) {
			std::optional<component::hitbox> &other_hitbox = m_components.hitbox[other_handle];
			if (other_hitbox.has_value()) {
				obb other_box{other_hitbox.value()};
				if (obb_obb_sat_test(this_box, other_box)) {
					return true;
				}
			}
		}
	}

	return false;
}

void model::world::fire_activator(adapter::adapter &adapter, const handle_t activator_handle, const event_reason reason) {
	auto &activator = m_activators[activator_handle];

	if (activator.enabled && reason == event_reason::ENTITY) {
		// Disable activator if it is enabled and an entity activate it again.
		activator.enabled = false;

		// Remove future events for this activator from the queue.
		m_event_queue.clear_for_handle(activator_handle);
	} else if (activator.activation_delay != 0 && reason != event_reason::DELAY) {
		// If an activation delay is set for this activator, enqueue an event fire firing the actionnable later.
		// When the activator is fired because of event_reason::DELAY, a new event should not be enqueued again.
		m_event_queue.add_event(activator_handle, activator.activation_delay, event_reason::DELAY);
		activator.enabled = true;
	} else {
		// Otherwise, the actionnables should be actually actionned.
		for (handle_t target : activator.targets) {
			fire_actionable(adapter, target);
		}

		// Notify that the activator actionned the actionnables.
		adapter.fire_activator(adapter::model_handle{activator_handle, adapter::model_handle::ACTIVATOR});

		// Job is done, mark the activator as disabled.
		activator.enabled = false;

		// … but reenable it if there is refire_after / refire_repeat configured on it.
		if (activator.refire_after && (reason != event_reason::REFIRE || activator.refire_repeat)) {
			m_event_queue.add_event(activator_handle, activator.refire_after.value(), event_reason::REFIRE);
			activator.enabled = true;
		}
	}
}

void model::world::fire_actionable(adapter::adapter &adapter, const handle_t handle) {
	m_actionables[handle].make_action({*this, adapter});
}
