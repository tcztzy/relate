use crate::{ffi, resource_usage};
use autocxx::{c_int, prelude::moveit};
use byteorder::{NativeEndian, ReadBytesExt};
use cxx::let_cxx_string;
use miette::IntoDiagnostic;
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

pub fn make_chunks(
    haps: PathBuf,
    sample: PathBuf,
    map: PathBuf,
    dist: PathBuf,
    output: PathBuf,
    transversion: bool,
    memory: f32,
) -> miette::Result<()> {
    eprintln!("---------------------------------------------------------");
    eprintln!("Parsing data..");
    if output.exists() {
        panic!(
            "Directory {} already exists. Relate will use this directory to store temporary files.",
            output.display()
        );
    }
    std::fs::create_dir(&output).expect("Fail to create output directory");
    let_cxx_string!(filename_haps = haps.to_str().unwrap());
    let_cxx_string!(filename_sample = sample.to_str().unwrap());
    let_cxx_string!(filename_map = map.to_str().unwrap());
    let_cxx_string!(filename_dist = dist.to_str().unwrap());
    let_cxx_string!(filename_out = output.to_str().unwrap());
    moveit! {
        let mut data = ffi::Data::new();
    }
    data.as_mut().MakeChunks(
        &filename_haps,
        &filename_sample,
        &filename_map,
        &filename_dist,
        &filename_out,
        !transversion,
        memory,
    );
    resource_usage();
    Ok(())
}

pub fn paint(output: &PathBuf, chunk_index: usize, painting: Vec<f64>) -> miette::Result<()> {
    let parameters = read_parameters_bin(&output.join(format!("parameters_c{}.bin", chunk_index)))?;
    let num_samples = parameters.num_samples;
    let num_windows = parameters.num_windows;
    let window_boundaries: Vec<c_int> = parameters
        .window_boundaries
        .into_iter()
        .map(|wb| c_int(wb as i32))
        .collect();
    eprintln!("---------------------------------------------------------");
    eprintln!("Painting sequences...");
    let basename = output.join(format!("chunk_{}", chunk_index));
    let hap = std::ffi::CString::new(basename.with_extension("hap").to_str().unwrap())
        .into_diagnostic()?;
    let pos = std::ffi::CString::new(basename.with_extension("bp").to_str().unwrap())
        .into_diagnostic()?;
    let dist = std::ffi::CString::new(basename.with_extension("dist").to_str().unwrap())
        .into_diagnostic()?;
    let rec =
        std::ffi::CString::new(basename.with_extension("r").to_str().unwrap()).into_diagnostic()?;
    let rpos = std::ffi::CString::new(basename.with_extension("rpos").to_str().unwrap())
        .into_diagnostic()?;
    let state = std::ffi::CString::new(basename.with_extension("state").to_str().unwrap())
        .into_diagnostic()?;
    moveit! {
        let mut data = unsafe { ffi::Data::new2(hap.as_ptr(), pos.as_ptr(), dist.as_ptr(), rec.as_ptr(), rpos.as_ptr(), state.as_ptr(), c_int(30000), 1.25e-8) };
    }
    data.as_mut().SetPainting(painting[0], painting[1]);
    eprintln!("---------------------------------------------------------");
    eprintln!("Painting sequences...");
    let chunk_dir = output.join(format!("chunk_{}", chunk_index));
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
        let basename = std::ffi::CString::new(data_basename.to_str().unwrap()).into_diagnostic()?;
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

pub fn build_topology(
    output: &PathBuf,
    chunk_index: usize,
    section_slice: Option<(usize, usize)>,
    efficient_population_size: i32,
    painting: Option<Vec<f64>>,
    seed: Option<u64>,
    anc_allele_unknown: bool,
    sample_ages_path: Option<&PathBuf>,
    fb: i32,
) -> miette::Result<()> {
    let parameters_path = output.join(format!("parameters_c{}.bin", chunk_index));
    let parameters = read_parameters_bin(&parameters_path)?;
    let (first_section, last_section) = section_slice.unwrap_or((0, parameters.num_windows - 1));
    let basename = output.join(format!("chunk_{}", chunk_index));
    let hap = std::ffi::CString::new(basename.with_extension("hap").to_str().unwrap())
        .into_diagnostic()?;
    let pos = std::ffi::CString::new(basename.with_extension("bp").to_str().unwrap())
        .into_diagnostic()?;
    let dist = std::ffi::CString::new(basename.with_extension("dist").to_str().unwrap())
        .into_diagnostic()?;
    let rec =
        std::ffi::CString::new(basename.with_extension("r").to_str().unwrap()).into_diagnostic()?;
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
    data.as_mut().SetEfficientPopulationSize(c_int(efficient_population_size * 50));
    if let Some(painting) = painting {
        data.as_mut().SetPainting(painting[0], painting[1]);
    }
    let mut rng: StdRng = if let Some(seed) = seed {
        SeedableRng::seed_from_u64(seed)
    } else {
        StdRng::from_entropy()
    };
    let ancestral_state = !anc_allele_unknown;
    let sample_ages: Vec<f64> = if let Some(path) = sample_ages_path {
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
            c_int(fb),
        );
        let section_basename = basename.join(format!(
            "{}_{}",
            output.file_name().unwrap().to_str().unwrap(),
            section
        ));
        let_cxx_string!(anc_filename = section_basename.with_extension("anc").to_str().unwrap());
        let_cxx_string!(mut_filename = section_basename.with_extension("mut").to_str().unwrap());
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

pub fn find_equivalent_branches(output: &PathBuf, chunk_index: usize) -> miette::Result<()> {
    ffi::FindEquivalentBranches(output.to_str().unwrap(), c_int(chunk_index as i32));
    Ok(())
}
