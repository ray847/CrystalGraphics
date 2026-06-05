import package::compute::math;
import package::compute::structural;

const RAY_OFFSET: f32 = 1e-3;

fn spawnSurfaceRay(
    surface_pos: structural::SurfacePos,
    next_dir: math::Vec
) -> math::Ray {
    let offset_normal = select(
        -surface_pos.geometry_norm,
        surface_pos.geometry_norm,
        dot(next_dir, surface_pos.geometry_norm) > 0.0f,
    );
    return math::Ray(surface_pos.pos + offset_normal * RAY_OFFSET, next_dir);
}