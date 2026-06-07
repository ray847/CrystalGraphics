import super::{
    cie,
    radiance::Radiance,
    wavelength::Wavelength,
};

alias Luminance = vec3f;

/**
 * Convert the input radiance to the CIE XYZ color space.
 * 
 */
fn illuminate(wavelength: Wavelength, radiance: Radiance) -> Luminance {
    let average_integral = cie::xyzMatchSumIntegral(380.0f, 720.0f) / 3.0f;
    return Luminance(cie::xyzMatch(wavelength) * radiance / average_integral);
}
