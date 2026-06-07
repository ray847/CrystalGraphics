import pathlib
from colour.recovery.datasets.smits1999 import DATA_SMITS1999

n_bins = len(DATA_SMITS1999["white"])

with open(pathlib.Path(__file__).parent / "smits.wesl", "w") as f:
    f.write(
        """import package::compute::physics;

const BINS = array<physics::Wavelength, {n_bins}>({bins});

const WHITE = array<f32, {n_bins}>({white});
const RED = array<f32, {n_bins}>({red});
const GREEN = array<f32, {n_bins}>({green});
const BLUE = array<f32, {n_bins}>({blue});
const MAGENTA = array<f32, {n_bins}>({magenta});
const CYAN = array<f32, {n_bins}>({cyan});
const YELLOW = array<f32, {n_bins}>({yellow});

""".format(
            n_bins=n_bins,
            bins=", ".join([str(i) for i in DATA_SMITS1999["white"].keys()]),
            white=", ".join([str(i) for i in DATA_SMITS1999["white"].values()]),
            red=", ".join([str(i) for i in DATA_SMITS1999["red"].values()]),
            green=", ".join([str(i) for i in DATA_SMITS1999["green"].values()]),
            blue=", ".join([str(i) for i in DATA_SMITS1999["blue"].values()]),
            magenta=", ".join(
                [str(i) for i in DATA_SMITS1999["magenta"].values()]
            ),
            cyan=", ".join([str(i) for i in DATA_SMITS1999["cyan"].values()]),
            yellow=", ".join(
                [str(i) for i in DATA_SMITS1999["yellow"].values()]
            ),
        )
    )

    # Functions
    wavelength_min = min(DATA_SMITS1999["white"].keys())
    wavelength_max = max(DATA_SMITS1999["white"].keys())
    for color in ["white", "red", "green", "blue", "magenta", "cyan", "yellow"]:
        f.write(
            f"""
fn {color}Reflectance(wavelength: physics::Wavelength)-> f32 {{
    if wavelength <= {wavelength_min} {{
        return {color.upper()}[0];
    }}
    if wavelength >= {wavelength_max} {{
        return {color.upper()}[{n_bins - 1}];
    }}

    let bin: u32 = u32(
        f32({n_bins}) *
        (wavelength - {wavelength_min}) / {wavelength_max - wavelength_min}
    );
    let mix = (wavelength - BINS[bin]) / (BINS[bin + 1] - BINS[bin]);
    return (1 - mix) * {color.upper()}[bin] + mix * {color.upper()}[bin + 1];
}}
"""
        )
