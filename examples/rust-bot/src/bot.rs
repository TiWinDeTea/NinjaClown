use crate::path::PathGraph;
use ninja_clown_bot::{
    decision::DecisionCommit,
    entity::EntityKind,
    map::{CellPos, InteractionKind},
    Api, Decision, Entity,
};
use pathfinding::utils::absdiff;
use std::vec;

#[derive(Debug)]
pub struct UserData {
    ninja_handle: usize,
    target: CellPos,
    button: CellPos,
    path: vec::IntoIter<CellPos>,
    current_target: Option<CellPos>,
    activated: bool,
}

pub fn init() {}

pub fn start_level(api: &mut Api) -> UserData {
    api.log_info("Bib bop I'm Rust bot");
    api.log_trace(api.map.to_string());

    let mut ninja_handle_opt = None;
    for entity in &api.entities {
        if let EntityKind::Dll = entity.kind() {
            ninja_handle_opt = Some(entity.handle());
        }
    }

    let mut button_bos = None;
    for cell in api.map.iter_pos() {
        if cell.interaction() != InteractionKind::NoInteraction {
            button_bos = Some(cell.pos);
        }
    }

    UserData {
        ninja_handle: ninja_handle_opt.expect("ninja clown not found"),
        target: CellPos::new(2, 1),
        button: button_bos.expect("button not found"),
        path: Vec::new().into_iter(),
        current_target: None,
        activated: false,
    }
}

pub fn think(api: &mut Api, data: &mut UserData) {
    let ninja_clown = api.entities.get(data.ninja_handle).expect("ninja clown not found");

    // compute path graph
    if data.current_target.is_none() || api.map.changed() {
        let start = CellPos::new(ninja_clown.x() as usize, ninja_clown.y() as usize);
        let graph = PathGraph::build(&api.map);

        let path = if let Some(path) = graph.path_to(&start, &data.target) {
            api.log_info("Moving to the target!");
            path
        } else if let Some(path) = graph.path_to(&start, &data.button) {
            api.log_info("Moving to the button!");
            path
        } else {
            api.log_warn("Bib bop I'm stuck");
            return;
        };

        data.path = path.into_iter();
        data.current_target = data.path.next();
    }

    let target = if let Some(target) = &data.current_target {
        target
    } else {
        return;
    };

    let dist = absdiff(target.center_x(), ninja_clown.x()) + absdiff(target.center_y(), ninja_clown.y());
    if dist < 0.6 {
        let interaction = api.map.cell_at_pos(target).unwrap().interaction();
        if !data.activated && interaction != InteractionKind::NoInteraction {
            api.commit_decisions(&[Decision::activate(target.column(), target.line()).commit(data.ninja_handle)]);
            data.activated = true;
        } else if let Some(next_target) = data.path.next() {
            let decision = move_towards(ninja_clown, &next_target);
            api.commit_decisions(&[decision]);
            data.current_target = Some(next_target);
            data.activated = false;
        }
    } else {
        let decision = move_towards(ninja_clown, target);
        api.commit_decisions(&[decision]);
    }
}

fn move_towards(ninja_clown: &Entity, target: &CellPos) -> DecisionCommit {
    let angle = ninja_clown.angle();

    let delta_x = target.center_x() - ninja_clown.x();
    let delta_y = ninja_clown.y() - target.center_y();
    let target_angle = delta_y.atan2(delta_x);

    let mut delta_angle = target_angle - angle;
    while delta_angle > std::f32::consts::PI {
        delta_angle -= 2.0 * std::f32::consts::PI;
    }
    while delta_angle < -std::f32::consts::PI {
        delta_angle += 2.0 * std::f32::consts::PI;
    }

    let forward = if delta_angle.abs() > 0.05 {
        0.1 / delta_angle.abs()
    } else {
        ninja_clown.properties().move_speed()
    };

    Decision::movement(delta_angle, forward, 0.0).commit(ninja_clown.handle())
}

pub fn end_level(_api: &mut Api, _data: &mut UserData) {}

pub fn destroy(_api: &mut Api) {}
