import package::compute::{
  lod,
  math,
  physics,
  structural,
};
import super::{
  material::{Material, IORChannel},
  pdf::{
    diffuse::{
      dist as diffusePdf,
      sample as diffuseSample,
    },
    specular_reflection::{
      dist as specularReflectPdf,
      sample as specularReflectSample,
    },
    rough_reflection::{
      dist as roughReflectPdf,
      sample as roughReflectSample,
    },
    specular_transmission::{
      dist as specularTransmitPdf,
      sample as specularTransmitSample,
    },
    rough_transmission::{
      dist as roughTransmitPdf,
      sample as roughTransmitSample,
    },
    emission::{
      canSample as canSampleEmission,
      dist as lightPdf,
      sample as emissionSample,
    },
  },
  dist::{
    diffuse::dist as diffuseDist,
    specular_reflection::dist as specularReflectDist,
    rough_reflection::dist as roughReflectDist,
    specular_transmission::dist as specularTransmitDist,
    rough_transmission::dist as roughTransmitDist,
  },
  util::{fresnelReflectance, orientedNormal, relIor},
};

const MAX_TRANSPORT_SAMPLE_COUNT: u32 = 4u;
const MAX_EMISSION_SAMPLE_COUNT: u32 = 4u;
const MAX_DIFFUSE_COUNT: u32 = 4u;
const MAX_ROUGH_COUNT: u32 = 4u;
const MIN_SAMPLE_WEIGHT: f32 = 1e-6f;
const EMISSION_HIT_EPSILON: f32 = 1e-3f;

struct TransportSample {
    dir: math::Vec,
    coeff: vec3f,
};

struct EmissionSample {
    dir: math::Vec,
    radiance: physics::Radiance,
    expected_dist: f32,
};

struct Sample {
    transport_count: u32,
    emission_count: u32,
    transport_samples: array<TransportSample, MAX_TRANSPORT_SAMPLE_COUNT>,
    emission_samples: array<EmissionSample, MAX_EMISSION_SAMPLE_COUNT>,
};

struct SampleHeuristics {
    diffuse: f32,
    specular_reflect: f32,
    rough_reflect: f32,
    specular_transmit: f32,
    rough_transmit: f32,
    light: f32,
};

struct SampleCounts {
    diffuse: u32,
    specular_reflect: u32,
    rough_reflect: u32,
    specular_transmit: u32,
    rough_transmit: u32,
    light: u32,
};

fn sample(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    curr_lod: lod::LOD
) -> Sample {
    let sample_heuristics = heuristics(material);
    let sample_counts = counts(sample_heuristics, curr_lod);
    return sampleModels(curr_dir, surface_pos, material, sample_counts);
}

fn heuristics(material: Material) -> SampleHeuristics {
    let diffuse_heuristic = max(
        material.diffuse_reflectance.r,
        max(material.diffuse_reflectance.g, material.diffuse_reflectance.b)
    );
    let reflection_heuristic = normalReflectance(material);
    let transmission_heuristic = material.transmission;
    let specular_reflect_heuristic = select(
        0.0f,
        reflection_heuristic,
        material.microfacet_roughness == 0.0f
    );
    let rough_reflect_heuristic = select(
        0.0f,
        reflection_heuristic,
        material.microfacet_roughness > 0.0f
    );
    let specular_transmit_heuristic = select(
        0.0f,
        transmission_heuristic,
        transmission_heuristic > MIN_SAMPLE_WEIGHT
            && material.microfacet_roughness == 0.0f
    );
    let rough_transmit_heuristic = select(
        0.0f,
        transmission_heuristic,
        transmission_heuristic > MIN_SAMPLE_WEIGHT
            && material.microfacet_roughness > 0.0f
    );

    return SampleHeuristics(
        diffuse_heuristic,
        specular_reflect_heuristic,
        rough_reflect_heuristic,
        specular_transmit_heuristic,
        rough_transmit_heuristic,
        select(0.0f, 1.0f, canSampleEmission())
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
                MIN_SAMPLE_WEIGHT
            ),
        0.0f,
        1.0f
    );
}

fn counts(sample_heuristics: SampleHeuristics, curr_lod: lod::LOD) -> SampleCounts {
    let transport_available = u32(
        lod::heuristic(curr_lod) * f32(MAX_TRANSPORT_SAMPLE_COUNT)
    );
    let emission_available = u32(
        lod::heuristic(curr_lod) * f32(MAX_EMISSION_SAMPLE_COUNT)
    );

    var sample_counts = transportCounts(transport_available, sample_heuristics);
    sample_counts.light = select(
        0u,
        max(1u, emission_available),
        sample_heuristics.light > MIN_SAMPLE_WEIGHT
            && emission_available > 0u
    );
    return sample_counts;
}

