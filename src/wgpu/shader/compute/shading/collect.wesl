import super::sample::{Sample, MAX_TRANSPORT_SAMPLE_COUNT};
import package::compute::physics;

fn collect(
    samples: Sample,
    transport_radiance: array<physics::Radiance, MAX_TRANSPORT_SAMPLE_COUNT>
) -> physics::Radiance {
    var radiance = physics::Radiance();
    for (var i: u32; i < samples.transport_count; i++) {
        radiance += samples.transport_samples[i].coeff
            * transport_radiance[i];
    }
    for (var i: u32; i < samples.emission_count; i++) {
        radiance += samples.emission_samples[i].radiance;
    }
    return radiance;
}
