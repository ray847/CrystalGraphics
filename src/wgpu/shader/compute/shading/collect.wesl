import super::sample::{SampledRays, MAX_SAMPLE_COUNT};
import package::compute::physics;

fn collect(
    curr_dir: vec3f,
    sampled_rays: SampledRays,
) -> physics::Radiance {
    /* Monte Carlo Estimator */
    var radiance = physics::Radiance();
    for (var i: u32; i < sampled_rays.count; i++) {
        radiance += sampled_rays.ray_coeff[i] * sampled_rays.ray_radiance[i];
    }
    return radiance;
}
