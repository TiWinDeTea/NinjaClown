use crate::path;
use ninja_clown_bot::{entity::EntityKind, Api, Decision, LogLevel};
use pathfinding::utils::absdiff;

#[derive(Debug)]
pub struct UserData {
    ninja_handle: usize,
    target: path::Pos,
    button: path::Pos,
    path: Vec<path::Pos>,
    path_idx: usize,
}

pub fn init() {}

pub fn start_level(api: &mut Api) -> UserData {
    api.log(LogLevel::Info, "Bib bop I'm Rust bot");
    api.log(LogLevel::Info, api.map.to_string());

    let mut ninja_handle_opt = None;
    for entity in &api.entities {
        if let EntityKind::Dll = entity.kind() {
            ninja_handle_opt = Some(entity.handle());
        }
    }

    UserData {
        ninja_handle: ninja_handle_opt.expect("ninja clown not found"),
        target: path::Pos(2, 1),
        button: path::Pos(6, 1),
        path: Vec::new(),
        path_idx: 0,
    }
}

pub fn think(api: &mut Api, data: &mut UserData) {
    let ninja_clown = api.entities.get(data.ninja_handle).expect("ninja clown not found");

    if data.path.is_empty() {
        let start = path::Pos(ninja_clown.x() as usize, ninja_clown.y() as usize);
        let graph = path::PathGraph::build(&api.map);

        data.path = if let Some(path) = graph.path_to(&start, &data.target) {
            api.log(LogLevel::Info, "Moving to the target!");
            path
        } else if let Some(path) = graph.path_to(&start, &data.button) {
            api.log(LogLevel::Info, "Moving to the button!");
            path
        } else {
            api.log(LogLevel::Warn, "Bib bop I'm stuck");
            return;
        };

        data.path_idx = 0;
    }

    let next_target = &data.path[data.path_idx];

    let dist = absdiff(next_target.center_x(), ninja_clown.x()) + absdiff(next_target.center_y(), ninja_clown.y());
    if dist < 0.6 {
        data.path_idx += 1;

        if data.path_idx >= data.path.len() {
            let pos_to_activate = next_target.clone();
            data.path_idx = 0;
            data.path.clear();
            api.commit_decisions(&[Decision::activate(pos_to_activate.0, pos_to_activate.1).commit(data.ninja_handle)]);
        } else {
            think(api, data);
        }
    } else {
        let angle = ninja_clown.angle();

        let delta_x = next_target.center_x() - ninja_clown.x();
        let delta_y = ninja_clown.y() - next_target.center_y();
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
            1.0
        };

        let decision = Decision::movement(delta_angle, forward, 0.0);
        api.commit_decisions(&[decision.commit(data.ninja_handle)]);
    }
}

pub fn end_level(_api: &mut Api, _data: &mut UserData) {}

pub fn destroy(_api: &mut Api) {}
