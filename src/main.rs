use autocxx::prelude::*;
use clap::{Parser, Subcommand};
use cxx::let_cxx_string;
use std::path::PathBuf;

include_cpp! {
    #include "data.hpp"
    #include "usage.hpp"
    safety!(unsafe_ffi)
    generate!("Data")
    generate!("ResourceUsage")
}

#[derive(Parser, Debug)]
struct Args {
    #[command(subcommand)]
    mode: Mode,
}

#[derive(Subcommand, Debug)]
enum Mode {
    All,
    /// Use to make smaller chunks from the data.
    MakeChunks {
        /// Filename of haps file (Output file format of Shapeit).
        #[arg(long, value_name = "FILE")]
        haps: PathBuf,
        /// Filename of sample file (Output file format of Shapeit).
        #[arg(long, value_name = "FILE")]
        sample: PathBuf,
        /// Genetic map.
        #[arg(long, value_name = "FILE")]
        map: PathBuf,
        /// Optional but recommended. Distance in BP between SNPs. Can be generated using RelateFileFormats. If unspecified, distances in haps are used.
        #[arg(long, value_name = "FILE", default_value = "unspecified")]
        dist: String,
        /// Filename of output without file extension.
        #[arg(short, long, value_name = "PATH")]
        output: PathBuf,
        /// Only use transversion for bl estimation.
        #[arg(long, default_value = "false")]
        transversion: bool,
        /// Optional. Approximate memory allowance in GB for storing distance matrices. Default is 5GB.
        #[arg(long, value_name = "FLOAT", default_value = "5")]
        memory: f32,
    },
}

fn main() -> miette::Result<()> {
    moveit! {
        let mut data = ffi::Data::new();
    }
    let args = Args::parse();
    match args.mode {
        Mode::All => {}
        Mode::MakeChunks {
            haps,
            sample,
            map,
            dist,
            output,
            transversion,
            memory,
        } => {
            eprintln!("---------------------------------------------------------");
            eprintln!("Parsing data..");
            if output.exists() {
                panic!("Directory {} already exists. Relate will use this directory to store temporary files.", output.display());
            }
            std::fs::create_dir(&output).expect("Fail to create output directory");
            let_cxx_string!(filename_haps = haps.to_str().unwrap());
            let_cxx_string!(filename_sample = sample.to_str().unwrap());
            let_cxx_string!(filename_map = map.to_str().unwrap());
            let_cxx_string!(filename_dist = dist);
            let_cxx_string!(filename_out = output.to_str().unwrap());
            data.as_mut().MakeChunks(
                &filename_haps,
                &filename_sample,
                &filename_map,
                &filename_dist,
                &filename_out,
                !transversion,
                memory,
            );
            ffi::ResourceUsage();
        }
    }
    Ok(())
}
