import package::compute::physics;

const BINS = array<physics::Wavelength, 10>(380.0, 417.7778, 455.5556, 493.3333, 531.1111, 568.8889, 606.6667, 644.4444, 682.2222, 720.0);

const WHITE = array<f32, 10>(1.0, 1.0, 0.9999, 0.9993, 0.9992, 0.9998, 1.0, 1.0, 1.0, 1.0);
const RED = array<f32, 10>(0.1012, 0.0515, 0.0, 0.0, 0.0, 0.0, 0.8325, 1.0149, 1.0149, 1.0149);
const GREEN = array<f32, 10>(0.0, 0.0, 0.0273, 0.7937, 1.0, 0.9418, 0.1719, 0.0, 0.0, 0.0025);
const BLUE = array<f32, 10>(1.0, 1.0, 0.8916, 0.3323, 0.0, 0.0, 0.0003, 0.0369, 0.0483, 0.0496);
const MAGENTA = array<f32, 10>(1.0, 1.0, 0.9685, 0.2229, 0.0, 0.0458, 0.8369, 1.0, 1.0, 0.9959);
const CYAN = array<f32, 10>(0.971, 0.9426, 1.0007, 1.0007, 1.0007, 1.0007, 0.1564, 0.0, 0.0, 0.0);
const YELLOW = array<f32, 10>(0.0001, 0.0, 0.1088, 0.6651, 1.0, 1.0, 0.9996, 0.9586, 0.9685, 0.984);


fn whiteReflectance(wavelength: physics::Wavelength)-> f32 {
    if wavelength <= 380.0 {
        return WHITE[0];
    }
    if wavelength >= 720.0 {
        return WHITE[9];
    }

    let bin: u32 = u32(
        f32(10) *
        (wavelength - 380.0) / 340.0
    );
    let mix = (wavelength - BINS[bin]) / (BINS[bin + 1] - BINS[bin]);
    return (1 - mix) * WHITE[bin] + mix * WHITE[bin + 1];
}

fn redReflectance(wavelength: physics::Wavelength)-> f32 {
    if wavelength <= 380.0 {
        return RED[0];
    }
    if wavelength >= 720.0 {
        return RED[9];
    }

    let bin: u32 = u32(
        f32(10) *
        (wavelength - 380.0) / 340.0
    );
    let mix = (wavelength - BINS[bin]) / (BINS[bin + 1] - BINS[bin]);
    return (1 - mix) * RED[bin] + mix * RED[bin + 1];
}

fn greenReflectance(wavelength: physics::Wavelength)-> f32 {
    if wavelength <= 380.0 {
        return GREEN[0];
    }
    if wavelength >= 720.0 {
        return GREEN[9];
    }

    let bin: u32 = u32(
        f32(10) *
        (wavelength - 380.0) / 340.0
    );
    let mix = (wavelength - BINS[bin]) / (BINS[bin + 1] - BINS[bin]);
    return (1 - mix) * GREEN[bin] + mix * GREEN[bin + 1];
}

fn blueReflectance(wavelength: physics::Wavelength)-> f32 {
    if wavelength <= 380.0 {
        return BLUE[0];
    }
    if wavelength >= 720.0 {
        return BLUE[9];
    }

    let bin: u32 = u32(
        f32(10) *
        (wavelength - 380.0) / 340.0
    );
    let mix = (wavelength - BINS[bin]) / (BINS[bin + 1] - BINS[bin]);
    return (1 - mix) * BLUE[bin] + mix * BLUE[bin + 1];
}

fn magentaReflectance(wavelength: physics::Wavelength)-> f32 {
    if wavelength <= 380.0 {
        return MAGENTA[0];
    }
    if wavelength >= 720.0 {
        return MAGENTA[9];
    }

    let bin: u32 = u32(
        f32(10) *
        (wavelength - 380.0) / 340.0
    );
    let mix = (wavelength - BINS[bin]) / (BINS[bin + 1] - BINS[bin]);
    return (1 - mix) * MAGENTA[bin] + mix * MAGENTA[bin + 1];
}

fn cyanReflectance(wavelength: physics::Wavelength)-> f32 {
    if wavelength <= 380.0 {
        return CYAN[0];
    }
    if wavelength >= 720.0 {
        return CYAN[9];
    }

    let bin: u32 = u32(
        f32(10) *
        (wavelength - 380.0) / 340.0
    );
    let mix = (wavelength - BINS[bin]) / (BINS[bin + 1] - BINS[bin]);
    return (1 - mix) * CYAN[bin] + mix * CYAN[bin + 1];
}

fn yellowReflectance(wavelength: physics::Wavelength)-> f32 {
    if wavelength <= 380.0 {
        return YELLOW[0];
    }
    if wavelength >= 720.0 {
        return YELLOW[9];
    }

    let bin: u32 = u32(
        f32(10) *
        (wavelength - 380.0) / 340.0
    );
    let mix = (wavelength - BINS[bin]) / (BINS[bin + 1] - BINS[bin]);
    return (1 - mix) * YELLOW[bin] + mix * YELLOW[bin + 1];
}
