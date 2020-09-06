use crate::path::PathGraph;
use ninja_clown_bot::{
    decision::DecisionCommit,
    entity::{EntityKind, EntityState},
    map::{CellPos, InteractionKind},
    Api, Decision, Entity,
};
use pathfinding::utils::absdiff;
use std::vec;

#[derive(Debug)]
pub struct UserData {
    ninja_handle: usize,
    enemy_handle: usize,
    target: CellPos,
    button: Option<CellPos>,
    path: vec::IntoIter<CellPos>,
    current_target: Option<CellPos>,
    activated: bool,
    is_going_to_button: bool,
}

pub fn init() {}

pub fn start_level(api: &mut Api) -> UserData {
    api.log_info("Bib bop I'm Rust bot");
    api.log_trace(api.map.to_string());

    let mut ninja_handle_opt = None;
    let mut enemy_handle_opt = None;
    for entity in &api.entities {
        match entity.kind() {
            EntityKind::Harmless | EntityKind::Patrol | EntityKind::Aggressive => {
                enemy_handle_opt = Some(entity.handle())
            }
            EntityKind::Dll => ninja_handle_opt = Some(entity.handle()),
            _ => {}
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
        enemy_handle: enemy_handle_opt.unwrap_or(usize::MAX),
        target: CellPos::new(2, 1),
        button: button_bos,
        path: Vec::new().into_iter(),
        current_target: None,
        activated: false,
        is_going_to_button: false,
    }
}

pub fn think(api: &mut Api, data: &mut UserData) {
    let ninja_clown = api.entities.get(data.ninja_handle).expect("ninja clown not found");

    if ninja_clown.state() == EntityState::Busy {
        return;
    }

    if let Some(enemy) = api.entities.get(data.enemy_handle) {
        let dist = absdiff(enemy.x(), ninja_clown.x()) + absdiff(enemy.y(), ninja_clown.y());
        if dist < ninja_clown.properties().attack_range() {
            api.commit_decisions(&[Decision::attack(data.enemy_handle).commit(data.ninja_handle)]);
            api.log_info("Attack!");
            return;
        }
    }

    // compute path graph
    if data.current_target.is_none() || api.map.changed() {
        let start = CellPos::new(ninja_clown.x() as usize, ninja_clown.y() as usize);
        let graph = PathGraph::build(&api.map);

        let path = if let Some(path) = graph.path_to(&start, &data.target) {
            data.is_going_to_button = false;
            api.log_info("Moving to the target!");
            path
        } else if let Some(button_pos) = &data.button {
            if let Some(path) = graph.path_to(&start, button_pos) {
                data.is_going_to_button = true;
                api.log_info("Moving to the button!");
                path
            } else {
                api.log_warn("Bib bop I can't find path to button");
                return;
            }
        } else {
            api.log_warn("Bib bop I can't find path to target");
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
    let interaction = api.map.cell_at_pos(target).unwrap().interaction();
    if dist < ninja_clown.properties().activate_range()
        && !data.activated
        && interaction != InteractionKind::NoInteraction
        && data.is_going_to_button
    {
        api.commit_decisions(&[Decision::activate(target.column(), target.line()).commit(data.ninja_handle)]);
        data.activated = true;
        api.log_info("I push button! Bop!");
    } else if dist < 0.5 {
        if let Some(next_target) = data.path.next() {
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
