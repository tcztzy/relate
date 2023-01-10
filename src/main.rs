use clap::{Parser, Subcommand};
use std::path::PathBuf;

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
        dist: PathBuf,
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
    /// Use after MakeChunks to paint all haps against all.
    Paint {
        /// Index of chunk. (Use when running parts of the algorithm on an individual chunk.)
        #[arg(long, value_name = "INT")]
        chunk_index: usize,
        /// Filename of output without file extension.
        #[arg(short, long, value_name = "PATH")]
        output: PathBuf,
        /// Copying and transition parameters in chromosome painting algorithm. Format: theta rho
        #[arg(long, value_name = "INT", num_args = 2, default_values = ["0.01", "1."])]
        painting: Vec<f64>,
    },
    BuildTopology {
        /// Index of chunk. (Use when running parts of the algorithm on an individual chunk.)
        #[arg(long, value_name = "INT")]
        chunk_index: usize,
        /// Filename of output without file extension.
        #[arg(short, long, value_name = "PATH")]
        output: PathBuf,
        /// Index of first section to infer. (Use when running parts of algorithm on an individual chunk.)
        #[arg(long, requires = "last_section", value_name = "INT")]
        first_section: Option<usize>,
        /// Index of last section to infer. (Use when running parts of algorithm on an individual chunk.)
        #[arg(long, requires = "first_section", value_name = "INT")]
        last_section: Option<usize>,
        /// Effective population size.
        #[arg(short = 'N', long = "efficientN", value_name = "INT", default_value = "0")]
        efficient_population_size: i32,
        /// Copying and transition parameters in chromosome painting algorithm. Format: theta rho
        #[arg(long, value_name = "INT", num_args = 2, default_values = ["0.01", "1."])]
        painting: Vec<f64>,
        /// Seed for MCMC in branch lengths estimation.
        #[arg(long, value_name = "INT")]
        seed: Option<u64>,
        /// Filename of file containing sample ages (one per line).
        #[arg(long, value_name = "PATH")]
        sample_ages: Option<PathBuf>,
        /// Force build a new tree every x bases.
        #[arg(long, value_name = "FLOAT", default_value = "0")]
        fb: i32,
        /// Specify if ancestral allele is unknown.
        #[arg(long, default_value = "false")]
        anc_allele_unknown: bool,
    },
}

fn main() -> miette::Result<()> {
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
            relate::pipelines::make_chunks(haps, sample, map, dist, output, transversion, memory)?;
        }
        Mode::Paint {
            chunk_index,
            output,
            painting,
        } => {
            relate::pipelines::paint(&output, chunk_index, painting)?;
        }
        Mode::BuildTopology {
            chunk_index,
            output,
            first_section,
            last_section,
            efficient_population_size,
            painting,
            seed,
            anc_allele_unknown,
            sample_ages,
            fb,
        } => {
            let section_slice = if let Some(first_section) = first_section {
                Some((first_section, last_section.unwrap()))
            } else {
                None
            };
            relate::pipelines::build_topology(
                &output,
                chunk_index,
                section_slice,
                efficient_population_size,
                Some(painting),
                seed,
                anc_allele_unknown,
                sample_ages.as_ref(),
                fb,
            )?;
        }
    }
    Ok(())
}
