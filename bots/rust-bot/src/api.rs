use crate::ffi::{Entity, RawApi};
use crate::map::Map;
use std::ffi::CString;

pub struct Bot {
    pub map: Map,
    pub entities: Vec<Entity>,

    raw: RawApi,
}

impl Bot {
    pub fn new(raw: RawApi) -> Self {
        let map = Map::new(&raw);

        let max_entities = unsafe { (raw.max_entities)() };
        let mut entities = Vec::new();
        entities.resize_with(max_entities, Entity::default);

        Self { map, entities, raw }
    }

    pub fn log<S: Into<String>>(&self, s: S) {
        let s = CString::new(s.into()).expect("CString::new failed");
        unsafe { (self.raw.log)(s.as_ptr()) };
    }

    pub fn map_update(&mut self) {
        unsafe { (self.raw.map_update)(self.raw.ninja_descriptor, self.map.as_mut_ptr()) }
    }

    pub fn entities_update(&mut self) {
        unsafe { (self.raw.entities_update)(self.raw.ninja_descriptor, self.entities.as_mut_ptr()) }
    }

    pub fn get_angle(&self) -> f32 {
        unsafe { (self.raw.get_angle)(self.raw.ninja_descriptor) }
    }

    pub fn get_x_position(&self) -> f32 {
        unsafe { (self.raw.get_x_position)(self.raw.ninja_descriptor) }
    }

    pub fn get_y_position(&self) -> f32 {
        unsafe { (self.raw.get_y_position)(self.raw.ninja_descriptor) }
    }

    pub fn turn_left(&mut self) {
        unsafe { (self.raw.turn_left)(self.raw.ninja_descriptor) }
    }

    pub fn turn_right(&mut self) {
        unsafe { (self.raw.turn_right)(self.raw.ninja_descriptor) }
    }

    pub fn move_forward(&mut self) {
        unsafe { (self.raw.move_forward)(self.raw.ninja_descriptor) }
    }

    pub fn move_backward(&mut self) {
        unsafe { (self.raw.move_backward)(self.raw.ninja_descriptor) }
    }

    pub fn move_toward(&mut self, x: f32, y: f32) {
        let angle = self.get_angle();

        let delta_x = x - self.get_x_position();
        let delta_y = self.get_y_position() - y;
        let target_angle = delta_y.atan2(delta_x);

        let mut delta_angle = angle - target_angle;
        while delta_angle > 2.0 * std::f32::consts::PI {
            delta_angle -= 2.0 * std::f32::consts::PI;
        }
        while delta_angle < 0.0 {
            delta_angle += 2.0 * std::f32::consts::PI;
        }

        if delta_angle.abs() > 0.20 {
            if delta_angle < std::f32::consts::PI {
                self.turn_right()
            } else {
                self.turn_left()
            }
        } else {
            self.move_forward();
        }
    }

    pub fn activate_button(&mut self) {
        unsafe { (self.raw.activate_button)(self.raw.ninja_descriptor) }
    }
}
