import package::compute::math;
import package::compute::structural;
import super::super::material::Material;
import super::super::model_sample::ModelSample;

/* Sample Function */
fn sample(
    surface_pos: structural::SurfacePos,
    curr_dir: math::Vec,
    material: Material
) -> ModelSample {
    return ModelSample(
        diffuseDir(surface_pos),
        diffuseDist(surface_pos, curr_dir, material)
    );
}

fn diffuseDir(
    surface_pos: structural::SurfacePos,
) -> vec3f {
    let next_dir = math::makeTangentSpace(surface_pos.shading_norm) * cosineWeightedSample();
    return next_dir;
}
fn cosineWeightedSample() -> math::Vec {
    let r = sqrt(math::randFloat());
    let angle = 2 * math::PI * math::randFloat();
    let next_dir = math::Vec(
        r * cos(angle),
        r * sin(angle),
        sqrt(max(0.0f, 1 - r * r))
    );
    return next_dir;
}

fn diffuseDist(
    surface_pos: structural::SurfacePos,
    curr_dir: math::Vec,
    material: Material
) -> vec3f {
    return lambertianDist(material, surface_pos);
}
fn lambertianDist(material: Material, surface_pos: structural::SurfacePos) -> vec3f {
    return material.diffuse_reflectance.rgb;
}
