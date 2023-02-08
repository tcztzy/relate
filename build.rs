fn main() -> miette::Result<()> {
    let build_path = std::path::PathBuf::from(std::env::var("OUT_DIR").unwrap());
    let build_path = build_path.to_str().unwrap();
    meson::build(".", build_path);
    let src_path = std::path::PathBuf::from("src");
    let gzstream_path = std::path::PathBuf::from("subprojects/gzstream");
    let mut b = autocxx_build::Builder::new("src/lib.rs", &[&src_path, &gzstream_path]).build()?;
    b.flag_if_supported("-std=c++17")
        .files([
            "src/anc.cpp",
            "src/anc_builder.cpp",
            "src/data.cpp",
            "src/fast_painting.cpp",
            "src/mutations.cpp",
            "src/tree_builder.cpp",
            "src/branch_length_estimator.cpp",
            "subprojects/gzstream/gzstream.C",
            "pipeline/FindEquivalentBranches.cpp",
            "pipeline/InferBranchLengths.cpp",
            "pipeline/CombineSections.cpp",
        ])
        .compile("relate");
    println!("cargo:rustc-link-lib=z");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/anc.cpp");
    println!("cargo:rerun-if-changed=src/anc.hpp");
    println!("cargo:rerun-if-changed=src/anc_builder.cpp");
    println!("cargo:rerun-if-changed=src/anc_builder.hpp");
    println!("cargo:rerun-if-changed=src/data.cpp");
    println!("cargo:rerun-if-changed=src/data.hpp");
    println!("cargo:rerun-if-changed=src/fast_painting.cpp");
    println!("cargo:rerun-if-changed=src/fast_painting.hpp");
    println!("cargo:rerun-if-changed=src/mutations.cpp");
    println!("cargo:rerun-if-changed=src/mutations.hpp");
    println!("cargo:rerun-if-changed=src/tree_builder.cpp");
    println!("cargo:rerun-if-changed=src/tree_builder.hpp");
    println!("cargo:rerun-if-changed=src/branch_length_estimator.cpp");
    println!("cargo:rerun-if-changed=src/branch_length_estimator.hpp");
    println!("cargo:rerun-if-changed=pipeline/FindEquivalentBranches.cpp");
    println!("cargo:rerun-if-changed=pipeline/InferBranchLengths.cpp");
    println!("cargo:rerun-if-changed=pipeline/CombineSections.cpp");
    Ok(())
}