fn transportCounts(
    available_count: u32,
    sample_heuristics: SampleHeuristics
) -> SampleCounts {
    let heuristic_sum = sample_heuristics.diffuse
        + sample_heuristics.specular_reflect
        + sample_heuristics.rough_reflect
        + sample_heuristics.specular_transmit
        + sample_heuristics.rough_transmit;
    if available_count == 0u || heuristic_sum <= MIN_SAMPLE_WEIGHT {
        return SampleCounts();
    }

    let sample_scale = f32(available_count) / heuristic_sum;
    var sample_counts = SampleCounts(
        scaledCount(sample_heuristics.diffuse, sample_scale, MAX_DIFFUSE_COUNT),
        scaledCount(sample_heuristics.specular_reflect, sample_scale, 1u),
        scaledCount(sample_heuristics.rough_reflect, sample_scale,
            MAX_ROUGH_COUNT),
        scaledCount(sample_heuristics.specular_transmit, sample_scale, 1u),
        scaledCount(sample_heuristics.rough_transmit, sample_scale,
            MAX_ROUGH_COUNT),
        0u
    );

    return capTransportCounts(sample_counts, available_count);
}

fn scaledCount(heuristic: f32, scale: f32, max_count: u32) -> u32 {
    if heuristic <= MIN_SAMPLE_WEIGHT {
        return 0u;
    }
    return min(max(1u, u32(heuristic * scale)), max_count);
}

fn capTransportCounts(
    sample_counts: SampleCounts,
    available_count: u32
) -> SampleCounts {
    var remaining_count = available_count;
    var capped_counts = SampleCounts();

    capped_counts.diffuse = min(sample_counts.diffuse, remaining_count);
    remaining_count -= capped_counts.diffuse;
    capped_counts.specular_reflect = min(
        sample_counts.specular_reflect,
        remaining_count
    );
    remaining_count -= capped_counts.specular_reflect;
    capped_counts.rough_reflect = min(sample_counts.rough_reflect,
        remaining_count);
    remaining_count -= capped_counts.rough_reflect;
    capped_counts.specular_transmit = min(
        sample_counts.specular_transmit,
        remaining_count
    );
    remaining_count -= capped_counts.specular_transmit;
    capped_counts.rough_transmit = min(sample_counts.rough_transmit,
        remaining_count);
    return capped_counts;
}

fn sampleModels(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_counts: SampleCounts
) -> Sample {
    var res = Sample();

    for (var i: u32 = 0u; i < sample_counts.diffuse; i++) {
        let model_sample = diffuseSample(curr_dir, surface_pos, material);
        addTransportSample(
            &res,
            model_sample.next_dir,
            diffuseCoeff(curr_dir, model_sample.next_dir, surface_pos, material,
                sample_counts)
        );
    }

    for (var i: u32 = 0u; i < sample_counts.specular_reflect; i++) {
        let model_sample = specularReflectSample(curr_dir, surface_pos,
            material);
        addTransportSample(
            &res,
            model_sample.next_dir,
            specularReflectCoeff(curr_dir, model_sample.next_dir, surface_pos,
                material, sample_counts)
        );
    }

    for (var i: u32 = 0u; i < sample_counts.rough_reflect; i++) {
        let model_sample = roughReflectSample(curr_dir, surface_pos, material);
        addTransportSample(
            &res,
            model_sample.next_dir,
            roughReflectCoeff(curr_dir, model_sample.next_dir, surface_pos,
                material, sample_counts)
        );
    }

    for (var i: u32 = 0u; i < sample_counts.specular_transmit; i++) {
        let model_sample = specularTransmitSample(curr_dir, surface_pos,
            material);
        addTransportSample(
            &res,
            model_sample.next_dir,
            specularTransmitCoeff(curr_dir, model_sample.next_dir, surface_pos,
                material, sample_counts)
        );
    }

    for (var i: u32 = 0u; i < sample_counts.rough_transmit; i++) {
        let model_sample = roughTransmitSample(curr_dir, surface_pos, material);
        addTransportSample(
            &res,
            model_sample.next_dir,
            roughTransmitCoeff(curr_dir, model_sample.next_dir, surface_pos,
                material, sample_counts)
        );
    }

    for (var i: u32 = 0u; i < sample_counts.light; i++) {
        let model_sample = emissionSample(curr_dir, surface_pos, material);
        addEmissionSample(
            &res,
            curr_dir,
            surface_pos,
            material,
            model_sample,
            sample_counts
        );
    }

    return res;
}

fn addTransportSample(
    samples: ptr<function, Sample>,
    next_dir: math::Vec,
    coeff: vec3f
) {
    let idx = (*samples).transport_count;
    (*samples).transport_samples[idx] = TransportSample(next_dir, coeff);
    (*samples).transport_count += 1u;
}

fn addEmissionSample(
    samples: ptr<function, Sample>,
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    model_sample: super::pdf::sample::EmissionSample,
    sample_counts: SampleCounts
) {
    if model_sample.prob <= MIN_SAMPLE_WEIGHT {
        return;
    }

    let idx = (*samples).emission_count;
    let next_dir = model_sample.next_dir;
    let surface_projection = abs(dot(surface_pos.shading_norm, next_dir));
    (*samples).emission_samples[idx] = EmissionSample(
        next_dir,
        model_sample.radiance
        * emissionCoeff(curr_dir, next_dir, surface_pos, material)
        * surface_projection
        / balancePdfDenominator(
            curr_dir,
            next_dir,
            surface_pos,
            material,
            sample_counts,
            model_sample.prob
        ),
        model_sample.expected_dist
    );
    (*samples).emission_count += 1u;
}

