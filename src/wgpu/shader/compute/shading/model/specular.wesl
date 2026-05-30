import package::compute::math;
import package::compute::structural;
import super::super::material::Material;
import super::super::model_sample::ModelSample;
import super::util::{fresnelReflectance};

/* Sample Function */
fn specularSample(
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
    let next_dir = reflect(-curr_dir, orientedNormal(curr_dir, surface_pos.shading_norm));
    return next_dir;
}
fn orientedNormal(curr_dir: vec3f, normal: vec3f) -> vec3f {
    return select(-normal, normal, dot(curr_dir, normal) >= 0.0f);
}

fn specularDist(
    surface_pos: structural::SurfacePos,
    curr_dir: math::Vec,
    material: Material
) -> vec3f {
    let fresnel_reflectance = fresnelReflectance(
        curr_dir,
        surface_pos.shading_norm,
        material.ior
    );
    return fresnel_reflectance;
}
