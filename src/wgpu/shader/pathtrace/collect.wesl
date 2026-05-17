import package::pathtrace::{
  sample::{SampledRays, MAX_SAMPLE_COUNT},
  radiance::Radiance,
  structural::SurfacePos,
};

fn collect(
    dir: vec3f,
    sampled_rays: SampledRays,
    ray_radiance: array<Radiance, MAX_SAMPLE_COUNT>
) -> Radiance {
    /* Monte Carlo Estimator */
    var radiance = Radiance();
    for (var i: i32; i < sampled_rays.count; i++) {
        radiance += ray_radiance[i] * sampled_rays.ray_coeff[i];
    }
    return radiance;
}
