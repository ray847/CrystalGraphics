import package::compute::math;
import package::compute::structural;
import super::super::material::Material;
import super::super::model_sample::ModelSample;
import super::util::{fresnelReflectance, orientedNormal};

const VNDF_EPSILON: f32 = 1e-6f;

/* Sample Function */
fn sample(
    surface_pos: structural::SurfacePos,
    curr_dir: math::Vec,
    material: Material
) -> ModelSample {
    let micro_norm = orientedNormal(
        curr_dir,
        sampleNorm(surface_pos, curr_dir, material)
    );
    let next_dir = reflect(curr_dir, micro_norm);
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    if dot(next_dir, macro_norm) <= 0.0f {
        return ModelSample(next_dir, vec3f(0.0f));
    }
    let dist = reflectDist(curr_dir, micro_norm, macro_norm,
        next_dir, material)
        * reflectPdfWeight(curr_dir, micro_norm, macro_norm, next_dir, material);
    return ModelSample(
        next_dir,
        dist
    );
}

/**
 * Sample a micro normal direction with the visible normal distribution function
 */
fn sampleNorm(
    surface_pos: structural::SurfacePos,
    curr_dir: math::Vec,
    material: Material
) -> math::Vec {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let tangent_space = math::makeTangentSpace(macro_norm);
    let view_dir = -curr_dir;
    let local_curr_dir = normalize(transpose(tangent_space) * view_dir);
    let alpha = material.microfacet_roughness;

    let stretched_curr_dir = normalize(vec3f(
        alpha * local_curr_dir.x,
        alpha * local_curr_dir.y,
        max(local_curr_dir.z, VNDF_EPSILON),
    ));

    let tangent_length_sq = stretched_curr_dir.x * stretched_curr_dir.x
        + stretched_curr_dir.y * stretched_curr_dir.y;
    let tangent_x = select(
        vec3f(1.0f, 0.0f, 0.0f),
        vec3f(-stretched_curr_dir.y, stretched_curr_dir.x, 0.0f)
            / sqrt(max(tangent_length_sq, VNDF_EPSILON)),
        tangent_length_sq > VNDF_EPSILON,
    );
    let tangent_y = cross(stretched_curr_dir, tangent_x);

    let radius = sqrt(math::randFloat());
    let angle = 2.0f * math::PI * math::randFloat();
    let projected_x = radius * cos(angle);
    var projected_y = radius * sin(angle);
    let visibility_lerp = 0.5f * (1.0f + stretched_curr_dir.z);
    projected_y = mix(
        sqrt(max(0.0f, 1.0f - projected_x * projected_x)),
        projected_y,
        visibility_lerp,
    );

    let local_stretched_micro_norm = projected_x * tangent_x
        + projected_y * tangent_y
        + sqrt(max(
        0.0f,
        1.0f - projected_x * projected_x - projected_y * projected_y,
    )) * stretched_curr_dir;
    let local_micro_norm = normalize(vec3f(
        alpha * local_stretched_micro_norm.x,
        alpha * local_stretched_micro_norm.y,
        max(local_stretched_micro_norm.z, VNDF_EPSILON),
    ));

    return normalize(tangent_space * local_micro_norm);
}

fn reflectDist(
    curr_dir: math::Vec,
    micro_norm: math::Vec,
    macro_norm: math::Vec,
    next_dir: math::Vec,
    material: Material
) -> vec3f {
    let curr_macro_projection = max(dot(-curr_dir, macro_norm), VNDF_EPSILON);
    let next_macro_projection = max(dot(next_dir, macro_norm), VNDF_EPSILON);

    return ggxNormDist(micro_norm, macro_norm, material)
        * ggxMaskBiDir(acos(curr_macro_projection), acos(next_macro_projection),
        material)
        * fresnelReflectance(curr_dir, micro_norm, material.ior)
        / (4.0f * curr_macro_projection * next_macro_projection);
}

fn reflectPdfWeight(
    curr_dir: math::Vec,
    micro_norm: math::Vec,
    macro_norm: math::Vec,
    next_dir: math::Vec,
    material: Material
) -> f32 {
    let curr_macro_projection = max(dot(-curr_dir, macro_norm), VNDF_EPSILON);
    let next_macro_projection = max(dot(next_dir, macro_norm), 0.0f);
    let curr_micro_projection = max(dot(-curr_dir, micro_norm), VNDF_EPSILON);
    let visible_normal_pdf = ggxNormDist(micro_norm, macro_norm, material)
        * ggxMaskUniDir(acos(curr_macro_projection), material)
        * curr_micro_projection
        / curr_macro_projection;
    let next_dir_pdf = visible_normal_pdf / (4.0f * curr_micro_projection);

    return next_macro_projection / max(next_dir_pdf, VNDF_EPSILON);
}

fn ggxMaskUniDir(angle: f32, material: Material) -> f32 {
    let alpha = material.microfacet_roughness;
    return 1 /
    (1 + (sqrt(1 + alpha * tan(angle) * tan(angle)) - 1) / 2);
}

fn ggxMaskBiDir(angle1: f32, angle2: f32, material: Material) -> f32 {
    let alpha = material.microfacet_roughness;
    return 1 /
    (1 +
        (sqrt(1 + alpha * tan(angle1) * tan(angle1)) - 1) / 2 +
        (sqrt(1 + alpha * tan(angle2) * tan(angle2)) - 1) / 2);
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
