use crate::api::Bot;
use crate::map::path;
use pathfinding::utils::absdiff;

#[derive(Debug)]
pub struct UserData {
    target: path::Pos,
    button: path::Pos,
    path: Vec<path::Pos>,
    path_idx: usize,
}

pub fn init() {}

pub fn start_level(bot: &mut Bot) -> UserData {
    bot.log("start_level");
    bot.log(bot.map.to_string());

    UserData {
        target: path::Pos(2, 1),
        button: path::Pos(6, 1),
        path: Vec::new(),
        path_idx: 0,
    }
}

pub fn think(bot: &mut Bot, data: &mut UserData) {
    let bot_pos = (bot.get_x_position(), bot.get_y_position());

    if data.path.is_empty() {
        let start = path::Pos(bot_pos.0 as usize, bot_pos.1 as usize);
        let graph = bot.map.build_path_graph();

        data.path = if let Some(path) = graph.path_to(&start, &data.target) {
            bot.log("Moving to the target!");
            path
        } else if let Some(path) = graph.path_to(&start, &data.button) {
            bot.log("Moving to the button!");
            path
        } else {
            bot.log("Bib bop I'm stuck");
            return;
        };

        data.path_idx = 0;
    }

    let next_target = &data.path[data.path_idx];

    let dist =
        absdiff(next_target.center_x(), bot_pos.0) + absdiff(next_target.center_y(), bot_pos.1);
    if dist < 0.4 {
        data.path_idx += 1;

        if data.path_idx >= data.path.len() {
            data.path_idx = 0;
            data.path.clear();
            bot.activate_button();
        } else {
            think(bot, data);
        }
    } else {
        bot.move_toward(next_target.center_x(), next_target.center_y());
    }
}

pub fn end_level(_bot: &mut Bot, _data: &mut UserData) {}

pub fn destroy(_bot: &mut Bot) {}
