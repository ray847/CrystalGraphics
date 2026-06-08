import package::compute::math;
import package::compute::shading::material::{Material, IOR};

fn orientedNormal(curr_dir: math::Vec, normal: math::Vec) -> math::Vec {
    return select(normal, -normal, dot(curr_dir, normal) > 0.0f);
}

fn relIor(
    shading_norm: math::Vec,
    curr_dir: math::Vec,
    material: Material
) -> IOR {
    if dot(shading_norm, curr_dir) < 0.0f {
        return invIor(material.ior);
    }
    return material.ior;
}

fn invIor(ior: IOR) -> IOR {
    let norm_sq = ior.re * ior.re + ior.im * ior.im;
    return IOR(ior.re / norm_sq, -ior.im / norm_sq);
}

const FRESNEL_EPSILON: f32 = 1e-6f;

fn fresnelReflectance(
    curr_dir: vec3f,
    normal: vec3f,
    relative_ior: IOR
) -> f32 {
    let incident_cosine = clamp(dot(-curr_dir, normal), 0.0f, 1.0f);

    if abs(relative_ior.im) > FRESNEL_EPSILON {
        return complexFresnelReflectance(incident_cosine, relative_ior);
    }

    let ior_ratio = relative_ior.re;
    let transmitted_sine_squared = ior_ratio * ior_ratio * max(
        0.0f,
        1.0f - incident_cosine * incident_cosine
    );

    if transmitted_sine_squared >= 1.0f {
        return 1.0f;
    }

    let transmitted_cosine = sqrt(max(0.0f, 1.0f - transmitted_sine_squared));
    let perpendicular_reflectance =
        (ior_ratio * incident_cosine - transmitted_cosine)
        / (ior_ratio * incident_cosine + transmitted_cosine);
    let parallel_reflectance =
        (incident_cosine - ior_ratio * transmitted_cosine)
        / (incident_cosine + ior_ratio * transmitted_cosine);
    return clamp(
        0.5f * (perpendicular_reflectance * perpendicular_reflectance
            + parallel_reflectance * parallel_reflectance),
        0.0f,
        1.0f
    );
}

fn complexFresnelReflectance(cosine: f32, material_ior: IOR) -> f32 {
    let real_ior = material_ior.re;
    let imaginary_ior = material_ior.im;
    let real_sq = real_ior * real_ior;
    let imaginary_sq = imaginary_ior * imaginary_ior;
    let cosine_sq = cosine * cosine;
    let ior_norm_sq = real_sq + imaginary_sq;
    let two_real_cosine = 2.0f * real_ior * cosine;

    let perpendicular_reflectance = (ior_norm_sq - two_real_cosine + cosine_sq)
        / max(ior_norm_sq + two_real_cosine + cosine_sq, FRESNEL_EPSILON);
    let parallel_reflectance = (ior_norm_sq * cosine_sq - two_real_cosine + 1.0f)
        / max(ior_norm_sq * cosine_sq + two_real_cosine + 1.0f,
            FRESNEL_EPSILON);
    return clamp(
        0.5f * (perpendicular_reflectance + parallel_reflectance),
        0.0f,
        1.0f
    );
}
