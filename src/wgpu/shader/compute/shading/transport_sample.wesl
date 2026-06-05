import package::compute::math;
import package::compute::physics;
import package::compute::structural;
import package::compute::lod;
import super::material::{Material, IORChannel, material};
import super::pdf::{
    diffuse::{sample as diffuseSample},
    specular_reflection::{sample as specularReflectSample},
    rough_reflection::{sample as roughReflectSample},
    specular_transmission::{sample as specularTransmitSample},
    rough_transmission::{sample as roughTransmitSample},
};
import super::dist::{
    diffuse::{dist as diffuseDist},
    specular_reflection::{dist as specularReflectDist},
    rough_reflection::{dist as roughReflectDist},
    specular_transmission::{dist as specularTransmitDist},
    rough_transmission::{dist as roughTransmitDist},
};
import super::util::{fresnelReflectance, orientedNormal, relIor};
import super::spawn_surface_ray::{spawnSurfaceRay};

const MAX_TRANSPORT_SAMPLE_COUNT: u32 = 4u;
const MAX_DIFFUSE_COUNT: u32 = 4u;
const MAX_ROUGH_COUNT: u32 = 4u;
const MIN_MODEL_WEIGHT: f32 = 1e-6;

struct TransportSamples {
    count: u32,
    rays: array<math::Ray, MAX_TRANSPORT_SAMPLE_COUNT>,
    ray_coeff: array<vec3f, MAX_TRANSPORT_SAMPLE_COUNT>, // coefficient for 3 channels
    ray_radiance: array<physics::Radiance, MAX_TRANSPORT_SAMPLE_COUNT>,
}

struct TransportModelHeuristics {
    diffuse: f32,
    specular_reflection: f32,
    rough_reflection: f32,
    specular_transmission: f32,
    rough_transmission: f32,
}

struct TransportModelSampleCounts {
    diffuse: u32,
    specular_reflect: u32,
    rough_reflect: u32,
    specular_transmit: u32,
    rough_transmit: u32,
}

fn sampleTransportRays(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    lod: lod::LOD
) -> TransportSamples {
    let heuristics = transportModelHeuristics(material);
    let counts = transportModelSampleCounts(
        u32(lod::heuristic(lod) * f32(MAX_TRANSPORT_SAMPLE_COUNT)),
        heuristics
    );
    let sample = sampleTransportModels(curr_dir, surface_pos, material, counts);
    return sample;
}

fn transportModelHeuristics(material: Material) -> TransportModelHeuristics {
    let diffuse_weight = max(
        material.diffuse_reflectance.r,
        max(material.diffuse_reflectance.g, material.diffuse_reflectance.b)
    );
    let reflection_weight = normalReflectance(material);
    let transmission_weight = material.transmission;
    let diffuse_heuristic = select(
        0.0f,
        diffuse_weight,
        diffuse_weight > MIN_MODEL_WEIGHT,
    );
    let specular_reflection_heuristic: f32 = select(
        0.0f,
        reflection_weight,
        material.microfacet_roughness == 0.0f,
    );
    let rough_reflection_heuristic: f32 = select(
        0.0f,
        reflection_weight,
        material.microfacet_roughness > 0.0f,
    );
    let specular_transmission_heuristic: f32 = select(
        0.0f,
        transmission_weight,
        transmission_weight > MIN_MODEL_WEIGHT
            && material.microfacet_roughness == 0.0f,
    );
    let rough_transmission_heuristic: f32 = select(
        0.0f,
        transmission_weight,
        transmission_weight > MIN_MODEL_WEIGHT
            && material.microfacet_roughness > 0.0f,
    );

    return TransportModelHeuristics(
        diffuse_heuristic,
        specular_reflection_heuristic,
        rough_reflection_heuristic,
        specular_transmission_heuristic,
        rough_transmission_heuristic
    );
}

