use ninja_clown_bot_sys::*;

#[repr(transparent)]
pub struct DecisionCommit(nnj_decision_commit);

#[derive(Default)]
#[repr(transparent)]
pub struct Decision(nnj_decision);

impl Decision {
    pub fn movement(rotation: f32, forward_diff: f32, lateral_diff: f32) -> Self {
        Self(nnj_decision {
            kind: nnj_decision_kind::DK_MOVEMENT,
            __bindgen_anon_1: nnj_decision__bindgen_ty_1 {
                movement_req: nnj_movement_request {
                    rotation,
                    forward_diff,
                    lateral_diff,
                },
            },
        })
    }

    pub fn attack(target_handle: usize) -> Self {
        Self(nnj_decision {
            kind: nnj_decision_kind::DK_ATTACK,
            __bindgen_anon_1: nnj_decision__bindgen_ty_1 {
                attack_req: nnj_attack_request { target_handle },
            },
        })
    }

    pub fn activate(column: usize, line: usize) -> Self {
        Self(nnj_decision {
            kind: nnj_decision_kind::DK_ACTIVATE,
            __bindgen_anon_1: nnj_decision__bindgen_ty_1 {
                activate_req: nnj_activate_request { column, line },
            },
        })
    }

    pub fn throw() -> Self {
        Self(nnj_decision {
            kind: nnj_decision_kind::DK_THROW,
            __bindgen_anon_1: nnj_decision__bindgen_ty_1 {
                throw_req: nnj_throw_request {
                    unused: std::ptr::null_mut(),
                },
            },
        })
    }

    pub fn commit(self, entity_handle: usize) -> DecisionCommit {
        DecisionCommit(nnj_decision_commit {
            target_handle: entity_handle,
            decision: self.0,
        })
    }
}
