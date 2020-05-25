use crate::{decision::DecisionCommit, Entities, LogLevel, Map, RawApi};
use ninja_clown_bot_sys::{nnj_cell, nnj_decision_commit, nnj_entity, nnj_log_level, size_t};
use std::ffi::CString;

pub struct Api {
    pub map: Map,
    pub entities: Entities,

    raw: RawApi,
}

impl Api {
    pub fn new(raw: RawApi) -> Self {
        Self {
            map: Map::new(&raw),
            entities: Entities::new(&raw),
            raw,
        }
    }

    pub fn log<S: Into<String>>(&self, level: LogLevel, s: S) {
        let s = CString::new(s.into()).expect("CString::new failed");
        unsafe { (self.raw.log.unwrap())(nnj_log_level::from(level), s.as_ptr()) };
    }

    // TODO: add trace, etc methods

    pub fn map_update(&mut self) -> u64 {
        unsafe {
            (self.raw.map_update.unwrap())(
                self.raw.ninja_descriptor,
                self.map.as_mut_ptr() as *mut nnj_cell,
                std::ptr::null_mut(),
                0,
            )
        }
    }

    pub fn entities_update(&mut self) -> u64 {
        unsafe {
            (self.raw.entities_update.unwrap())(
                self.raw.ninja_descriptor,
                self.entities.as_mut_ptr() as *mut nnj_entity,
            )
        }
    }

    pub fn commit_decisions(&self, commits: &[DecisionCommit]) {
        unsafe {
            (self.raw.commit_decisions.unwrap())(
                self.raw.ninja_descriptor,
                commits.as_ptr() as *mut nnj_decision_commit,
                commits.len() as size_t,
            )
        }
    }
}
