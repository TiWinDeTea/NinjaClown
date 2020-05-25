pub mod api;
pub mod decision;
pub mod entity;
pub mod map;

pub use api::Api;
pub use decision::Decision;
pub use entity::{Entities, Entity};
pub use map::Map;

pub type RawApi = ninja_clown_bot_sys::nnj_api;

#[derive(Clone, Debug, Copy)]
#[repr(u32)]
pub enum LogLevel {
    Trace = ninja_clown_bot_sys::nnj_log_level::LL_TRACE.0,
    Debug = ninja_clown_bot_sys::nnj_log_level::LL_DEBUG.0,
    Info = ninja_clown_bot_sys::nnj_log_level::LL_INFO.0,
    Warn = ninja_clown_bot_sys::nnj_log_level::LL_WARN.0,
    Error = ninja_clown_bot_sys::nnj_log_level::LL_ERROR.0,
    Critical = ninja_clown_bot_sys::nnj_log_level::LL_CRITICAL.0,
}

impl From<LogLevel> for ninja_clown_bot_sys::nnj_log_level {
    fn from(ll: LogLevel) -> Self {
        ninja_clown_bot_sys::nnj_log_level(ll as u32)
    }
}
