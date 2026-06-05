import package::compute::math;
import package::compute::structural;
import package::compute::shading::material::Material;
import super::super::util::orientedNormal;
import super::sample::TransportSample;

const VNDF_EPSILON: f32 = 1e-6f;

fn sample(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> TransportSample {
    let micro_norm = orientedNormal(
        curr_dir,
        sampleNorm(curr_dir, surface_pos, material)
    );
    let next_dir = reflect(curr_dir, micro_norm);
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    if dot(next_dir, macro_norm) <= 0.0f {
        return TransportSample(next_dir, 0.0f);
    }
    return TransportSample(
        next_dir,
        reflectPdf(curr_dir, micro_norm, macro_norm, material)
    );
}

fn dist(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> f32 {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    if dot(next_dir, macro_norm) <= 0.0f {
        return 0.0f;
    }

    let micro_norm_unnormalized = next_dir - curr_dir;
    if dot(micro_norm_unnormalized, micro_norm_unnormalized) <= VNDF_EPSILON {
        return 0.0f;
    }

    let micro_norm = orientedNormal(
        curr_dir,
        normalize(micro_norm_unnormalized)
    );
    return reflectPdf(curr_dir, micro_norm, macro_norm, material);
}

fn sampleNorm(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
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
        max(local_curr_dir.z, VNDF_EPSILON)
    ));

    let tangent_length_sq = stretched_curr_dir.x * stretched_curr_dir.x
        + stretched_curr_dir.y * stretched_curr_dir.y;
    let tangent_x = select(
        vec3f(1.0f, 0.0f, 0.0f),
        vec3f(-stretched_curr_dir.y, stretched_curr_dir.x, 0.0f)
            / sqrt(max(tangent_length_sq, VNDF_EPSILON)),
        tangent_length_sq > VNDF_EPSILON
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
        visibility_lerp
    );

    let local_stretched_micro_norm = projected_x * tangent_x
        + projected_y * tangent_y
        + sqrt(max(
            0.0f,
            1.0f - projected_x * projected_x - projected_y * projected_y
        )) * stretched_curr_dir;
    let local_micro_norm = normalize(vec3f(
        alpha * local_stretched_micro_norm.x,
        alpha * local_stretched_micro_norm.y,
        max(local_stretched_micro_norm.z, VNDF_EPSILON)
    ));

    return normalize(tangent_space * local_micro_norm);
}

fn reflectPdf(
    curr_dir: math::Vec,
    micro_norm: math::Vec,
    macro_norm: math::Vec,
    material: Material
) -> f32 {
    let curr_macro_projection = max(dot(-curr_dir, macro_norm), VNDF_EPSILON);
    let curr_micro_projection = max(dot(-curr_dir, micro_norm), VNDF_EPSILON);
    let visible_normal_pdf = ggxNormDist(micro_norm, macro_norm, material)
        * ggxMaskUniDir(acos(curr_macro_projection), material)
        * curr_micro_projection
        / curr_macro_projection;
    return visible_normal_pdf / (4.0f * curr_micro_projection);
}

fn ggxMaskUniDir(angle: f32, material: Material) -> f32 {
    let alpha = material.microfacet_roughness;
    return 1.0f /
        (1.0f + (sqrt(1.0f + alpha * tan(angle) * tan(angle)) - 1.0f) / 2.0f);
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
