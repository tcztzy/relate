pub mod pipelines;
use autocxx::prelude::include_cpp;

include_cpp! {
    #include "anc.hpp"
    #include "anc_builder.hpp"
    #include "data.hpp"
    #include "fast_painting.hpp"
    #include "ffi.hpp"
    safety!(unsafe_ffi)
    generate!("AncesTree")
    generate!("AncesTreeBuilder")
    generate!("CombineSections")
    generate!("Data")
    generate_pod!("FastPainting")
    generate!("FindEquivalentBranches")
    generate!("Finalize")
    generate!("GetBranchLengths")
    generate!("Mutations")
    generate!("construct_vector_double")
}

/// Print resource usage
fn resource_usage() {
    let usage = unsafe {
        #[allow(invalid_value)]
        let mut u = std::mem::MaybeUninit::<libc::rusage>::uninit().assume_init();
        libc::getrusage(libc::RUSAGE_SELF, &mut u);
        u
    };
    #[cfg(target_vendor = "apple")]
    let ru_maxrss = (usage.ru_maxrss as f64) / 1000000.;
    #[cfg(not(target_vendor = "apple"))]
    let ru_maxrss = (usage.ru_maxrss as f64) / 1000.;
    eprintln!(
        "CPU Time spent: {}.{:0>6}s; Max Memory usage: {}Mb.",
        usage.ru_utime.tv_sec, usage.ru_utime.tv_usec, ru_maxrss
    );
    eprintln!("---------------------------------------------------------");
}
