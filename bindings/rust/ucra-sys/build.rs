use std::{env, path::PathBuf};

fn main() {
    // Tell cargo to rerun if the header changes
    println!("cargo:rerun-if-changed=../../include/ucra/ucra.h");

    // Where the compiled C library lives; provided by CMake via env
    let lib_dir = env::var("UCRA_LIB_DIR").unwrap_or_else(|_| "../../build".into());

    // Link search path and library name (shared or static both named ucra_impl)
    println!("cargo:rustc-link-search=native={}", lib_dir);

    // On Unix, also link against m and pthread which CMake may have linked
    #[cfg(target_family = "unix")]
    {
        println!("cargo:rustc-link-lib=dylib=ucra_impl");
        println!("cargo:rustc-link-lib=dylib=m");
        println!("cargo:rustc-link-lib=dylib=pthread");
    }

    #[cfg(target_family = "windows")]
    {
        // On Windows, link to import library name if present
        println!("cargo:rustc-link-lib=dylib=ucra_impl");
    }

    // Generate bindings
    let header = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
        .join("../../../include/ucra/ucra.h")
        .canonicalize()
        .expect("ucra.h not found");
    let include_dir = header.parent().unwrap().parent().unwrap().to_path_buf();
    let bindings = bindgen::Builder::default()
        .header(header.to_string_lossy())
        .allowlist_type("UCRA_.*")
        .allowlist_function("ucra_.*")
        .allowlist_var("UCRA_.*")
        .clang_arg(format!("-I{}", include_dir.display()))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
