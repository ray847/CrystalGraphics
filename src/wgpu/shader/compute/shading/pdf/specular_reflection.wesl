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
    let next_dir = specularDir(curr_dir, surface_pos);
    return TransportSample(next_dir, dist(curr_dir, next_dir, surface_pos,
        material));
}

const SPECULAR_PDF_EPSILON: f32 = 1e-6f;

fn dist(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> f32 {
    let expected_dir = specularDir(curr_dir, surface_pos);
    return select(
        0.0f,
        1.0f,
        dot(next_dir - expected_dir, next_dir - expected_dir)
            <= SPECULAR_PDF_EPSILON
    );
}

fn specularDir(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos
) -> math::Vec {
    return reflect(curr_dir, orientedNormal(curr_dir, surface_pos.shading_norm));
}