fn normalReflectance(material: Material) -> f32 {
    return max(
        normalReflectanceChannel(material.ior.r),
        max(
            normalReflectanceChannel(material.ior.g),
            normalReflectanceChannel(material.ior.b)
        )
    );
}

fn normalReflectanceChannel(channel: IORChannel) -> f32 {
    let real_offset_low = channel.re - 1.0f;
    let real_offset_high = channel.re + 1.0f;
    let imaginary_squared = channel.im * channel.im;
    return clamp(
        (real_offset_low * real_offset_low + imaginary_squared)
            / max(
            real_offset_high * real_offset_high + imaginary_squared,
            MIN_MODEL_WEIGHT,
        ),
        0.0f,
        1.0f,
    );
}

fn transportModelSampleCounts(
    available_count: u32,
    heuristics: TransportModelHeuristics
) -> TransportModelSampleCounts {
    let heuristic_sum: f32 = heuristics.diffuse
        + heuristics.specular_reflection
        + heuristics.rough_reflection
        + heuristics.specular_transmission
        + heuristics.rough_transmission;
    if heuristic_sum <= 0.0f {
        return TransportModelSampleCounts();
    }

    let diffuse_desired = desiredSampleCount(
        heuristics.diffuse,
        MAX_DIFFUSE_COUNT
    );
    let specular_reflect_desired = desiredSampleCount(
        heuristics.specular_reflection,
        1u
    );
    let rough_reflect_desired = desiredSampleCount(
        heuristics.rough_reflection,
        MAX_ROUGH_COUNT
    );
    let specular_transmit_desired = desiredSampleCount(
        heuristics.specular_transmission,
        1u
    );
    let rough_transmit_desired = desiredSampleCount(
        heuristics.rough_transmission,
        MAX_ROUGH_COUNT
    );

    let desired_count = diffuse_desired
        + specular_reflect_desired
        + rough_reflect_desired
        + specular_transmit_desired
        + rough_transmit_desired;
    if desired_count <= available_count {
        return TransportModelSampleCounts(
            diffuse_desired,
            specular_reflect_desired,
            rough_reflect_desired,
            specular_transmit_desired,
            rough_transmit_desired
        );
    }

    let available_float = f32(available_count);
    let diffuse_share = available_float * heuristics.diffuse / heuristic_sum;
    let specular_reflect_share = available_float * heuristics.specular_reflection / heuristic_sum;
    let rough_reflect_share = available_float * heuristics.rough_reflection / heuristic_sum;
    let specular_transmit_share = available_float * heuristics.specular_transmission / heuristic_sum;
    let rough_transmit_share = available_float * heuristics.rough_transmission / heuristic_sum;

    var counts = TransportModelSampleCounts(
        min(diffuse_desired, u32(diffuse_share)),
        min(specular_reflect_desired, u32(specular_reflect_share)),
        min(rough_reflect_desired, u32(rough_reflect_share)),
        min(specular_transmit_desired, u32(specular_transmit_share)),
        min(rough_transmit_desired, u32(rough_transmit_share))
    );
    var remaining_count = available_count
        - counts.diffuse
        - counts.specular_reflect
        - counts.rough_reflect
        - counts.specular_transmit
        - counts.rough_transmit;

    var diffuse_cap = diffuse_desired - counts.diffuse;
    var specular_reflect_cap = specular_reflect_desired - counts.specular_reflect;
    var rough_reflect_cap = rough_reflect_desired - counts.rough_reflect;
    var specular_transmit_cap = specular_transmit_desired - counts.specular_transmit;
    var rough_transmit_cap = rough_transmit_desired - counts.rough_transmit;

    var diffuse_remainder = select(
        -1.0f,
        diffuse_share - f32(counts.diffuse),
        diffuse_cap > 0u,
    );
    var specular_reflect_remainder = select(
        -1.0f,
        specular_reflect_share - f32(counts.specular_reflect),
        specular_reflect_cap > 0u,
    );
    var rough_reflect_remainder = select(
        -1.0f,
        rough_reflect_share - f32(counts.rough_reflect),
        rough_reflect_cap > 0u,
    );
    var specular_transmit_remainder = select(
        -1.0f,
        specular_transmit_share - f32(counts.specular_transmit),
        specular_transmit_cap > 0u,
    );
    var rough_transmit_remainder = select(
        -1.0f,
        rough_transmit_share - f32(counts.rough_transmit),
        rough_transmit_cap > 0u,
    );

    while remaining_count > 0u {
        if specular_transmit_remainder >= diffuse_remainder
            && specular_transmit_remainder >= specular_reflect_remainder
            && specular_transmit_remainder >= rough_reflect_remainder
            && specular_transmit_remainder >= rough_transmit_remainder
            && specular_transmit_cap > 0u {
            counts.specular_transmit += 1u;
            specular_transmit_cap -= 1u;
            specular_transmit_remainder = -1.0f;
        } else if diffuse_remainder >= specular_reflect_remainder
            && diffuse_remainder >= rough_reflect_remainder
            && diffuse_remainder >= rough_transmit_remainder
            && diffuse_cap > 0u {
            counts.diffuse += 1u;
            diffuse_cap -= 1u;
            diffuse_remainder = -1.0f;
        } else if specular_reflect_remainder >= rough_reflect_remainder
            && specular_reflect_remainder >= rough_transmit_remainder
            && specular_reflect_cap > 0u {
            counts.specular_reflect += 1u;
            specular_reflect_cap -= 1u;
            specular_reflect_remainder = -1.0f;
        } else if rough_reflect_remainder >= rough_transmit_remainder
            && rough_reflect_cap > 0u {
            counts.rough_reflect += 1u;
            rough_reflect_cap -= 1u;
            rough_reflect_remainder = -1.0f;
        } else if rough_transmit_cap > 0u {
            counts.rough_transmit += 1u;
            rough_transmit_cap -= 1u;
            rough_transmit_remainder = -1.0f;
        } else {
            break;
        }
        remaining_count -= 1u;
    }

    return counts;
}

