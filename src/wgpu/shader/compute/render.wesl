import package::compute::math;
import package::compute::physics;
import package::compute::trace;

@id(2) override SAMPLE_COUNT: u32;

fn render(ray: math::Ray) -> physics::Luminance {
    var luminance = physics::Luminance();
    for (var i: u32; i < SAMPLE_COUNT; i++) {
        let wavelength_sample = physics::sampleWavelength();
        let radiance = trace::trace(ray, wavelength_sample.wavelength);
        luminance += \
            physics::illuminate(wavelength_sample.wavelength, radiance) /
            wavelength_sample.prob /
            f32(SAMPLE_COUNT);
    }
    return luminance;
}
