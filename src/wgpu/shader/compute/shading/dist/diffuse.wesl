import package::compute::math;
import package::compute::structural;
import package::compute::shading::material::Material;
import super::super::util::orientedNormal;

fn dist(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> f32 {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    if dot(next_dir, macro_norm) <= 0.0f {
        return 0.0f;
    }
    return 1.0f / math::PI;
}
