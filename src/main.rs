use clap::{Parser, Subcommand};
use std::path::PathBuf;

#[derive(Parser, Debug)]
struct Args {
    #[command(subcommand)]
    mode: Mode,
}

#[derive(Subcommand, Debug)]
enum Mode {
    /// Executes all stages of the algorithm.
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
    /// Use after Paint to build tree topologies in a small chunk specified by first section - last_section.
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
        #[arg(
            short = 'N',
            long = "efficientN",
            value_name = "INT",
            default_value = "0"
        )]
        effective_population_size: i32,
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
        #[arg(long, value_name = "FLOAT", default_value_t = 0)]
        fb: i32,
        /// Specify if ancestral allele is unknown.
        #[arg(long, default_value_t = false)]
        anc_allele_unknown: bool,
    },
    /// Use after BuildTopology to find equivalent branches in adjacent trees. Output written as bin file.
    FindEquivalentBranches {
        /// Filename of output without file extension.
        #[arg(short, long, value_name = "PATH")]
        output: PathBuf,
        /// Index of chunk. (Use when running parts of the algorithm on an individual chunk.)
        #[arg(long, value_name = "INT")]
        chunk_index: usize,
    },
    /// Use after FindEquivalentBranches to infer branch lengths.
    InferBranchLengths {
        /// Filename of output without file extension.
        #[arg(short, long, value_name = "PATH")]
        output: PathBuf,
        /// Index of chunk. (Use when running parts of the algorithm on an individual chunk.)
        #[arg(long, value_name = "INT")]
        chunk_index: usize,
        /// Index of first section to infer. (Use when running parts of algorithm on an individual chunk.)
        #[arg(long, requires = "last_section", value_name = "INT")]
        first_section: Option<usize>,
        /// Index of last section to infer. (Use when running parts of algorithm on an individual chunk.)
        #[arg(long, requires = "first_section", value_name = "INT")]
        last_section: Option<usize>,
        /// Mutation rate.
        #[arg(long, value_name = "FLOAT")]
        mutation_rate: f64,
        /// Effective population size.
        #[arg(
            short = 'N',
            long,
            visible_alias = "effectiveN",
            value_name = "FLOAT",
            required_unless_present = "coal",
            default_value_t = 30000.
        )]
        effective_population_size: f64,
        /// Filename of file containing sample ages (one per line).
        #[arg(long, value_name = "PATH")]
        sample_ages: Option<PathBuf>,
        /// Filename of file containing coalescent rates. If specified, it will overwrite --effectiveN.
        #[arg(long, value_name = "FILE")]
        coal: Option<PathBuf>,
        /// Seed for MCMC in branch lengths estimation.
        #[arg(long, value_name = "INT")]
        seed: Option<u64>,
    },
    /// Use after InferBranchLengths to combine files containing trees in small chunks to one file for a section.
    CombineSections {
        /// Filename of output without file extension.
        #[arg(short, long, value_name = "PATH")]
        output: PathBuf,
        /// Index of chunk. (Use when running parts of the algorithm on an individual chunk.)
        #[arg(long, value_name = "INT")]
        chunk_index: usize,
        /// Effective population size.
        #[arg(
            short = 'N',
            long,
            visible_alias = "effectiveN",
            value_name = "FLOAT",
            required_unless_present = "coal",
            default_value_t = 30000.
        )]
        effective_population_size: f64,
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
            effective_population_size,
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
                effective_population_size,
                Some(painting),
                seed,
                anc_allele_unknown,
                sample_ages.as_ref(),
                fb,
            )?;
        }
        Mode::FindEquivalentBranches {
            output,
            chunk_index,
        } => {
            relate::pipelines::find_equivalent_branches(&output, chunk_index)?;
        }
        Mode::InferBranchLengths {
            output,
            chunk_index,
            first_section,
            last_section,
            mutation_rate,
            effective_population_size,
            sample_ages,
            coal,
            seed,
        } => {
            let section_slice = if let Some(first_section) = first_section {
                Some((first_section, last_section.unwrap()))
            } else {
                None
            };
            relate::pipelines::infer_branch_lengths(
                &output,
                chunk_index,
                section_slice,
                mutation_rate,
                Some(effective_population_size),
                sample_ages.as_ref(),
                coal.as_ref(),
                seed,
            )?;
        },
        Mode::CombineSections { output, chunk_index, effective_population_size } => {
            relate::pipelines::combine_sections(&output, chunk_index, effective_population_size)?;
        }
    }
    Ok(())
}
