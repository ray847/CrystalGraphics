import package::compute::math;
import package::compute::structural;
import super::super::material::{Material, IOR, IORChannel};
import super::super::model_sample::ModelSample;

const FRESNEL_EPSILON: f32 = 1e-6f;

/* Fresnel Equations */
fn fresnelReflectance(
    curr_dir: vec3f,
    normal: vec3f,
    material_ior: IOR
) -> vec3f {
    return vec3f(
        fresnelReflectanceChannel(curr_dir, normal, material_ior.r),
        fresnelReflectanceChannel(curr_dir, normal, material_ior.g),
        fresnelReflectanceChannel(curr_dir, normal, material_ior.b)
    );
}

fn fresnelReflectanceChannel(
    curr_dir: vec3f,
    normal: vec3f,
    material_ior: IORChannel
) -> f32 {
    let outside = dot(curr_dir, normal) >= 0.0f;
    let facing_normal = select(-normal, normal, outside);
    let incident_cosine = clamp(dot(curr_dir, facing_normal), 0.0f, 1.0f);

    if material_ior.im > FRESNEL_EPSILON {
        return complexFresnelReflectance(incident_cosine, material_ior);
    }

    let incident_ior = select(material_ior.re, 1.0f, outside);
    let transmitted_ior = select(1.0f, material_ior.re, outside);
    let ior_ratio = incident_ior / transmitted_ior;
    let transmitted_sine_squared = ior_ratio * ior_ratio * max(
        0.0f,
        1.0f - incident_cosine * incident_cosine
    );

    if transmitted_sine_squared >= 1.0f {
        return 1.0f;
    }

    let transmitted_cosine = sqrt(max(0.0f, 1.0f - transmitted_sine_squared));
    let perpendicular_reflectance = (incident_ior * incident_cosine - transmitted_ior * transmitted_cosine)
        / (incident_ior * incident_cosine + transmitted_ior * transmitted_cosine);
    let parallel_reflectance = (transmitted_ior * incident_cosine - incident_ior * transmitted_cosine)
        / (transmitted_ior * incident_cosine + incident_ior * transmitted_cosine);
    return clamp(
        0.5f * (perpendicular_reflectance * perpendicular_reflectance
            + parallel_reflectance * parallel_reflectance),
        0.0f,
        1.0f,
    );
}

fn complexFresnelReflectance(cosine: f32, material_ior: IORChannel) -> f32 {
    let real_ior = material_ior.re;
    let imaginary_ior = material_ior.im;
    let real_sq = real_ior * real_ior;
    let imaginary_sq = imaginary_ior * imaginary_ior;
    let cosine_sq = cosine * cosine;
    let ior_norm_sq = real_sq + imaginary_sq;
    let two_real_cosine = 2.0f * real_ior * cosine;

    let perpendicular_reflectance =
        (ior_norm_sq - two_real_cosine + cosine_sq)
        / max(ior_norm_sq + two_real_cosine + cosine_sq, FRESNEL_EPSILON);
    let parallel_reflectance =
        (ior_norm_sq * cosine_sq - two_real_cosine + 1.0f)
        / max(ior_norm_sq * cosine_sq + two_real_cosine + 1.0f, FRESNEL_EPSILON);

    return clamp(
        0.5f * (perpendicular_reflectance + parallel_reflectance),
        0.0f,
        1.0f
    );
}