fn desiredSampleCount(heuristic: f32, max_count: u32) -> u32 {
    if heuristic <= 0.0f {
        return 0u;
    }
    return min(max(1u, u32(ceil(heuristic))), max_count);
}

fn sampleTransportModels(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    counts: TransportModelSampleCounts
) -> TransportSamples {
    var transport_samples = TransportSamples();

    for (var i: u32 = 0; i < counts.diffuse; i++) {
        let sample = diffuseSample(curr_dir, surface_pos, material);
        transport_samples.rays[transport_samples.count] = spawnSurfaceRay(
            surface_pos,
            sample.next_dir
        );
        transport_samples.ray_coeff[transport_samples.count] = \
            diffuseCoeff(curr_dir, sample.next_dir, surface_pos, material,
                sample.prob) / f32(counts.diffuse);
        transport_samples.count += 1;
    }

    for (var i: u32 = 0; i < counts.specular_reflect; i++) {
        let sample = specularReflectSample(
            curr_dir,
            surface_pos,
            material
        );
        transport_samples.rays[transport_samples.count] = spawnSurfaceRay(
            surface_pos,
            sample.next_dir
        );
        transport_samples.ray_coeff[transport_samples.count] = \
            specularReflectCoeff(curr_dir, sample.next_dir, surface_pos,
                material, sample.prob);
        transport_samples.count += 1;
    }

    for (var i: u32 = 0; i < counts.rough_reflect; i++) {
        let sample = roughReflectSample(curr_dir, surface_pos, material);
        transport_samples.rays[transport_samples.count] = spawnSurfaceRay(
            surface_pos,
            sample.next_dir
        );
        transport_samples.ray_coeff[transport_samples.count] = \
            roughReflectCoeff(curr_dir, sample.next_dir, surface_pos, material,
                sample.prob) / f32(counts.rough_reflect);
        transport_samples.count += 1;
    }

    for (var i: u32 = 0u; i < counts.specular_transmit; i++) {
        let sample = specularTransmitSample(curr_dir, surface_pos, material);
        transport_samples.rays[transport_samples.count] = spawnSurfaceRay(
            surface_pos,
            sample.next_dir
        );
        transport_samples.ray_coeff[transport_samples.count] = \
            specularTransmitCoeff(curr_dir, sample.next_dir, surface_pos,
                material, sample.prob);
        transport_samples.count += 1;
    }

    for (var i: u32 = 0u; i < counts.rough_transmit; i++) {
        let sample = roughTransmitSample(curr_dir, surface_pos, material);
        transport_samples.rays[transport_samples.count] = spawnSurfaceRay(
            surface_pos,
            sample.next_dir
        );
        transport_samples.ray_coeff[transport_samples.count] = \
            roughTransmitCoeff(curr_dir, sample.next_dir, surface_pos, material,
                sample.prob) / f32(counts.rough_transmit);
        transport_samples.count += 1;
    }

    return transport_samples;
}

