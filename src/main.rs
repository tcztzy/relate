use clap::{Parser, Subcommand};
use miette::Result;
use relate::pipelines::{
    BuildTopology, CombineSections, Finalize, FindEquivalentBranches, InferBranchLengths,
    MakeChunks, Paint, PipelineAll,
};

#[derive(Parser, Debug)]
struct Args {
    #[command(subcommand)]
    mode: Mode,
    #[clap(flatten)]
    verbose: clap_verbosity_flag::Verbosity,
}

#[derive(Subcommand, Debug)]
enum Mode {
    All(PipelineAll),
    MakeChunks(MakeChunks),
    Paint(Paint),
    BuildTopology(BuildTopology),
    FindEquivalentBranches(FindEquivalentBranches),
    InferBranchLengths(InferBranchLengths),
    CombineSections(CombineSections),
    Finalize(Finalize),
}

impl Mode {
    fn execute(&self) -> Result<()> {
        match &self {
            Mode::All(options) => options.execute(),
            Mode::MakeChunks(options) => options.execute(),
            Mode::Paint(options) => options.execute(),
            Mode::BuildTopology(options) => options.execute(),
            Mode::FindEquivalentBranches(options) => options.execute(),
            Mode::InferBranchLengths(options) => options.execute(),
            Mode::CombineSections(options) => options.execute(),
            Mode::Finalize(options) => options.execute(),
        }
    }
}

fn main() -> miette::Result<()> {
    let args = Args::parse();
    args.mode.execute()
}
