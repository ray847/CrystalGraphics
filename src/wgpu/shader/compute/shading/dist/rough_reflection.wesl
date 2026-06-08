import package::compute::math;
import package::compute::structural;
import package::compute::shading::material::Material;
import super::super::util::{fresnelReflectance, orientedNormal, relIor};

const VNDF_EPSILON: f32 = 1e-6f;

fn dist(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> f32 {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let next_projection = max(dot(next_dir, macro_norm), 0.0f);
    if next_projection <= 0.0f {
        return 0.0f;
    }

    let micro_norm_unnormalized = next_dir - curr_dir;
    if dot(micro_norm_unnormalized, micro_norm_unnormalized) <= VNDF_EPSILON {
        return 0.0f;
    }

    let raw_micro_norm = normalize(micro_norm_unnormalized);
    let micro_norm = orientedNormal(
        curr_dir,
        raw_micro_norm
    );
    let relative_ior = relIor(raw_micro_norm, curr_dir, material);
    return fresnelReflectance(curr_dir, micro_norm, relative_ior)
        * reflectDist(curr_dir, micro_norm, macro_norm, next_dir, material)
        * next_projection;
}

fn reflectDist(
    curr_dir: math::Vec,
    micro_norm: math::Vec,
    macro_norm: math::Vec,
    next_dir: math::Vec,
    material: Material
) -> f32 {
    let curr_macro_projection = max(dot(-curr_dir, macro_norm), VNDF_EPSILON);
    let next_macro_projection = max(dot(next_dir, macro_norm), VNDF_EPSILON);
    return ggxNormDist(micro_norm, macro_norm, material)
        * ggxMaskBiDir(acos(curr_macro_projection), acos(next_macro_projection),
            material)
        / (4.0f * curr_macro_projection * next_macro_projection);
}

fn ggxMaskBiDir(angle1: f32, angle2: f32, material: Material) -> f32 {
    let alpha = material.microfacet_roughness;
    return 1.0f /
    (1.0f +
        (sqrt(1.0f + alpha * tan(angle1) * tan(angle1)) - 1.0f) / 2.0f +
        (sqrt(1.0f + alpha * tan(angle2) * tan(angle2)) - 1.0f) / 2.0f);
}

fn ggxNormDist(
    micro_norm: math::Vec,
    macro_norm: math::Vec,
    material: Material
) -> f32 {
    let alpha = material.microfacet_roughness;
    let norm_cosine = clamp(dot(micro_norm, macro_norm), 0.0f, 1.0f);
    let norm_tangent_sq = max(0.0f, 1.0f - norm_cosine * norm_cosine)
        / max(norm_cosine * norm_cosine, VNDF_EPSILON);
    let denominator = math::PI
        * pow(norm_cosine, 4.0f)
        * pow(alpha + norm_tangent_sq, 2.0f);
    return alpha / max(denominator, VNDF_EPSILON);
}
