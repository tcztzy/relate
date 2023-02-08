use crate::{ffi, resource_usage};
use autocxx::{c_int, prelude::moveit};
use byteorder::{NativeEndian, ReadBytesExt};
use clap::Parser;
use cxx::{let_cxx_string, CxxString};
use miette::{IntoDiagnostic, Result};
use rand::{rngs::StdRng, Rng, SeedableRng};
use std::io::{BufRead, Read, Write};
use std::path::PathBuf;

#[derive(Debug)]
enum PipelineError {
    ValueError,
}
impl std::fmt::Display for PipelineError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}
impl std::error::Error for PipelineError {}
impl miette::Diagnostic for PipelineError {}

struct Parameters {
    num_samples: usize,
    num_alleles: usize,
    num_windows: usize,
    window_boundaries: Vec<usize>,
}

fn read_parameters_bin(parameters: &PathBuf) -> miette::Result<Parameters> {
    let mut file = std::fs::File::open(parameters).into_diagnostic()?;
    let mut buf = Vec::new();
    let _n = file.read_to_end(&mut buf).into_diagnostic()?;
    let mut rdr = std::io::Cursor::new(&buf[..]);
    let num_samples = rdr.read_i32::<NativeEndian>().into_diagnostic()? as usize;
    let num_alleles = rdr.read_i32::<NativeEndian>().into_diagnostic()? as usize;
    let num_windows = rdr.read_i32::<NativeEndian>().into_diagnostic()? as usize - 1;
    let mut window_boundaries = Vec::new();
    window_boundaries.resize(num_windows + 1, 0);
    rdr.read_i32_into::<NativeEndian>(&mut window_boundaries)
        .into_diagnostic()?;
    Ok(Parameters {
        num_samples,
        num_alleles,
        num_windows,
        window_boundaries: window_boundaries
            .into_iter()
            .map(|wb| wb as usize)
            .collect(),
    })
}

/// Use to make smaller chunks from the data.
#[derive(Parser, Debug)]
pub struct MakeChunks {
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
}

impl MakeChunks {
    pub fn execute(&self) -> Result<()> {
        eprintln!("---------------------------------------------------------");
        eprintln!("Parsing data..");
        if self.output.exists() {
            panic!(
                "Directory {} already exists. Relate will use this directory to store temporary files.",
                self.output.display()
            );
        }
        std::fs::create_dir(&self.output).expect("Fail to create output directory");
        let_cxx_string!(filename_haps = self.haps.to_str().unwrap());
        let_cxx_string!(filename_sample = self.sample.to_str().unwrap());
        let_cxx_string!(filename_map = self.map.to_str().unwrap());
        let_cxx_string!(filename_dist = self.dist.to_str().unwrap());
        let_cxx_string!(filename_out = self.output.to_str().unwrap());
        moveit! {
            let mut data = ffi::Data::new();
        }
        data.as_mut().MakeChunks(
            &filename_haps,
            &filename_sample,
            &filename_map,
            &filename_dist,
            &filename_out,
            !self.transversion,
            self.memory,
        );
        resource_usage();
        Ok(())
    }
}

/// Use after [MakeChunks] to paint all haps against all.
#[derive(Parser, Debug)]
pub struct Paint {
    /// Index of chunk. (Use when running parts of the algorithm on an individual chunk.)
    #[arg(long, value_name = "INT")]
    chunk_index: usize,
    /// Filename of output without file extension.
    #[arg(short, long, value_name = "PATH")]
    output: PathBuf,
    /// Copying and transition parameters in chromosome painting algorithm. Format: theta rho
    #[arg(long, value_name = "INT", num_args = 2, default_values = ["0.01", "1."])]
    painting: Vec<f64>,
}

