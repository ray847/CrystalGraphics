import package::pathtrace::{
    luminance::{Luminance, illuminate},
    radiance::Radiance,
    math::Ray,
    trace::trace,
};

fn render(ray: Ray) -> Luminance {
    const SAMPLES: i32 = 4;
    var radiance = Radiance();
    for (var i: i32; i < SAMPLES; i++) {
        radiance += trace(ray);
    }
    radiance /= f32(SAMPLES);
    return illuminate(radiance);
}
