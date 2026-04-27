import package::pathtrace::{
  math::{Ray, rand_float, makeTangentSpace, PI, RAY_OFFSET},
  structural::Vec,
  structural::SurfacePos,
  radiance::Radiance,
  lod::LOD,
};

struct Material {
    base_color: vec3f,
    emission_strength: f32,
    emission_color: vec3f,
    roughness: f32,
    metallic: f32,
    transmission: f32,
    ior: f32,
    flags: u32,
};

@group(2) @binding(5) var<storage, read> materials: array<Material>;

const MAX_GENERATED_COUNT: i32 = 4;

struct GeneratedRays {
    count: i32,
    rays: array<Ray, MAX_GENERATED_COUNT>,
    ray_coeff: array<vec3f, MAX_GENERATED_COUNT>, // coefficient for 3 channels
}

fn generateRays(
    in_dir: vec3f,
    surface_pos: SurfacePos,
    lod: LOD
) -> GeneratedRays {
    var res: GeneratedRays;
    res.count = max(1, MAX_GENERATED_COUNT - lod.depth);
    for (var i: i32 = 0; i < res.count; i++) {
        let material: Material = materials[surface_pos.material_idx];
        let sampled = sample(in_dir, surface_pos, material);
        res.rays[i] = Ray(
            surface_pos.pos + sampled.out_dir * RAY_OFFSET,
            sampled.out_dir,
        );
        let f = reflectDist(in_dir, sampled.out_dir, material);
        let projection_coeff: f32 = max(
            dot(sampled.out_dir, surface_pos.norm),
            0.0f
        );
        res.ray_coeff[i] = f * projection_coeff / sampled.prob;
    }
    return res;
}

struct Sample {
    out_dir: vec3f,
    prob: f32,
};

fn sample(in_dir: vec3f, surface_pos: SurfacePos, material: Material) -> Sample {
    let r = sqrt(rand_float());
    let angle = 2 * PI * rand_float();
    let local = vec3f(
        r * cos(angle),
        r * sin(angle),
        sqrt(max(0.0f, 1 - r * r))
    );
    return Sample(makeTangentSpace(surface_pos.norm) * local, local.z / PI);
}

fn reflectDist(in_dir: vec3f, out_dir: vec3f, material: Material) -> vec3f {
    return material.base_color / PI;
}