fn diffuseCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_counts: SampleCounts
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let next_projection = max(dot(next_dir, macro_norm), 0.0f);
    return material.diffuse_reflectance.rgb
        * diffuseDist(curr_dir, next_dir, surface_pos, material)
        * next_projection
        / balancePdfDenominator(
            curr_dir,
            next_dir,
            surface_pos,
            material,
            sample_counts,
            -1.0f
        );
}

fn specularReflectCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_counts: SampleCounts
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    return fresnelReflectance(curr_dir, macro_norm, material.ior)
        * specularReflectDist(curr_dir, next_dir, surface_pos, material)
        / balancePdfDenominator(
            curr_dir,
            next_dir,
            surface_pos,
            material,
            sample_counts,
            -1.0f
        );
}

fn roughReflectCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_counts: SampleCounts
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let next_projection = max(dot(next_dir, macro_norm), 0.0f);
    let micro_norm = reflectionMicroNorm(curr_dir, next_dir);
    return fresnelReflectance(curr_dir, micro_norm, material.ior)
        * roughReflectDist(curr_dir, next_dir, surface_pos, material)
        * next_projection
        / balancePdfDenominator(
            curr_dir,
            next_dir,
            surface_pos,
            material,
            sample_counts,
            -1.0f
        );
}

fn specularTransmitCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_counts: SampleCounts
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    return material.transmission
        * (1.0f - fresnelReflectance(curr_dir, macro_norm, material.ior))
        * specularTransmitDist(curr_dir, next_dir, surface_pos, material)
        / balancePdfDenominator(
            curr_dir,
            next_dir,
            surface_pos,
            material,
            sample_counts,
            -1.0f
        );
}

fn roughTransmitCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_counts: SampleCounts
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let next_projection = max(dot(-next_dir, macro_norm), 0.0f);
    let micro_norm = transmissionMicroNorm(curr_dir, next_dir, macro_norm,
        material);
    return (1.0f - fresnelReflectance(curr_dir, micro_norm, material.ior))
        * roughTransmitDist(curr_dir, next_dir, surface_pos, material)
        * next_projection
        / balancePdfDenominator(
            curr_dir,
            next_dir,
            surface_pos,
            material,
            sample_counts,
            -1.0f
        );
}

fn emissionCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> vec3f {
    let macro_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    let reflection_micro_norm = reflectionMicroNorm(curr_dir, next_dir);
    let transmission_micro_norm = transmissionMicroNorm(
        curr_dir,
        next_dir,
        macro_norm,
        material
    );
    return material.diffuse_reflectance.rgb
        * diffuseDist(curr_dir, next_dir, surface_pos, material)
        + fresnelReflectance(curr_dir, reflection_micro_norm, material.ior)
        * roughReflectDist(curr_dir, next_dir, surface_pos, material)
        + (1.0f - fresnelReflectance(
            curr_dir,
            transmission_micro_norm,
            material.ior
        ))
        * roughTransmitDist(curr_dir, next_dir, surface_pos, material);
}

fn balancePdfDenominator(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_counts: SampleCounts,
    sampled_light_pdf: f32
) -> f32 {
    let light_pdf = select(
        lightPdf(curr_dir, next_dir, surface_pos, material),
        sampled_light_pdf,
        sampled_light_pdf >= 0.0f
    );
    return max(
        f32(sample_counts.diffuse)
            * diffusePdf(curr_dir, next_dir, surface_pos, material)
        + f32(sample_counts.specular_reflect)
            * specularReflectPdf(curr_dir, next_dir, surface_pos, material)
        + f32(sample_counts.rough_reflect)
            * roughReflectPdf(curr_dir, next_dir, surface_pos, material)
        + f32(sample_counts.specular_transmit)
            * specularTransmitPdf(curr_dir, next_dir, surface_pos, material)
        + f32(sample_counts.rough_transmit)
            * roughTransmitPdf(curr_dir, next_dir, surface_pos, material)
        + f32(sample_counts.light) * light_pdf,
        MIN_SAMPLE_WEIGHT
    );
}

fn reflectionMicroNorm(curr_dir: math::Vec, next_dir: math::Vec) -> math::Vec {
    let micro_norm_unnormalized = next_dir - curr_dir;
    if dot(micro_norm_unnormalized, micro_norm_unnormalized)
        <= MIN_SAMPLE_WEIGHT {
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
        MIN_SAMPLE_WEIGHT
    );
    let micro_norm_unnormalized = relative_ior_value * curr_dir - next_dir;
    if dot(micro_norm_unnormalized, micro_norm_unnormalized)
        <= MIN_SAMPLE_WEIGHT {
        return math::Vec();
    }
    return orientedNormal(curr_dir, normalize(micro_norm_unnormalized));
}
