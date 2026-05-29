import package::compute::math;
import package::compute::physics;
import package::compute::structural;
import package::compute::lod;
import super::material::{Material, material};
import super::model::{
    diffuse::diffuseSample,
    dielectric_specular::dielectricSpecularSample,
    dielectric_rough::dielectricRoughSample,
};

const MAX_SAMPLE_COUNT: u32 = 6u;
const MAX_DIFFUSE_COUNT: u32 = 4u;
const MAX_DIELECTRIC_ROUGH_COUNT: u32 = 4u;
const MIN_MODEL_WEIGHT: f32 = 1e-6;

const RAY_OFFSET: f32 = 1e-3;

struct SampledRays {
    count: u32,
    rays: array<math::Ray, MAX_SAMPLE_COUNT>,
    ray_coeff: array<vec3f, MAX_SAMPLE_COUNT>, // coefficient for 3 channels
    ray_radiance: array<physics::Radiance, MAX_SAMPLE_COUNT>,
}

struct ModelHeuristics {
    diffuse: f32,
    dielectric_specular: f32,
    dielectric_rough: f32,
}

struct ModelSampleCounts {
    diffuse: u32,
    dielectric_specular: u32,
    dielectric_rough: u32,
}

fn sampleRays(
    curr_dir: vec3f,
    surface_pos: structural::SurfacePos,
    lod: lod::LOD
) -> SampledRays {
    let material: Material = material(surface_pos.material_idx, surface_pos.uvs);
    let heuristics = modelHeuristics(material);
    let counts = modelSampleCounts(
        u32(lod::heuristic(lod) * f32(MAX_SAMPLE_COUNT)),
        heuristics
    );
    let sample = sampleModels(curr_dir, surface_pos, material, counts);
    return sample;
}

fn sampleModels(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    counts: ModelSampleCounts
) -> SampledRays {
    var new_sample = SampledRays();

    let diffuse_weight: f32 = 1 - material.metallic;
    let dielectric_reflection_weight: f32 = 1 - material.metallic;

    for (var i: u32 = 0; i < counts.diffuse; i++) {
        let diffuse_sample = diffuseSample(surface_pos, curr_dir, material);
        new_sample.rays[new_sample.count] = spawnSurfaceRay(
            surface_pos,
            diffuse_sample.next_dir
        );
        new_sample.ray_coeff[new_sample.count] = diffuse_weight
            * diffuse_sample.dist
            / f32(counts.diffuse);
        new_sample.count += 1;
    }

    for (var i: u32 = 0; i < counts.dielectric_specular; i++) {
        let dielectric_specular_sample = dielectricSpecularSample(
            surface_pos,
            curr_dir,
            material
        );
        new_sample.rays[new_sample.count] = spawnSurfaceRay(
            surface_pos,
            dielectric_specular_sample.next_dir
        );
        new_sample.ray_coeff[new_sample.count] = dielectric_reflection_weight * dielectric_specular_sample.dist;
        new_sample.count += 1;
    }

    for (var i: u32 = 0; i < counts.dielectric_rough; i++) {
        let dielectric_rough_sample = dielectricRoughSample(surface_pos, curr_dir, material);
        new_sample.rays[new_sample.count] = spawnSurfaceRay(surface_pos, dielectric_rough_sample.next_dir);
        new_sample.ray_coeff[new_sample.count] = dielectric_reflection_weight
            * dielectric_rough_sample.dist
            / f32(counts.dielectric_rough);
        new_sample.count += 1;
    }

    return new_sample;
}

fn modelHeuristics(material: Material) -> ModelHeuristics {
    let diffuse_weight: f32 = 1.0f - material.metallic;
    let dielectric_reflection_weight: f32 = 1.0f - material.metallic;

    let diffuse_heuristic = select(
        0.0f,
        f32(MAX_DIFFUSE_COUNT),
        diffuse_weight > MIN_MODEL_WEIGHT,
    );
    let dielectric_specular_heuristic = select(
        0.0f,
        1.0f,
        dielectric_reflection_weight > MIN_MODEL_WEIGHT
            && material.microfacet_roughness == 0.0f,
    );
    let dielectric_rough_heuristic = select(
        0.0f,
        f32(MAX_DIELECTRIC_ROUGH_COUNT),
        dielectric_reflection_weight > MIN_MODEL_WEIGHT
            && material.microfacet_roughness > 0.0f,
    );

    return ModelHeuristics(
        diffuse_heuristic,
        dielectric_specular_heuristic,
        dielectric_rough_heuristic
    );
}

