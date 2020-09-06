use crate::{decision::DecisionCommit, Entities, LogLevel, Map, RawApi, map::CellPos};
use ninja_clown_bot_sys::{nnj_decision_commit, nnj_log_level};
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

    pub fn log_trace<S: Into<String>>(&self, s: S) {
        self.log(LogLevel::Trace, s);
    }

    pub fn log_debug<S: Into<String>>(&self, s: S) {
        self.log(LogLevel::Debug, s);
    }

    pub fn log_info<S: Into<String>>(&self, s: S) {
        self.log(LogLevel::Info, s);
    }

    pub fn log_warn<S: Into<String>>(&self, s: S) {
        self.log(LogLevel::Warn, s);
    }

    pub fn log_error<S: Into<String>>(&self, s: S) {
        self.log(LogLevel::Error, s);
    }

    pub fn log_critical<S: Into<String>>(&self, s: S) {
        self.log(LogLevel::Critical, s);
    }

    pub fn target_position(&self) -> CellPos {
        let pos = unsafe { (self.raw.target_position.unwrap())(self.raw.ninja_descriptor) };
        CellPos::from(pos)
    }

    pub fn map_update(&mut self) -> usize {
        let n = unsafe {
            (self.raw.map_update.unwrap())(
                self.raw.ninja_descriptor,
                self.map.as_mut_ptr(),
                std::ptr::null_mut(),
                0,
            )
        };
        self.map.changed = n != 0;
        n
    }

    pub fn entities_update(&mut self) -> usize {
        let n = unsafe { (self.raw.entities_update.unwrap())(self.raw.ninja_descriptor, self.entities.as_mut_ptr()) };
        self.entities.changed = n != 0;
        n
    }

    pub fn commit_decisions(&mut self, commits: &[DecisionCommit]) {
        unsafe {
            (self.raw.commit_decisions.unwrap())(
                self.raw.ninja_descriptor,
                commits.as_ptr() as *mut nnj_decision_commit,
                commits.len(),
            )
        }
    }
}
