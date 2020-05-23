use crate::api::Bot;
use crate::bot;
use crate::bot::UserData;
use std::ffi::c_void;
use std::mem::MaybeUninit;
use std::os::raw::c_char;

static mut BOT: MaybeUninit<Bot> = MaybeUninit::uninit();
static mut DATA: MaybeUninit<UserData> = MaybeUninit::uninit();

/// # SAFETY
/// BOT should be initialized before calling this function
unsafe fn get_bot() -> &'static mut Bot {
    &mut *BOT.as_mut_ptr()
}

/// # SAFETY
/// DATA should be initialized before calling this function
unsafe fn get_data() -> &'static mut UserData {
    &mut *DATA.as_mut_ptr()
}

#[repr(C)]
pub struct RawApi {
    pub ninja_descriptor: *mut c_void,

    pub log: unsafe extern "sysv64" fn(*const c_char),

    pub map_width: unsafe extern "sysv64" fn(ninja_data: *mut c_void) -> usize,
    pub map_height: unsafe extern "sysv64" fn(ninja_data: *mut c_void) -> usize,
    pub map_scan: unsafe extern "sysv64" fn(ninja_data: *mut c_void, map_view: *mut Cell),
    pub map_update: unsafe extern "sysv64" fn(ninja_data: *mut c_void, map_view: *mut Cell),
    pub max_entities: unsafe extern "sysv64" fn() -> usize,
    pub entities_update: unsafe extern "sysv64" fn(ninja_data: *mut c_void, entities: *mut Entity),

    pub get_angle: unsafe extern "sysv64" fn(ninja_data: *mut c_void) -> f32,
    pub get_x_position: unsafe extern "sysv64" fn(ninja_data: *mut c_void) -> f32,
    pub get_y_position: unsafe extern "sysv64" fn(ninja_data: *mut c_void) -> f32,

    pub turn_left: unsafe extern "sysv64" fn(ninja_data: *mut c_void),
    pub turn_right: unsafe extern "sysv64" fn(ninja_data: *mut c_void),
    pub move_forward: unsafe extern "sysv64" fn(ninja_data: *mut c_void),
    pub move_backward: unsafe extern "sysv64" fn(ninja_data: *mut c_void),

    pub activate_button: unsafe extern "sysv64" fn(ninja_data: *mut c_void),
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum CellType {
    Unknown = 0,
    Chasm = 1,
    Ground = 2,
    Wall = 3,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum InteractionKind {
    NoInteraction = 0,
    LightManual = 1,
    HeavyManual = 2,
    LightMidair = 3,
    HeavyMidair = 4,
    WalkOnGround = 5,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct Cell {
    pub typ: CellType,
    pub interaction: InteractionKind,
}

impl Default for Cell {
    fn default() -> Self {
        Self {
            typ: CellType::Unknown,
            interaction: InteractionKind::NoInteraction,
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum EntityKind {
    NotAnEntity = 0,
    Harmless = 1,
    Patrol = 2,
    Aggressive = 3,
    Projectile = 4,
    Dll = 5,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct Entity {
    pub typ: EntityKind,
    pub x: f32,
    pub y: f32,
    pub angle: f32,
}

impl Default for Entity {
    fn default() -> Self {
        Self {
            typ: EntityKind::NotAnEntity,
            x: 0.0,
            y: 0.0,
            angle: 0.0,
        }
    }
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "sysv64" fn bot_init() {
    bot::init();
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "sysv64" fn bot_start_level(api: RawApi) {
    let bot = Bot::new(api);
    BOT.as_mut_ptr().write(bot);

    let user_data = bot::start_level(get_bot());

    DATA.as_mut_ptr().write(user_data);
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "sysv64" fn bot_think() {
    let bot = get_bot();
    bot.entities_update();
    bot.map_update();
    bot::think(bot, get_data())
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "sysv64" fn bot_end_level() {
    bot::end_level(get_bot(), get_data())
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "sysv64" fn bot_destroy() {
    bot::destroy(get_bot())
}
