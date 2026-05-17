import package::pathtrace::{
  math::{Ray, randFloat, makeTangentSpace, PI, RAY_OFFSET},
  structural::{Vec, SurfacePos},
  radiance::Radiance,
  material::{Material, materials},
  lod::{LOD, heuristic},
  reflect::{diffuse::{diffuseDir, diffuseDist}},
};

const MAX_SAMPLE_COUNT: i32 = 5;
const MAX_LAMBERTIAN_COUNT: i32 = 4;
const MAX_SPECULAR_COUNT: i32 = 1;
const_assert MAX_SAMPLE_COUNT >= MAX_LAMBERTIAN_COUNT + MAX_SPECULAR_COUNT;

struct SampledRays {
    count: i32,
    rays: array<Ray, MAX_SAMPLE_COUNT>,
    ray_coeff: array<vec3f, MAX_SAMPLE_COUNT>, // coefficient for 3 channels
}

fn sampleRays(
    out_dir: vec3f,
    surface_pos: SurfacePos,
    lod: LOD
) -> SampledRays {
    var sample: SampledRays = SampledRays();
    let material: Material = materials[surface_pos.material_idx];
    sample = sampleReflection(sample, out_dir, surface_pos, lod, material);
    return sample;
}

fn sampleReflection(
    sample: SampledRays,
    out_dir: vec3f,
    surface_pos: SurfacePos,
    lod: LOD,
    material: Material
) -> SampledRays {
    var new_sample = sample;
    let diffuse_count: i32 = i32(4 * heuristic(lod));
    for (var i: i32 = new_sample.count; i < diffuse_count; i++) {
        let dir = diffuseDir(surface_pos, out_dir, material);
        new_sample.rays[i] = Ray(
            surface_pos.pos + dir * RAY_OFFSET,
            dir
        );
        new_sample.ray_coeff[i] = diffuseDist(
            surface_pos, new_sample.rays[i].dir, material
        ) / f32(diffuse_count);
    }
    new_sample.count += diffuse_count;
    return new_sample;
}