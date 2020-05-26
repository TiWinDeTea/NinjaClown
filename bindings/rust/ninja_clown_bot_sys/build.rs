use std::{env, path::PathBuf};

fn main() {
    println!("cargo:rerun-if-changed=wrapper.h");

    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .newtype_enum("nnj_.*")
        .derive_default(true)
        .derive_eq(true)
        .derive_partialeq(true)
        .derive_hash(true)
        .derive_ord(true)
        .derive_partialord(true)
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .size_t_is_usize(true) // we don't expect to target platform where size_t != uintptr_t
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
