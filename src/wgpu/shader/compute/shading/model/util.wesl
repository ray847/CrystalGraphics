import package::compute::math;
import package::compute::structural;
import super::super::material::Material;
import super::super::model_sample::ModelSample;

/* Fresnel Equations */
fn fresnelReflectance(
    curr_dir: vec3f,
    normal: vec3f,
    material_ior: f32
) -> f32 {
    let outside = dot(curr_dir, normal) >= 0.0f;
    let incident_ior = select(material_ior, 1.0f, outside);
    let transmitted_ior = select(1.0f, material_ior, outside);
    let facing_normal = select(-normal, normal, outside);
    let incident_cosine = clamp(dot(curr_dir, facing_normal), 0.0f, 1.0f);
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
