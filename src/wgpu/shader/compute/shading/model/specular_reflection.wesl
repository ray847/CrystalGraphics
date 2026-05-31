import package::compute::math;
import package::compute::structural;
import super::super::material::Material;
import super::super::model_sample::ModelSample;
import super::util::{fresnelReflectance, orientedNormal};

/* Sample Function */
fn sample(
    surface_pos: structural::SurfacePos,
    curr_dir: math::Vec,
    material: Material
) -> ModelSample {
    return ModelSample(
        specularDir(surface_pos, curr_dir),
        specularDist(surface_pos, curr_dir, material)
    );
}

fn specularDir(
    surface_pos: structural::SurfacePos,
    curr_dir: math::Vec,
) -> math::Vec {
    let next_dir = reflect(curr_dir, orientedNormal(curr_dir, surface_pos.shading_norm));
    return next_dir;
}

fn specularDist(
    surface_pos: structural::SurfacePos,
    curr_dir: math::Vec,
    material: Material
) -> vec3f {
    let normal = orientedNormal(curr_dir, surface_pos.shading_norm);
    let fresnel_reflectance = fresnelReflectance(
        curr_dir,
        normal,
        material.ior
    );
    return fresnel_reflectance;
}
