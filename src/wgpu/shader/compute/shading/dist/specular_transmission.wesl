import package::compute::math;
import package::compute::structural;
import package::compute::shading::material::{Material, IOR};
import super::super::util::{fresnelReflectance, orientedNormal, relIor};

const SPECULAR_DIST_EPSILON: f32 = 1e-6f;

fn dist(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> f32 {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let expected_dir = specularTransmissionDir(curr_dir, surface_pos, material);
    let distribution = select(
        0.0f,
        1.0f,
        dot(next_dir - expected_dir, next_dir - expected_dir)
            <= SPECULAR_DIST_EPSILON
    );
    let relative_ior = relIor(surface_pos.shading_norm, curr_dir, material);
    return material.transmission
        * (1.0f - fresnelReflectance(curr_dir, macro_norm, relative_ior))
        * distribution;
}

fn specularTransmissionDir(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> math::Vec {
    if material.volumn_transmission {
        let relative_ior: IOR = relIor(
            surface_pos.shading_norm,
            curr_dir,
            material
        );
        return refractDir(surface_pos.shading_norm, curr_dir, relative_ior);
    }

    return curr_dir;
}

fn refractDir(
    shading_norm: math::Vec,
    curr_dir: math::Vec,
    relative_ior: IOR
) -> math::Vec {
    let normal = orientedNormal(curr_dir, shading_norm);
    return refract(curr_dir, normal, relative_ior.re);
}