impl Paint {
    pub fn execute(&self) -> Result<()> {
        let parameters = read_parameters_bin(
            &self
                .output
                .join(format!("parameters_c{}.bin", self.chunk_index)),
        )?;
        let num_samples = parameters.num_samples;
        let num_windows = parameters.num_windows;
        let window_boundaries: Vec<c_int> = parameters
            .window_boundaries
            .into_iter()
            .map(|wb| c_int(wb as i32))
            .collect();
        eprintln!("---------------------------------------------------------");
        eprintln!("Painting sequences...");
        let basename = self.output.join(format!("chunk_{}", self.chunk_index));
        let hap = std::ffi::CString::new(basename.with_extension("hap").to_str().unwrap())
            .into_diagnostic()?;
        let pos = std::ffi::CString::new(basename.with_extension("bp").to_str().unwrap())
            .into_diagnostic()?;
        let dist = std::ffi::CString::new(basename.with_extension("dist").to_str().unwrap())
            .into_diagnostic()?;
        let rec = std::ffi::CString::new(basename.with_extension("r").to_str().unwrap())
            .into_diagnostic()?;
        let rpos = std::ffi::CString::new(basename.with_extension("rpos").to_str().unwrap())
            .into_diagnostic()?;
        let state = std::ffi::CString::new(basename.with_extension("state").to_str().unwrap())
            .into_diagnostic()?;
        moveit! {
            let mut data = unsafe { ffi::Data::new2(hap.as_ptr(), pos.as_ptr(), dist.as_ptr(), rec.as_ptr(), rpos.as_ptr(), state.as_ptr(), c_int(30000), 1.25e-8) };
        }
        data.as_mut()
            .SetPainting(self.painting[0], self.painting[1]);
        eprintln!("---------------------------------------------------------");
        eprintln!("Painting sequences...");
        let chunk_dir = self.output.join(format!("chunk_{}", self.chunk_index));
        let paint_dir = chunk_dir.join("paint");
        std::fs::create_dir_all(&paint_dir).into_diagnostic()?;
        for path in std::fs::read_dir(&paint_dir).unwrap() {
            let path = path.unwrap().path();
            let extension = path.extension().unwrap();
            if extension == std::ffi::OsStr::new("bin") {
                std::fs::remove_file(path).into_diagnostic()?;
            }
        }
        let data_basename = paint_dir.join("relate");
        for hap in 0..num_samples as i32 {
            moveit! {
                let mut painter = ffi::FastPainting::new(num_samples as usize, 0.001);
            }
            let basename =
                std::ffi::CString::new(data_basename.to_str().unwrap()).into_diagnostic()?;
            unsafe {
                painter.as_mut().PaintSteppingStones1(
                    &data,
                    basename.as_ptr(),
                    num_windows,
                    window_boundaries.as_ptr(),
                    c_int(hap),
                );
            }
        }
        resource_usage();
        Ok(())
    }
}

/// Use after Paint to build tree topologies in a small chunk specified by first section - last_section.
#[derive(Parser, Debug)]
pub struct BuildTopology {
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
        long,
        visible_alias = "effectiveN",
        value_name = "FLOAT",
        required_unless_present = "coal",
        default_value_t = 30000.
    )]
    effective_population_size: f64,
    /// Copying and transition parameters in chromosome painting algorithm. Format: theta rho
    #[arg(long, value_name = "INT", num_args = 2, default_values = ["0.01", "1."])]
    painting: Vec<f64>,
    /// Seed for MCMC in branch lengths estimation.
    #[arg(long, value_name = "INT")]
    seed: Option<u64>,
    /// Filename of file containing sample ages (one per line).
    #[arg(long, value_name = "FILE")]
    sample_ages: Option<PathBuf>,
    /// Force build a new tree every x bases.
    #[arg(long, value_name = "FLOAT", default_value_t = 0)]
    fb: i32,
    /// Specify if ancestral allele is unknown.
    #[arg(long, default_value_t = false)]
    anc_allele_unknown: bool,
}