fn diffuseCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_prob: f32
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let next_projection = max(dot(next_dir, macro_norm), 0.0f);
    return material.diffuse_reflectance.rgb
        * diffuseDist(curr_dir, next_dir, surface_pos, material)
        * next_projection
        / max(sample_prob, MIN_MODEL_WEIGHT);
}

fn specularReflectCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_prob: f32
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    return fresnelReflectance(curr_dir, macro_norm, material.ior)
        * specularReflectDist(curr_dir, next_dir, surface_pos, material)
        / max(sample_prob, MIN_MODEL_WEIGHT);
}

fn roughReflectCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_prob: f32
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let next_projection = max(dot(next_dir, macro_norm), 0.0f);
    let micro_norm = reflectionMicroNorm(curr_dir, next_dir);
    return fresnelReflectance(curr_dir, micro_norm, material.ior)
        * roughReflectDist(curr_dir, next_dir, surface_pos, material)
        * next_projection
        / max(sample_prob, MIN_MODEL_WEIGHT);
}

fn specularTransmitCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_prob: f32
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    return material.transmission
        * (1.0f - fresnelReflectance(curr_dir, macro_norm, material.ior))
        * specularTransmitDist(curr_dir, next_dir, surface_pos, material)
        / max(sample_prob, MIN_MODEL_WEIGHT);
}

fn roughTransmitCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_prob: f32
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let next_projection = max(dot(-next_dir, macro_norm), 0.0f);
    let micro_norm = transmissionMicroNorm(curr_dir, next_dir, macro_norm,
        material);
    return (1.0f - fresnelReflectance(curr_dir, micro_norm, material.ior))
        * roughTransmitDist(curr_dir, next_dir, surface_pos, material)
        * next_projection
        / max(sample_prob, MIN_MODEL_WEIGHT);
}

fn reflectionMicroNorm(curr_dir: math::Vec, next_dir: math::Vec) -> math::Vec {
    let micro_norm_unnormalized = next_dir - curr_dir;
    if dot(micro_norm_unnormalized, micro_norm_unnormalized)
        <= MIN_MODEL_WEIGHT {
        return math::Vec();
    }
    return normalize(micro_norm_unnormalized);
}

fn transmissionMicroNorm(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    macro_norm: math::Vec,
    material: Material
) -> math::Vec {
    let relative_ior = relIor(macro_norm, curr_dir, material);
    let relative_ior_value = max(
        (relative_ior.r.re + relative_ior.g.re + relative_ior.b.re) / 3.0f,
        MIN_MODEL_WEIGHT
    );
    let micro_norm_unnormalized = relative_ior_value * curr_dir - next_dir;
    if dot(micro_norm_unnormalized, micro_norm_unnormalized)
        <= MIN_MODEL_WEIGHT {
        return math::Vec();
    }
    return orientedNormal(curr_dir, normalize(micro_norm_unnormalized));
}
