import package::compute::{
  lod,
  math,
  physics,
  structural,
};
import super::{
  material::Material,
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
};

const MAX_TRANSPORT_SAMPLE_COUNT: u32 = 4u;
const MAX_EMISSION_SAMPLE_COUNT: u32 = 1u;
const MAX_DIFFUSE_COUNT: u32 = 1u;
const MAX_ROUGH_COUNT: u32 = 1u;
@id(5) override TRANSPORT_SAMPLE_COUNT: u32;
@id(6) override EMISSION_SAMPLE_COUNT: u32;
@id(7) override DIFFUSE_SAMPLE_COUNT: u32;
@id(8) override ROUGH_SAMPLE_COUNT: u32;
const MIN_SAMPLE_WEIGHT: f32 = 1e-6f;
const EMISSION_HIT_EPSILON: f32 = 1e-3f;
const SAMPLE_MODEL_NONE: u32 = 0u;
const SAMPLE_MODEL_DIFFUSE: u32 = 1u;
const SAMPLE_MODEL_SPECULAR_REFLECT: u32 = 2u;
const SAMPLE_MODEL_ROUGH_REFLECT: u32 = 3u;
const SAMPLE_MODEL_SPECULAR_TRANSMIT: u32 = 4u;
const SAMPLE_MODEL_ROUGH_TRANSMIT: u32 = 5u;

struct TransportSample {
    dir: math::Vec,
    coeff: physics::Radiance,
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
    wavelength: physics::Wavelength,
    surface_pos: structural::SurfacePos,
    material: Material,
    curr_lod: lod::LOD
) -> Sample {
    let sample_heuristics = heuristics(material);
    let sample_counts = counts(sample_heuristics, curr_lod);
    return sampleModels(curr_dir, wavelength, surface_pos, material, sample_counts);
}

fn heuristics(material: Material) -> SampleHeuristics {
    let diffuse_heuristic = material.diffuse_reflectance;
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
    let real_offset_low = material.ior.re - 1.0f;
    let real_offset_high = material.ior.re + 1.0f;
    let imaginary_squared = material.ior.im * material.ior.im;
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
        lod::heuristic(curr_lod)
            * f32(min(TRANSPORT_SAMPLE_COUNT, MAX_TRANSPORT_SAMPLE_COUNT))
    );
    let emission_available = u32(
        lod::heuristic(curr_lod)
            * f32(min(EMISSION_SAMPLE_COUNT, MAX_EMISSION_SAMPLE_COUNT))
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
        scaledCount(
            sample_heuristics.diffuse,
            sample_scale,
            min(DIFFUSE_SAMPLE_COUNT, MAX_DIFFUSE_COUNT)
        ),
        scaledCount(sample_heuristics.specular_reflect, sample_scale, 1u),
        scaledCount(sample_heuristics.rough_reflect, sample_scale,
            min(ROUGH_SAMPLE_COUNT, MAX_ROUGH_COUNT)),
        scaledCount(sample_heuristics.specular_transmit, sample_scale, 1u),
        scaledCount(sample_heuristics.rough_transmit, sample_scale,
            min(ROUGH_SAMPLE_COUNT, MAX_ROUGH_COUNT)),
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
    wavelength: physics::Wavelength,
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
            transportCoeff(
                curr_dir,
                model_sample.next_dir,
                surface_pos,
                material,
                sample_counts,
                SAMPLE_MODEL_DIFFUSE,
                model_sample.prob,
                diffuseDist(curr_dir, model_sample.next_dir, surface_pos,
                    material)
            )
        );
    }

    for (var i: u32 = 0u; i < sample_counts.specular_reflect; i++) {
        let model_sample = specularReflectSample(curr_dir, surface_pos,
            material);
        addTransportSample(
            &res,
            model_sample.next_dir,
            transportCoeff(
                curr_dir,
                model_sample.next_dir,
                surface_pos,
                material,
                sample_counts,
                SAMPLE_MODEL_SPECULAR_REFLECT,
                model_sample.prob,
                specularReflectDist(curr_dir, model_sample.next_dir,
                    surface_pos, material)
            )
        );
    }

    for (var i: u32 = 0u; i < sample_counts.rough_reflect; i++) {
        let model_sample = roughReflectSample(curr_dir, surface_pos, material);
        addTransportSample(
            &res,
            model_sample.next_dir,
            transportCoeff(
                curr_dir,
                model_sample.next_dir,
                surface_pos,
                material,
                sample_counts,
                SAMPLE_MODEL_ROUGH_REFLECT,
                model_sample.prob,
                roughReflectDist(curr_dir, model_sample.next_dir, surface_pos,
                    material)
            )
        );
    }

    for (var i: u32 = 0u; i < sample_counts.specular_transmit; i++) {
        let model_sample = specularTransmitSample(curr_dir, surface_pos,
            material);
        addTransportSample(
            &res,
            model_sample.next_dir,
            transportCoeff(
                curr_dir,
                model_sample.next_dir,
                surface_pos,
                material,
                sample_counts,
                SAMPLE_MODEL_SPECULAR_TRANSMIT,
                model_sample.prob,
                specularTransmitDist(curr_dir, model_sample.next_dir,
                    surface_pos, material)
            )
        );
    }

    for (var i: u32 = 0u; i < sample_counts.rough_transmit; i++) {
        let model_sample = roughTransmitSample(curr_dir, surface_pos, material);
        addTransportSample(
            &res,
            model_sample.next_dir,
            transportCoeff(
                curr_dir,
                model_sample.next_dir,
                surface_pos,
                material,
                sample_counts,
                SAMPLE_MODEL_ROUGH_TRANSMIT,
                model_sample.prob,
                roughTransmitDist(curr_dir, model_sample.next_dir, surface_pos,
                    material)
            )
        );
    }

    for (var i: u32 = 0u; i < sample_counts.light; i++) {
        let model_sample = emissionSample(
            curr_dir,
            wavelength,
            surface_pos,
            material
        );
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
    coeff: physics::Radiance
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
            SAMPLE_MODEL_NONE,
            -1.0f,
            model_sample.prob
        ),
        model_sample.expected_dist
    );
    (*samples).emission_count += 1u;
}

