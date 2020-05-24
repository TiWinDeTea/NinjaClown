#include "bot.h"

struct nnj_decision nnj_build_decision_none() {
	struct nnj_decision decision;
	decision.kind = DK_NONE;
	return decision;
}

struct nnj_decision nnj_build_decision_movement(float rotation, float forward_diff, float lateral_diff) {
	struct nnj_decision decision;
	decision.kind = DK_MOVEMENT;
	struct nnj_movement_request move_req;
	move_req.rotation     = rotation;
	move_req.forward_diff = forward_diff;
	move_req.lateral_diff = lateral_diff;
	decision.movement     = move_req;
	return decision;
}

struct nnj_decision nnj_build_decision_attack(size_t target_handle) {
	struct nnj_decision decision;
	decision.kind = DK_ATTACK;
	struct nnj_attack_request attack_req;
	attack_req.target_handle = target_handle;
	decision.attack          = attack_req;
	return decision;
}

struct nnj_decision nnj_build_decision_activate(size_t column, size_t line) {
	struct nnj_decision decision;
	decision.kind = DK_ACTIVATE;
	struct nnj_activate_request activate_req;
	activate_req.column = column;
	activate_req.line   = line;
	decision.activate   = activate_req;
	return decision;
}

struct nnj_decision nnj_build_decision_throw() {
	struct nnj_decision decision;
	decision.kind = DK_THROW;
	return decision;
}
