import package::pathtrace::{
  math::{Ray, rand_float, PI, RAY_OFFSET},
  structural::Vec,
  structural::SurfacePos,
  radiance::Radiance,
  lod::LOD,
};

const MAX_SAMPLE_COUNT: u32 = 1;
struct Sample {
    count: u32,
    rays: array<Ray, MAX_SAMPLE_COUNT>,
    ray_radiance: array<Radiance, MAX_SAMPLE_COUNT>,
}

fn sample(in_dir: Vec, surface_pos: SurfacePos, lod: LOD) -> Sample {
    var res: Sample;
    res.count = 1;
    let rand_vec = normalize(Vec(rand_float(), rand_float(), rand_float()));
    let same_hemisphere = dot(rand_vec, surface_pos.norm) > 0.0;
    let out_dir = select(-rand_vec, rand_vec, same_hemisphere);
    res.rays[0] = Ray(
        surface_pos.pos + out_dir * RAY_OFFSET,
        out_dir,
    );
    return res;
}