impl BuildTopology {
    pub fn execute(&self) -> Result<()> {
        let parameters_path = self
            .output
            .join(format!("parameters_c{}.bin", self.chunk_index));
        let parameters = read_parameters_bin(&parameters_path)?;
        let first_section = self.first_section.unwrap_or(0);
        let last_section = self.last_section.unwrap_or(parameters.num_windows - 1);
        let basename = self.output.join(format!("chunk_{}", self.chunk_index));
        let hap = std::ffi::CString::new(basename.with_extension("hap").to_str().unwrap())
            .into_diagnostic()?;
        let pos = std::ffi::CString::new(basename.with_extension("bp").to_str().unwrap())
            .into_diagnostic()?;
        let dist = std::ffi::CString::new(basename.with_extension("dist").to_str().unwrap())
            .into_diagnostic()?;
        let rec = std::ffi::CString::new(basename.with_extension("r").to_str().unwrap())
            .into_diagnostic()?;
        let rpos = std::ffi::CString::new(basename.with_extension("rpos").to_str().unwrap())
            .into_diagnostic()?;
        let state = std::ffi::CString::new(basename.with_extension("state").to_str().unwrap())
            .into_diagnostic()?;

        if first_section >= parameters.num_windows {
            return Err(PipelineError::ValueError.into());
        }

        moveit! {
            let mut data = unsafe { ffi::Data::new2(hap.as_ptr(), pos.as_ptr(), dist.as_ptr(), rec.as_ptr(), rpos.as_ptr(), state.as_ptr(), c_int(30000), 1.25e-8) };
        }
        let_cxx_string!(name = basename.join("paint/relate").to_str().unwrap());
        data.as_mut().SetName(&name);
        data.as_mut()
            .SetEfficientPopulationSize(c_int(self.effective_population_size as i32 * 50));
        data.as_mut()
            .SetPainting(self.painting[0], self.painting[1]);
        let mut rng: StdRng = if let Some(seed) = self.seed {
            SeedableRng::seed_from_u64(seed)
        } else {
            StdRng::from_entropy()
        };
        let ancestral_state = !self.anc_allele_unknown;
        let sample_ages: Vec<f64> = if let Some(path) = &self.sample_ages {
            let file = std::fs::File::open(path).into_diagnostic()?;
            let rdr = std::io::BufReader::new(file);
            rdr.lines()
                .into_iter()
                .map(|line| line.unwrap().parse::<f64>().unwrap())
                .collect()
        } else {
            Vec::new()
        };
        let mut sample_ages =
            unsafe { ffi::construct_vector_double(sample_ages.as_ptr(), sample_ages.len()) };
        let last_section = std::cmp::min(parameters.num_windows - 1, last_section);

        eprintln!("---------------------------------------------------------");
        eprintln!(
            "Estimating topologies of AncesTrees in sections {}-{}...",
            first_section, last_section
        );

        for section in first_section..(last_section + 1) {
            eprintln!("[{}/{}]", section, last_section);
            std::io::stderr().flush().into_diagnostic()?;
            moveit! {
                let mut anc = ffi::AncesTree::new();
                let mut ancbuiler = ffi::AncesTreeBuilder::new1(data.as_mut(), sample_ages.as_mut().unwrap());
            }
            let section_startpos = parameters.window_boundaries[section];
            let mut section_endpos = parameters.window_boundaries[section + 1] - 1;
            if section_endpos >= parameters.num_alleles {
                section_endpos = parameters.num_alleles - 1;
            }
            ancbuiler.as_mut().BuildTopology(
                c_int(section as i32),
                c_int(section_startpos as i32),
                c_int(section_endpos as i32),
                data.as_mut(),
                anc.as_mut(),
                c_int(rng.gen::<i32>()),
                ancestral_state,
                c_int(self.fb),
            );
            let section_basename = basename.join(format!(
                "{}_{}",
                self.output.file_name().unwrap().to_str().unwrap(),
                section
            ));
            let_cxx_string!(
                anc_filename = section_basename.with_extension("anc").to_str().unwrap()
            );
            let_cxx_string!(
                mut_filename = section_basename.with_extension("mut").to_str().unwrap()
            );
            anc.as_mut().DumpBin1(&anc_filename);
            ancbuiler.as_mut().GetMutations().as_mut().DumpShortFormat1(
                &mut_filename,
                c_int(section_startpos as i32),
                c_int(section_endpos as i32),
            );
        }

        resource_usage();
        Ok(())
    }
}

/// Use after BuildTopology to find equivalent branches in adjacent trees. Output written as bin file.
#[derive(Parser, Debug)]
pub struct FindEquivalentBranches {
    /// Filename of output without file extension.
    #[arg(short, long, value_name = "PATH")]
    output: PathBuf,
    /// Index of chunk. (Use when running parts of the algorithm on an individual chunk.)
    #[arg(long, value_name = "INT")]
    chunk_index: usize,
}

impl FindEquivalentBranches {
    pub fn execute(&self) -> Result<()> {
        ffi::FindEquivalentBranches(
            self.output.to_str().unwrap(),
            c_int(self.chunk_index as i32),
        );
        Ok(())
    }
}
/// Use after FindEquivalentBranches to infer branch lengths.
#[derive(Parser, Debug)]
pub struct InferBranchLengths {
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
    #[arg(long, value_name = "FILE")]
    sample_ages: Option<PathBuf>,
    /// Filename of file containing coalescent rates. If specified, it will overwrite --effectiveN.
    #[arg(long, value_name = "FILE")]
    coal: Option<PathBuf>,
    /// Seed for MCMC in branch lengths estimation.
    #[arg(long, value_name = "INT")]
    seed: Option<u64>,
}

