import package::compute::math;
import super::cie;

alias Wavelength = f32;

/**
 * Sample wavelengths from the normalized sum of the CIE XYZ color matching
 * functions over the visible range [380, 720].
 */
const VISIBLE_MIN: Wavelength = 380.0f;
const VISIBLE_MAX: Wavelength = 720.0f;
const INVERSE_CDF_STEPS: u32 = 16u;
const PDF_EPSILON: f32 = 1e-6f;

struct Sample {
    wavelength: Wavelength,
    prob: f32,
};

fn sampleWavelength() -> Sample {
    return sampleWavelengthCdf(math::randFloat());
}

fn sampleWavelengthCdf(cdf_sample: f32) -> Sample {
    let total_match = cie::xyzMatchSumIntegral(VISIBLE_MIN, VISIBLE_MAX);
    let target_cdf = clamp(cdf_sample, 0.0f, 0.999999f);
    var low = VISIBLE_MIN;
    var high = VISIBLE_MAX;

    for (var i = 0u; i < INVERSE_CDF_STEPS; i++) {
        let mid = 0.5f * (low + high);
        let cdf = cie::xyzMatchSumIntegral(VISIBLE_MIN, mid) / total_match;
        if cdf < target_cdf {
            low = mid;
        } else {
            high = mid;
        }
    }

    let wavelength = 0.5f * (low + high);
    return Sample(
        wavelength,
        max(cie::xyzMatchSum(wavelength) / total_match, PDF_EPSILON)
    );
}
