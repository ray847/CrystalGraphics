import package::compute::math;
import package::compute::physics;
import package::compute::trace;

fn render(ray: math::Ray) -> physics::Luminance {
    const SAMPLES: u32 = 1;
    var radiance = physics::Radiance();
    for (var i: u32; i < SAMPLES; i++) {
        radiance += trace::trace(ray);
    }
    radiance /= f32(SAMPLES);
    return physics::illuminate(radiance);
}
