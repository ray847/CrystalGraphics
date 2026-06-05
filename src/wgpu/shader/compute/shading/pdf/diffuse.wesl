import package::compute::math;
import package::compute::structural;
import package::compute::shading::material::Material;
import super::super::util::orientedNormal;
import super::sample::TransportSample;

fn sample(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> TransportSample {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let next_dir = math::makeTangentSpace(macro_norm) * cosineWeightedSample();
    return TransportSample(next_dir, dist(curr_dir, next_dir,
        surface_pos, material));
}

fn dist(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> f32 {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    return max(dot(next_dir, macro_norm), 0.0f) / math::PI;
}

fn cosineWeightedSample() -> math::Vec {
    let r = sqrt(math::randFloat());
    let angle = 2.0f * math::PI * math::randFloat();
    return math::Vec(
        r * cos(angle),
        r * sin(angle),
        sqrt(max(0.0f, 1.0f - r * r))
    );
}
