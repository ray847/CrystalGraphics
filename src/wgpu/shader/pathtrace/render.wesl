import package::pathtrace::{
    luminance::{Luminance, illuminate},
    math::Ray,
    trace::trace,
};

fn render(ray: Ray) -> Luminance {
    return illuminate(trace(ray));
}