fn transportCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_counts: SampleCounts,
    sampled_transport_model: u32,
    sampled_transport_pdf: f32,
    dist: f32
) -> physics::Radiance {
    return dist / balancePdfDenominator(
        curr_dir,
        next_dir,
        surface_pos,
        material,
        sample_counts,
        sampled_transport_model,
        sampled_transport_pdf,
        -1.0f
    );
}

fn emissionCoeff(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material
) -> physics::Radiance {
    return diffuseDist(curr_dir, next_dir, surface_pos, material)
        + roughReflectDist(curr_dir, next_dir, surface_pos, material)
        + roughTransmitDist(curr_dir, next_dir, surface_pos, material);
}

fn balancePdfDenominator(
    curr_dir: math::Vec,
    next_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    material: Material,
    sample_counts: SampleCounts,
    sampled_transport_model: u32,
    sampled_transport_pdf: f32,
    sampled_light_pdf: f32
) -> f32 {
    let diffuse_pdf = select(
        diffusePdf(curr_dir, next_dir, surface_pos, material),
        sampled_transport_pdf,
        sampled_transport_model == SAMPLE_MODEL_DIFFUSE
    );
    let specular_reflect_pdf = select(
        specularReflectPdf(curr_dir, next_dir, surface_pos, material),
        sampled_transport_pdf,
        sampled_transport_model == SAMPLE_MODEL_SPECULAR_REFLECT
    );
    let rough_reflect_pdf = select(
        roughReflectPdf(curr_dir, next_dir, surface_pos, material),
        sampled_transport_pdf,
        sampled_transport_model == SAMPLE_MODEL_ROUGH_REFLECT
    );
    let specular_transmit_pdf = select(
        specularTransmitPdf(curr_dir, next_dir, surface_pos, material),
        sampled_transport_pdf,
        sampled_transport_model == SAMPLE_MODEL_SPECULAR_TRANSMIT
    );
    let rough_transmit_pdf = select(
        roughTransmitPdf(curr_dir, next_dir, surface_pos, material),
        sampled_transport_pdf,
        sampled_transport_model == SAMPLE_MODEL_ROUGH_TRANSMIT
    );
    let light_pdf = select(
        lightPdf(curr_dir, next_dir, surface_pos, material),
        sampled_light_pdf,
        sampled_light_pdf >= 0.0f
    );
    return max(
        f32(sample_counts.diffuse) * diffuse_pdf
        + f32(sample_counts.specular_reflect) * specular_reflect_pdf
        + f32(sample_counts.rough_reflect) * rough_reflect_pdf
        + f32(sample_counts.specular_transmit) * specular_transmit_pdf
        + f32(sample_counts.rough_transmit) * rough_transmit_pdf
        + f32(sample_counts.light) * light_pdf,
        MIN_SAMPLE_WEIGHT
    );
}

