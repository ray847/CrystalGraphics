import package::compute::math;
import package::compute::physics;
import package::compute::structural;
import package::compute::shading::material::Material;
import super::sample::EmissionSample;

const EMISSION_PDF_EPSILON: f32 = 1e-3f;

struct EmissivePrim {
    vertices: array<math::Pos, 3>,
    emission: vec3f,
    area: f32,
    weight: f32,
    _pad0: f32,
    _pad1: f32,
    _pad2: f32,
};

struct AliasTableEntry {
    alias_idx: u32,
    prob: f32,
};

@group(2) @binding(10) var<storage, read> emissive_prims: array<EmissivePrim>;
@group(2) @binding(11) var<storage, read> emissive_alias_table: array<AliasTableEntry>;

fn canSample() -> bool {
    return arrayLength(&emissive_alias_table) > 0u;
}

fn sample(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> EmissionSample {
    if !canSample() {
        return EmissionSample(
            math::Vec(),
            0.0f,
            physics::Radiance(),
            0.0f
        );
    }

    let sampled_prim: EmissivePrim = emissive_prims[sampleEmissivePrim()];
    let triangle_sample = vec2f(math::randFloat(), math::randFloat());
    let triangle_sample_sqrt = sqrt(triangle_sample.x);
    let barycentric = vec3f(
        1.0f - triangle_sample_sqrt,
        triangle_sample_sqrt * (1.0f - triangle_sample.y),
        triangle_sample_sqrt * triangle_sample.y
    );
    let sample_pos = barycentric.x * sampled_prim.vertices[0]
        + barycentric.y * sampled_prim.vertices[1]
        + barycentric.z * sampled_prim.vertices[2];
    let light_offset = sample_pos - surface_pos.pos;
    let distance_sq = max(
        dot(light_offset, light_offset),
        EMISSION_PDF_EPSILON
    );
    let next_dir = light_offset / sqrt(distance_sq);
    let light_norm = normalize(cross(
        sampled_prim.vertices[1] - sampled_prim.vertices[0],
        sampled_prim.vertices[2] - sampled_prim.vertices[0]
    ));
    let light_projection = abs(dot(light_norm, -next_dir));
    let area_pdf = max(
        sampled_prim.weight / sampled_prim.area,
        EMISSION_PDF_EPSILON
    );
    let solid_angle_pdf = area_pdf * distance_sq
        / max(light_projection, EMISSION_PDF_EPSILON);

    return EmissionSample(
        next_dir,
        solid_angle_pdf,
        sampled_prim.emission,
        sqrt(distance_sq)
    );
}

fn dist(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> f32 {
    if !canSample() {
        return 0.0f;
    }

    var pdf = 0.0f;
    let ray_dir = normalize(next_dir);
    for (var i = 0u; i < arrayLength(&emissive_prims); i++) {
        let prim = emissive_prims[i];
        let hit_dist = triangleHitDist(surface_pos.pos, ray_dir, prim);
        if hit_dist <= 0.0f {
            continue;
        }

        let light_norm = normalize(cross(
            prim.vertices[1] - prim.vertices[0],
            prim.vertices[2] - prim.vertices[0]
        ));
        let light_projection = abs(dot(light_norm, -ray_dir));
        let area_pdf = max(prim.weight / prim.area, EMISSION_PDF_EPSILON);
        pdf += area_pdf * hit_dist * hit_dist
            / max(light_projection, EMISSION_PDF_EPSILON);
    }
    return pdf;
}

fn triangleHitDist(
    ray_pos: math::Pos,
    ray_dir: math::Vec,
    prim: EmissivePrim
) -> f32 {
    let edge1 = prim.vertices[1] - prim.vertices[0];
    let edge2 = prim.vertices[2] - prim.vertices[0];
    let pvec = cross(ray_dir, edge2);
    let det = dot(edge1, pvec);
    if abs(det) <= EMISSION_PDF_EPSILON {
        return -1.0f;
    }

    let inv_det = 1.0f / det;
    let tvec = ray_pos - prim.vertices[0];
    let bary_u = dot(tvec, pvec) * inv_det;
    if bary_u < 0.0f || bary_u > 1.0f {
        return -1.0f;
    }

    let qvec = cross(tvec, edge1);
    let bary_v = dot(ray_dir, qvec) * inv_det;
    if bary_v < 0.0f || bary_u + bary_v > 1.0f {
        return -1.0f;
    }

    let hit_dist = dot(edge2, qvec) * inv_det;
    return select(-1.0f, hit_dist, hit_dist > EMISSION_PDF_EPSILON);
}

fn sampleEmissivePrim() -> u32 {
    let bin: u32 = u32(
        math::randFloat() * f32(arrayLength(&emissive_alias_table))
    );
    let randf: f32 = math::randFloat();
    return select(
        emissive_alias_table[bin].alias_idx,
        bin,
        randf < emissive_alias_table[bin].prob
    );
}
