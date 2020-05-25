pub mod bot;
pub mod path;

use crate::bot::UserData;
use ninja_clown_bot::{Api, RawApi};
use std::mem::MaybeUninit;

static mut API: MaybeUninit<Api> = MaybeUninit::uninit();
static mut DATA: MaybeUninit<UserData> = MaybeUninit::uninit();

/// # SAFETY
/// API should be initialized before calling this function
unsafe fn get_api() -> &'static mut Api {
    &mut *API.as_mut_ptr()
}

/// # SAFETY
/// DATA should be initialized before calling this function
unsafe fn get_data() -> &'static mut UserData {
    &mut *DATA.as_mut_ptr()
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "C" fn bot_init() {
    bot::init();
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "C" fn bot_start_level(api: RawApi) {
    let bot = Api::new(api);
    API.as_mut_ptr().write(bot);

    let user_data = bot::start_level(get_api());

    DATA.as_mut_ptr().write(user_data);
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "C" fn bot_think() {
    let bot = get_api();
    bot.entities_update();
    bot.map_update();
    bot::think(bot, get_data())
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "C" fn bot_end_level() {
    bot::end_level(get_api(), get_data())
}

/// # Safety
/// Called by NinjaClown game
#[no_mangle]
pub unsafe extern "C" fn bot_destroy() {
    bot::destroy(get_api())
}