fn modelSampleCounts(
    available_count: u32,
    heuristics: ModelHeuristics
) -> ModelSampleCounts {
    let diffuse_desired = desiredSampleCount(
        heuristics.diffuse,
        MAX_DIFFUSE_COUNT
    );
    let dielectric_specular_desired = desiredSampleCount(
        heuristics.dielectric_specular,
        1u
    );
    let dielectric_rough_desired = desiredSampleCount(
        heuristics.dielectric_rough,
        MAX_DIELECTRIC_ROUGH_COUNT
    );
    let desired_count = diffuse_desired
        + dielectric_specular_desired
        + dielectric_rough_desired;

    if desired_count <= available_count {
        return ModelSampleCounts(
            diffuse_desired,
            dielectric_specular_desired,
            dielectric_rough_desired
        );
    }

    var counts = ModelSampleCounts();
    var remaining_count = available_count;

    if diffuse_desired > 0u && remaining_count > 0u {
        counts.diffuse = 1u;
        remaining_count -= 1u;
    }
    if dielectric_specular_desired > 0u && remaining_count > 0u {
        counts.dielectric_specular = 1u;
        remaining_count -= 1u;
    }
    if dielectric_rough_desired > 0u && remaining_count > 0u {
        counts.dielectric_rough = 1u;
        remaining_count -= 1u;
    }

    var diffuse_extra_cap = diffuse_desired - counts.diffuse;
    var dielectric_specular_extra_cap = dielectric_specular_desired - counts.dielectric_specular;
    var dielectric_rough_extra_cap = dielectric_rough_desired - counts.dielectric_rough;
    let extra_heuristic = select(0.0f, heuristics.diffuse, diffuse_extra_cap > 0u)
        + select(
        0.0f,
        heuristics.dielectric_specular,
        dielectric_specular_extra_cap > 0u,
    )
        + select(
        0.0f,
        heuristics.dielectric_rough,
        dielectric_rough_extra_cap > 0u,
    );

    if remaining_count > 0u && extra_heuristic > 0.0f {
        let diffuse_share = f32(remaining_count) * heuristics.diffuse / extra_heuristic;
        let dielectric_specular_share = f32(remaining_count) * heuristics.dielectric_specular
            / extra_heuristic;
        let dielectric_rough_share = f32(remaining_count) * heuristics.dielectric_rough
            / extra_heuristic;

        let diffuse_extra = min(diffuse_extra_cap, u32(diffuse_share));
        let dielectric_specular_extra = min(
            dielectric_specular_extra_cap,
            u32(dielectric_specular_share)
        );
        let dielectric_rough_extra = min(
            dielectric_rough_extra_cap,
            u32(dielectric_rough_share)
        );

        counts.diffuse += diffuse_extra;
        counts.dielectric_specular += dielectric_specular_extra;
        counts.dielectric_rough += dielectric_rough_extra;
        diffuse_extra_cap -= diffuse_extra;
        dielectric_specular_extra_cap -= dielectric_specular_extra;
        dielectric_rough_extra_cap -= dielectric_rough_extra;
        remaining_count -= diffuse_extra
            + dielectric_specular_extra
            + dielectric_rough_extra;

        var diffuse_remainder = select(
            -1.0f,
            diffuse_share - f32(diffuse_extra),
            diffuse_extra_cap > 0u,
        );
        var dielectric_specular_remainder = select(
            -1.0f,
            dielectric_specular_share - f32(dielectric_specular_extra),
            dielectric_specular_extra_cap > 0u,
        );
        var dielectric_rough_remainder = select(
            -1.0f,
            dielectric_rough_share - f32(dielectric_rough_extra),
            dielectric_rough_extra_cap > 0u,
        );

        while remaining_count > 0u {
            if diffuse_remainder >= dielectric_specular_remainder
                && diffuse_remainder >= dielectric_rough_remainder
                && diffuse_extra_cap > 0u {
                counts.diffuse += 1u;
                diffuse_extra_cap -= 1u;
                diffuse_remainder = -1.0f;
            } else if dielectric_specular_remainder >= dielectric_rough_remainder
                && dielectric_specular_extra_cap > 0u {
                counts.dielectric_specular += 1u;
                dielectric_specular_extra_cap -= 1u;
                dielectric_specular_remainder = -1.0f;
            } else if dielectric_rough_extra_cap > 0u {
                counts.dielectric_rough += 1u;
                dielectric_rough_extra_cap -= 1u;
                dielectric_rough_remainder = -1.0f;
            } else {
                break;
            }
            remaining_count -= 1u;
        }
    }

    return counts;
}

fn desiredSampleCount(heuristic: f32, max_count: u32) -> u32 {
    if heuristic <= 0.0f {
        return 0u;
    }
    return min(max(1u, u32(ceil(heuristic))), max_count);
}

fn spawnSurfaceRay(surface_pos: structural::SurfacePos, next_dir: math::Vec) -> math::Ray {
    let offset_normal = select(
        -surface_pos.geometry_norm,
        surface_pos.geometry_norm,
        dot(next_dir, surface_pos.geometry_norm) > 0.0f,
    );
    return math::Ray(surface_pos.pos + offset_normal * RAY_OFFSET, next_dir);
}
