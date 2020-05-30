use crate::path::PathGraph;
use ninja_clown_bot::{
    entity::EntityKind,
    map::{CellPos, InteractionKind},
    Api, Decision, LogLevel,
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
    api.log(LogLevel::Info, "Bib bop I'm Rust bot");
    api.log(LogLevel::Info, api.map.to_string());

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

    if data.current_target.is_none() {
        let start = CellPos::new(ninja_clown.x() as usize, ninja_clown.y() as usize);
        let graph = PathGraph::build(&api.map);

        let path = if let Some(path) = graph.path_to(&start, &data.target) {
            api.log(LogLevel::Info, "Moving to the target!");
            path
        } else if let Some(path) = graph.path_to(&start, &data.button) {
            api.log(LogLevel::Info, "Moving to the button!");
            path
        } else {
            api.log(LogLevel::Warn, "Bib bop I'm stuck");
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
        let interaction = api.map.cell_at(target.column(), target.line()).unwrap().interaction();
        if !data.activated && interaction != InteractionKind::NoInteraction {
            api.commit_decisions(&[Decision::activate(target.column(), target.line()).commit(data.ninja_handle)]);
            data.activated = true;
        } else {
            data.current_target = data.path.next();
            think(api, data)
        }
    } else {
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

        let decision = Decision::movement(delta_angle, forward, 0.0);
        api.commit_decisions(&[decision.commit(data.ninja_handle)]);
    }
}

pub fn end_level(_api: &mut Api, _data: &mut UserData) {}

pub fn destroy(_api: &mut Api) {}
