import package::compute::math;
import package::compute::physics;
import package::compute::structural;
import package::compute::lod;
import super::material::{Material, material};
import super::model::{
    diffuse::diffuseSample,
    specular::specularSample,
    rough::roughSample,
};

const MAX_SAMPLE_COUNT: u32 = 6u;
const MAX_DIFFUSE_COUNT: u32 = 4u;
const MAX_ROUGH_COUNT: u32 = 4u;
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
    specular: f32,
    rough: f32,
}

struct ModelSampleCounts {
    diffuse: u32,
    specular: u32,
    rough: u32,
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

    for (var i: u32 = 0; i < counts.diffuse; i++) {
        let diffuse_sample = diffuseSample(surface_pos, curr_dir, material);
        new_sample.rays[new_sample.count] = spawnSurfaceRay(
            surface_pos,
            diffuse_sample.next_dir
        );
        new_sample.ray_coeff[new_sample.count] =
            diffuse_sample.dist / f32(counts.diffuse);
        new_sample.count += 1;
    }

    for (var i: u32 = 0; i < counts.specular; i++) {
        let specular_sample = specularSample(
            surface_pos,
            curr_dir,
            material
        );
        new_sample.rays[new_sample.count] = spawnSurfaceRay(
            surface_pos,
            specular_sample.next_dir
        );
        new_sample.ray_coeff[new_sample.count] = specular_sample.dist;
        new_sample.count += 1;
    }

    for (var i: u32 = 0; i < counts.rough; i++) {
        let rough_sample = roughSample(surface_pos, curr_dir, material);
        new_sample.rays[new_sample.count] =
            spawnSurfaceRay(surface_pos, rough_sample.next_dir);
        new_sample.ray_coeff[new_sample.count] =
            rough_sample.dist / f32(counts.rough);
        new_sample.count += 1;
    }

    return new_sample;
}

fn modelHeuristics(material: Material) -> ModelHeuristics {
    let diffuse_weight: f32 = max(
        material.diffuse_reflectance.r,
        max(material.diffuse_reflectance.g, material.diffuse_reflectance.b)
    );

    let diffuse_heuristic = select(
        0.0f,
        f32(MAX_DIFFUSE_COUNT),
        diffuse_weight > MIN_MODEL_WEIGHT,
    );
    let specular_heuristic = select(
        0.0f,
        1.0f,
        material.microfacet_roughness == 0.0f,
    );
    let rough_heuristic = select(
        0.0f,
        f32(MAX_ROUGH_COUNT),
        material.microfacet_roughness > 0.0f,
    );

    return ModelHeuristics(
        diffuse_heuristic,
        specular_heuristic,
        rough_heuristic
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
    let specular_desired = desiredSampleCount(
        heuristics.specular,
        1u
    );
    let rough_desired = desiredSampleCount(
        heuristics.rough,
        MAX_ROUGH_COUNT
    );
    let desired_count = diffuse_desired
        + specular_desired
        + rough_desired;

    if desired_count <= available_count {
        return ModelSampleCounts(
            diffuse_desired,
            specular_desired,
            rough_desired
        );
    }

    var counts = ModelSampleCounts();
    var remaining_count = available_count;

    if diffuse_desired > 0u && remaining_count > 0u {
        counts.diffuse = 1u;
        remaining_count -= 1u;
    }
    if specular_desired > 0u && remaining_count > 0u {
        counts.specular = 1u;
        remaining_count -= 1u;
    }
    if rough_desired > 0u && remaining_count > 0u {
        counts.rough = 1u;
        remaining_count -= 1u;
    }

    var diffuse_extra_cap = diffuse_desired - counts.diffuse;
    var specular_extra_cap = specular_desired - counts.specular;
    var rough_extra_cap = rough_desired - counts.rough;
    let extra_heuristic = select(0.0f, heuristics.diffuse, diffuse_extra_cap > 0u)
        + select(
        0.0f,
        heuristics.specular,
        specular_extra_cap > 0u,
    )
        + select(
        0.0f,
        heuristics.rough,
        rough_extra_cap > 0u,
    );

    if remaining_count > 0u && extra_heuristic > 0.0f {
        let diffuse_share = f32(remaining_count) * heuristics.diffuse / extra_heuristic;
        let specular_share = f32(remaining_count) * heuristics.specular
            / extra_heuristic;
        let rough_share = f32(remaining_count) * heuristics.rough
            / extra_heuristic;

        let diffuse_extra = min(diffuse_extra_cap, u32(diffuse_share));
        let specular_extra = min(
            specular_extra_cap,
            u32(specular_share)
        );
        let rough_extra = min(
            rough_extra_cap,
            u32(rough_share)
        );

        counts.diffuse += diffuse_extra;
        counts.specular += specular_extra;
        counts.rough += rough_extra;
        diffuse_extra_cap -= diffuse_extra;
        specular_extra_cap -= specular_extra;
        rough_extra_cap -= rough_extra;
        remaining_count -= diffuse_extra
            + specular_extra
            + rough_extra;

        var diffuse_remainder = select(
            -1.0f,
            diffuse_share - f32(diffuse_extra),
            diffuse_extra_cap > 0u,
        );
        var specular_remainder = select(
            -1.0f,
            specular_share - f32(specular_extra),
            specular_extra_cap > 0u,
        );
        var rough_remainder = select(
            -1.0f,
            rough_share - f32(rough_extra),
            rough_extra_cap > 0u,
        );

        while remaining_count > 0u {
            if diffuse_remainder >= specular_remainder
                && diffuse_remainder >= rough_remainder
                && diffuse_extra_cap > 0u {
                counts.diffuse += 1u;
                diffuse_extra_cap -= 1u;
                diffuse_remainder = -1.0f;
            } else if specular_remainder >= rough_remainder
                && specular_extra_cap > 0u {
                counts.specular += 1u;
                specular_extra_cap -= 1u;
                specular_remainder = -1.0f;
            } else if rough_extra_cap > 0u {
                counts.rough += 1u;
                rough_extra_cap -= 1u;
                rough_remainder = -1.0f;
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
