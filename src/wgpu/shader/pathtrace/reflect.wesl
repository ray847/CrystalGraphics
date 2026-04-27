import package::pathtrace::{
  sample::{GeneratedRays, MAX_GENERATED_COUNT},
  radiance::Radiance,
  structural::SurfacePos,
};

fn reflect(
    dir: vec3f,
    generated_rays: GeneratedRays,
    ray_radiance: array<Radiance, MAX_GENERATED_COUNT>
) -> Radiance {
    /* Monte Carlo Estimator */
    var radiance = Radiance();
    for (var i: i32; i < generated_rays.count; i++) {
        radiance += ray_radiance[i] * generated_rays.ray_coeff[i];
    }
    radiance /= f32(generated_rays.count);

    return radiance;
}