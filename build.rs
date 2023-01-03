fn main() -> miette::Result<()> {
    let build_path = std::path::PathBuf::from(std::env::var("OUT_DIR").unwrap());
    let build_path = build_path.to_str().unwrap();
    meson::build(".", build_path);
    let src_path = std::path::PathBuf::from("src");
    let gzstream_path = std::path::PathBuf::from("subprojects/gzstream");
    let mut b = autocxx_build::Builder::new("src/main.rs", &[&src_path, &gzstream_path]).build()?;
    b.flag_if_supported("-std=c++17")
        .file("src/data.cpp")
        .compile("relate");
    println!("cargo:rerun-if-changed=src/main.rs");
    println!("cargo:rerun-if-changed=src/data.cpp");
    println!("cargo:rerun-if-changed=src/data.hpp");
    Ok(())
}
