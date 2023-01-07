use autocxx::prelude::moveit;
use crate::{ffi, resource_usage};
use cxx::let_cxx_string;
use miette::IntoDiagnostic;
use std::io::Read;
use std::path::PathBuf;
use byteorder::ReadBytesExt;

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
    let parameters = output.join(format!("parameters_c{}.bin", chunk_index));
    let mut file = std::fs::File::open(parameters).into_diagnostic()?;
    let mut buf = Vec::new();
    let _n = file.read_to_end(&mut buf).into_diagnostic()?;
    let mut rdr = std::io::Cursor::new(&buf[..]);
    let num_samples = rdr.read_i32::<byteorder::NativeEndian>().into_diagnostic()?;
    let _num_alleles = rdr.read_i32::<byteorder::NativeEndian>().into_diagnostic()?;
    let num_windows = rdr.read_i32::<byteorder::NativeEndian>().into_diagnostic()? as usize - 1;
    let mut window_boundaries = Vec::new();
    window_boundaries.resize(num_windows + 1, 0);
    rdr.read_i32_into::<byteorder::NativeEndian>(&mut window_boundaries).into_diagnostic()?;
    eprintln!("---------------------------------------------------------");
    eprintln!("Painting sequences...");
    let basename = output.join(format!("chunk_{}", chunk_index));
    let hap = std::ffi::CString::new(basename.with_extension("hap").to_str().unwrap()).into_diagnostic()?;
    let pos = std::ffi::CString::new(basename.with_extension("bp").to_str().unwrap()).into_diagnostic()?;
    let dist = std::ffi::CString::new(basename.with_extension("dist").to_str().unwrap()).into_diagnostic()?;
    let rec = std::ffi::CString::new(basename.with_extension("r").to_str().unwrap()).into_diagnostic()?;
    let rpos = std::ffi::CString::new(basename.with_extension("rpos").to_str().unwrap()).into_diagnostic()?;
    let state = std::ffi::CString::new(basename.with_extension("state").to_str().unwrap()).into_diagnostic()?;
    moveit! {
        let mut data = unsafe { ffi::Data::new2(hap.as_ptr(), pos.as_ptr(), dist.as_ptr(), rec.as_ptr(), rpos.as_ptr(), state.as_ptr(), autocxx::c_int(30000), 1.25e-8) };
    }
    data.as_mut().SetPainting(painting[0], painting[1]);
    eprintln!("---------------------------------------------------------");
    eprintln!("Painting sequences...");
    let chunk_dir = output.join(format!("chunk_{}", chunk_index));
    let paint_dir = chunk_dir.join("paint");
    std::fs::create_dir_all(&paint_dir).into_diagnostic()?;
    let data_basename = paint_dir.join("relate");
    for hap in 0..num_samples as i32 {
        moveit! {
            let mut painter = ffi::FastPainting::new(num_samples as usize, 0.001);
        }
        let basename = std::ffi::CString::new(data_basename.to_str().unwrap()).into_diagnostic()?;
        unsafe {
            painter.as_mut()
            .PaintSteppingStones1(
                &data, 
                basename.as_ptr(), 
                num_windows,
                window_boundaries.as_ptr() as *const autocxx::c_int,
                autocxx::c_int(hap)
            );
        }
    }
    resource_usage();
    Ok(())
}
