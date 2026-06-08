import package::compute::math;
import package::compute::structural;
import package::compute::shading::material::{Material, IOR};
import super::super::util::{fresnelReflectance, orientedNormal, relIor};

const VNDF_EPSILON: f32 = 1e-6f;

fn dist(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> f32 {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    if dot(next_dir, macro_norm) >= 0.0f {
        return 0.0f;
    }
    let next_projection = max(dot(-next_dir, macro_norm), 0.0f);

    let macro_relative_ior = relIor(macro_norm, curr_dir, material);
    let macro_relative_ior_value = relativeIor(macro_relative_ior);
    let micro_norm_unnormalized = macro_relative_ior_value * curr_dir
        - next_dir;
    if dot(micro_norm_unnormalized, micro_norm_unnormalized) <= VNDF_EPSILON {
        return 0.0f;
    }

    let raw_micro_norm = normalize(micro_norm_unnormalized);
    let micro_norm = orientedNormal(
        curr_dir,
        raw_micro_norm
    );
    let relative_ior = relIor(raw_micro_norm, curr_dir, material);
    let transmission_ior = transmissionIor(relative_ior);
    return (1.0f - fresnelReflectance(curr_dir, micro_norm, relative_ior))
        * transmitDist(
        curr_dir,
        micro_norm,
        macro_norm,
        next_dir,
        transmission_ior,
        material
    )
        * next_projection;
}

fn relativeIor(ior: IOR) -> f32 {
    return max(ior.re, VNDF_EPSILON);
}

fn transmissionIor(relative_ior: IOR) -> f32 {
    return 1.0f / relativeIor(relative_ior);
}

fn transmitDist(
    curr_dir: math::Vec,
    micro_norm: math::Vec,
    macro_norm: math::Vec,
    next_dir: math::Vec,
    transmission_ior: f32,
    material: Material
) -> f32 {
    let curr_macro_projection = max(dot(-curr_dir, macro_norm), VNDF_EPSILON);
    let next_macro_projection = max(dot(-next_dir, macro_norm), VNDF_EPSILON);
    let curr_micro_projection = max(dot(-curr_dir, micro_norm), VNDF_EPSILON);
    let next_micro_projection = dot(next_dir, micro_norm);
    let denominator = transmissionDenominator(
        curr_micro_projection,
        next_micro_projection,
        transmission_ior
    );
    return material.transmission
        * ggxNormDist(micro_norm, macro_norm, material)
        * ggxMaskBiDir(acos(curr_macro_projection), acos(next_macro_projection),
            material)
        * abs(curr_micro_projection * next_micro_projection)
        / max(
            curr_macro_projection
                * next_macro_projection
                * denominator
                * denominator,
            VNDF_EPSILON
        );
}

fn transmissionDenominator(
    curr_micro_projection: f32,
    next_micro_projection: f32,
    transmission_ior: f32
) -> f32 {
    return next_micro_projection + curr_micro_projection / transmission_ior;
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
