import package::compute::structural;
import super::dielectric_specular::{dielectricSpecularDir, orientedNormal};
import super::rough::{roughDir, ggxSurfaceRoughness, ggxMasking, ggxMaskingShadowing};
import super::super::material::Material;

const METALLIC_EPSILON: f32 = 1e-4f;

fn metallicFresnel(
    material: Material,
    surface_pos: structural::SurfacePos,
    curr_dir: vec3f,
    micro_normal: vec3f
) -> vec3f {
    let base_reflectance = clamp(
        material.base_color.rgb,
        vec3f(0.0f),
        vec3f(1.0f),
    );
    let facing_micro_normal = orientedNormal(curr_dir, micro_normal);
    let reflectance_cosine = clamp(dot(curr_dir, facing_micro_normal), 0.0f, 1.0f);
    let grazing_weight = pow(1.0f - reflectance_cosine, 5.0f);
    return base_reflectance
        + (vec3f(1.0f) - base_reflectance) * grazing_weight;
}

/* Sample Function */
fn metallicSpecularDir(
    surface_pos: structural::SurfacePos,
    curr_dir: vec3f,
    material: Material
) -> vec3f {
    let next_dir = dielectricSpecularDir(surface_pos, curr_dir);
    return next_dir;
}

fn metallicRoughDir(
    surface_pos: structural::SurfacePos,
    curr_dir: vec3f,
    material: Material
) -> vec3f {
    let next_dir = roughDir(surface_pos, curr_dir, material);
    return next_dir;
}

/* Distribution Function */
fn metallicSpecularDist(
    surface_pos: structural::SurfacePos,
    curr_dir: vec3f,
    material: Material
) -> vec3f {
    return metallicFresnel(material, surface_pos, curr_dir, surface_pos.shading_norm);
}

fn metallicRoughDist(
    surface_pos: structural::SurfacePos,
    next_dir: vec3f,
    curr_dir: vec3f,
    material: Material
) -> vec3f {
    let normal = orientedNormal(curr_dir, surface_pos.shading_norm);
    let next_normal_projection = max(dot(next_dir, normal), 0.0f);
    let curr_normal_projection = max(dot(curr_dir, normal), 0.0f);

    if next_normal_projection <= 0.0f || curr_normal_projection <= 0.0f {
        return vec3f(0.0f);
    }

    let micro_normal = normalize(next_dir + curr_dir);
    let curr_micro_projection = max(dot(curr_dir, micro_normal), 0.0f);

    if curr_micro_projection <= 0.0f {
        return vec3f(0.0f);
    }

    let surface_roughness = ggxSurfaceRoughness(material, surface_pos);
    let visible_masking = ggxMasking(curr_normal_projection, surface_roughness);
    let masking_shadowing = ggxMaskingShadowing(
        next_normal_projection,
        curr_normal_projection,
        surface_roughness,
    );
    return metallicFresnel(material, surface_pos, curr_dir, micro_normal)
        * masking_shadowing
        / max(visible_masking, METALLIC_EPSILON);
}