impl InferBranchLengths {
    pub fn execute(&self) -> Result<()> {
        let sample_ages = if let Some(sample_ages) = &self.sample_ages {
            sample_ages.to_str().unwrap().as_ptr()
        } else {
            std::ptr::null()
        } as *const CxxString;
        let coal = if let Some(coal) = &self.coal {
            coal.to_str().unwrap().as_ptr()
        } else {
            std::ptr::null()
        } as *const CxxString;
        let seed = if let Some(seed) = self.seed {
            &c_int(seed as i32)
        } else {
            std::ptr::null()
        };
        let parameters_path = self
            .output
            .join(format!("parameters_c{}.bin", self.chunk_index));
        let parameters = read_parameters_bin(&parameters_path)?;
        let first_section = self.first_section.unwrap_or(0);
        let last_section = self.last_section.unwrap_or(parameters.num_windows - 1);
        unsafe {
            ffi::GetBranchLengths(
                self.output.to_str().unwrap(),
                c_int(self.chunk_index as i32),
                c_int(first_section as i32),
                c_int(last_section as i32),
                self.mutation_rate,
                &self.effective_population_size,
                sample_ages,
                coal,
                seed,
            );
        }
        Ok(())
    }
}
/// Use after InferBranchLengths to combine files containing trees in small chunks to one file for a section.
#[derive(Parser, Debug)]
pub struct CombineSections {
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
}
impl CombineSections {
    pub fn execute(&self) -> Result<()> {
        ffi::CombineSections(
            self.output.to_str().unwrap(),
            c_int(self.chunk_index as i32),
            c_int(self.effective_population_size as i32),
        );
        Ok(())
    }
}

/// Use at the end to finalize results. This will summarize all sections into one file.
#[derive(Parser, Debug)]
pub struct Finalize {
    /// Filename of output without file extension.
    #[arg(short, long, value_name = "PATH")]
    output: PathBuf,
    /// Filename of file containing sample ages (one per line).
    #[arg(long, value_name = "FILE")]
    sample_ages: Option<PathBuf>,
    /// Filename of file containing additional annotation of snps. Can be generated using RelateFileFormats.
    #[arg(long, value_name = "FILE")]
    annot: Option<PathBuf>,
}

impl Finalize {
    pub fn execute(&self) -> Result<()> {
        let sample_ages = if let Some(sample_ages) = &self.sample_ages {
            sample_ages.to_str().unwrap().as_ptr()
        } else {
            std::ptr::null()
        } as *const CxxString;
        let annot = if let Some(annot) = &self.annot {
            annot.to_str().unwrap().as_ptr()
        } else {
            std::ptr::null()
        } as *const CxxString;
        unsafe {
            ffi::Finalize(self.output.to_str().unwrap(), sample_ages, annot);
        }
        Ok(())
    }
}

/// Executes all stages of the algorithm.
#[derive(Parser, Debug)]
pub struct PipelineAll {
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
    /// Index of chunk. (Use when running parts of the algorithm on an individual chunk.)
    #[arg(long, value_name = "INT")]
    chunk_index: Option<usize>,
    /// Copying and transition parameters in chromosome painting algorithm. Format: theta rho
    #[arg(long, value_name = "INT", num_args = 2, default_values = ["0.01", "1."])]
    painting: Vec<f64>,
    /// Index of first section to infer. (Use when running parts of algorithm on an individual chunk.)
    #[arg(long, requires = "last_section", value_name = "INT")]
    first_section: Option<usize>,
    /// Index of last section to infer. (Use when running parts of algorithm on an individual chunk.)
    #[arg(long, requires = "first_section", value_name = "INT")]
    last_section: Option<usize>,
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
    #[arg(long, value_name = "FILE")]
    sample_ages: Option<PathBuf>,
    /// Filename of file containing coalescent rates. If specified, it will overwrite --effectiveN.
    #[arg(long, value_name = "FILE")]
    coal: Option<PathBuf>,
    /// Force build a new tree every x bases.
    #[arg(long, value_name = "FLOAT", default_value_t = 0)]
    fb: i32,
    /// Specify if ancestral allele is unknown.
    #[arg(long, default_value_t = false)]
    anc_allele_unknown: bool,
    /// Filename of file containing additional annotation of snps. Can be generated using RelateFileFormats.
    #[arg(long, value_name = "FILE")]
    annot: Option<PathBuf>,
    /// Mutation rate.
    #[arg(long, value_name = "FLOAT")]
    mutation_rate: f64,
    /// Seed for MCMC in branch lengths estimation.
    #[arg(long, value_name = "INT")]
    seed: Option<u64>,
}

impl PipelineAll {
    pub fn execute(&self) -> Result<()> {
        Ok(())
    }
}
