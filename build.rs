fn main() -> miette::Result<()> {
    let build_path = std::path::PathBuf::from(std::env::var("OUT_DIR").unwrap());
    let build_path = build_path.to_str().unwrap();
    meson::build(".", build_path);
    let src_path = std::path::PathBuf::from("src");
    let gzstream_path = std::path::PathBuf::from("subprojects/gzstream");
    let mut b = autocxx_build::Builder::new("src/lib.rs", &[&src_path, &gzstream_path]).build()?;
    b.flag_if_supported("-std=c++17")
        .files(["src/data.cpp", "src/fast_painting.cpp"])
        .compile("relate");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/data.cpp");
    println!("cargo:rerun-if-changed=src/data.hpp");
    println!("cargo:rerun-if-changed=src/fast_painting.cpp");
    println!("cargo:rerun-if-changed=src/fast_painting.hpp");
    Ok(())
}
