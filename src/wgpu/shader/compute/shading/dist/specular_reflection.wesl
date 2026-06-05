import package::compute::math;
import package::compute::structural;
import package::compute::shading::material::Material;
import super::super::util::orientedNormal;

const SPECULAR_DIST_EPSILON: f32 = 1e-6f;

fn dist(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> f32 {
    let expected_dir = reflect(
        curr_dir,
        orientedNormal(curr_dir, surface_pos.shading_norm)
    );
    return select(
        0.0f,
        1.0f,
        dot(next_dir - expected_dir, next_dir - expected_dir)
            <= SPECULAR_DIST_EPSILON
    );
}
