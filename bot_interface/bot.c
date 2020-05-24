#include "bot.h"

struct decision build_decision_movement(float rotation, float forward_diff, float lateral_diff) {
	struct decision decision;
	decision.kind = DK_MOVEMENT;
	struct movement_request move_req;
	move_req.rotation     = rotation;
	move_req.forward_diff = forward_diff;
	move_req.lateral_diff = lateral_diff;
	decision.movement     = move_req;
	return decision;
}

struct decision build_decision_attack(size_t target_handle) {
	struct decision decision;
	decision.kind = DK_ATTACK;
	struct attack_request attack_req;
	attack_req.target_handle = target_handle;
	decision.attack          = attack_req;
	return decision;
}

struct decision build_decision_activate(size_t column, size_t line) {
	struct decision decision;
	decision.kind = DK_ACTIVATE;
	struct activate_request activate_req;
	activate_req.column = column;
	activate_req.line   = line;
	decision.activate   = activate_req;
	return decision;
}

struct decision build_decision_throw() {
	struct decision decision;
	decision.kind = DK_THROW;
	return decision;
}
