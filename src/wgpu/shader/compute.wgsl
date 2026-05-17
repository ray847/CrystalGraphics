struct Camera {
    pos: vec3f,
    dir: vec3f,
    viewport: vec2f
}

@group(0) @binding(0)
var target_texture: texture_storage_2d<rgba8unorm, write>;

@group(1) @binding(0)
var<uniform> camera: Camera;

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let dims = textureDimensions(target_texture);
    let pixel = global_id.xy;
    let coord: vec2f = (vec2f(pixel) / vec2f(dims)) * 2f - vec2f(1, 1);
    if pixel.x >= dims.x || pixel.y >= dims.y {
        return;
    }
    package_pathtrace_math_seedRng(pixel.x + pixel.y * dims.x);
    let ray = cameraRay(coord);
    let color: vec3f = package_pathtrace_render_render(ray);
    textureStore(target_texture, pixel, vec4f(color, 1f));
}

fn cameraRay(coord: vec2f) -> package_pathtrace_math_Ray {
    let dx: vec3f = normalize(cross(vec3f(0, 0, 1), camera.dir)) * camera.viewport.x;
    let dy: vec3f = normalize(cross(dx, camera.dir)) * camera.viewport.y;
    return package_pathtrace_math_Ray(camera.pos, normalize(camera.dir + coord.x * dx + coord.y * dy));
}

struct package_pathtrace_math_Ray {
    pos: vec3f,
    dir: vec3f
}

const package_pathtrace_math_PI: f32 = 3.1415926535;

const package_pathtrace_math_EPSILON: f32 = 1e-12;

const package_pathtrace_math__1RAY_OFFSET: f32 = 0.0001;

const package_pathtrace_math_MAX: f32 = 1000000.0;

fn package_pathtrace_math_dirInv(dir: vec3f) -> vec3f {
    let is_zero = dir == vec3f(0.0);
    let safe_dir = select(dir, vec3f(1e-7), is_zero);
    return 1.0 / safe_dir;
}

fn package_pathtrace_math_rayIntersectAabb(ray: package_pathtrace_math_Ray, lb: vec3f, ub: vec3f) -> bool {
    let inv_dir = package_pathtrace_math_dirInv(ray.dir);
    let t0 = (lb - ray.pos) * inv_dir;
    let t1 = (ub - ray.pos) * inv_dir;
    let tmin = min(t0, t1);
    let tmax = max(t0, t1);
    let t_near = max(max(tmin.x, tmin.y), tmin.z);
    let t_far = min(min(tmax.x, tmax.y), tmax.z);
    return t_far >= t_near && t_far > 0.0;
}

fn package_pathtrace_math_makeTangentSpace(n: vec3f) -> mat3x3f {
    let helper = select(vec3f(1.0, 0.0, 0.0), vec3f(0.0, 1.0, 0.0), abs(n.x) > 0.9);
    let tangent = normalize(cross(helper, n));
    let bitangent = cross(n, tangent);
    return mat3x3f(tangent, bitangent, n);
}

var<private> package_pathtrace_math__1rng_state: u32;

fn package_pathtrace_math_seedRng(seed: u32) {
    package_pathtrace_math__1rng_state = seed;
    package_pathtrace_math_stepRng();
}

fn package_pathtrace_math_stepRng() {
    package_pathtrace_math__1rng_state = package_pathtrace_math__1rng_state * 747796405u + 2891336453u;
    var word: u32 = ((package_pathtrace_math__1rng_state >> ((package_pathtrace_math__1rng_state >> 28u) + 4u)) ^ package_pathtrace_math__1rng_state) * 277803737u;
    package_pathtrace_math__1rng_state = (word >> 22u) ^ word;
}

fn package_pathtrace_math_randFloat() -> f32 {
    package_pathtrace_math_stepRng();
    return f32(package_pathtrace_math__1rng_state) / 4294967296.0;
}

fn package_pathtrace_render_render(ray: package_pathtrace_math_Ray) -> package_pathtrace_luminance_Luminance {
    const SAMPLES: i32 = 1;
    var radiance = package_pathtrace_radiance_Radiance();
    for (var i: i32; i < SAMPLES; i++) {
        radiance += package_pathtrace_trace_trace(ray);
    }
    radiance /= f32(SAMPLES);
    return package_pathtrace_luminance_illuminate(radiance);
}

alias package_pathtrace_luminance_Luminance = vec3f;

fn package_pathtrace_luminance_illuminate(radiance: package_pathtrace_radiance_Radiance) -> package_pathtrace_luminance_Luminance {
    return radiance;
}

alias package_pathtrace_radiance_Radiance = vec3f;

fn package_pathtrace_radiance_radiate(surface_pos: package_pathtrace_structural_SurfacePos, lod: package_pathtrace_lod_LOD) -> package_pathtrace_radiance_Radiance {
    return package_pathtrace_radiance_Radiance();
}

const package_pathtrace_trace__2MAX_TRACE_DEPTH: u32 = 8;

struct package_pathtrace_trace_Stack {
    ray: package_pathtrace_math_Ray,
    lod: package_pathtrace_lod_LOD,
    hit_info: package_pathtrace_structural_HitInfo,
    sampled_rays: package_pathtrace_sample_SampledRays,
    sampled_ray_radiance: array<package_pathtrace_radiance_Radiance, package_pathtrace_sample__2MAX_SAMPLE_COUNT>,
    iter: i32,
    in_recurse: bool,
    surface_radiance: package_pathtrace_radiance_Radiance
}

fn package_pathtrace_trace_trace(ray: package_pathtrace_math_Ray) -> package_pathtrace_radiance_Radiance {
    var ret: package_pathtrace_radiance_Radiance;
    var stk = array<package_pathtrace_trace_Stack, package_pathtrace_trace__2MAX_TRACE_DEPTH>();
    var top: i32 = 0;
    stk[top].ray = ray;
    stk[top].lod = package_pathtrace_lod_LOD(0);
    while (top >= 0) {
        if !stk[top].in_recurse {
            stk[top].hit_info = package_pathtrace_bvh_rayHit(stk[top].ray);
            if !stk[top].hit_info.travel_info.hit {
                ret = package_pathtrace_trace_environmentRadiance(stk[top].ray.dir);
                stk[top] = package_pathtrace_trace_Stack();
                top--;
                continue;
            }
            if package_pathtrace_lod_reachEnd(stk[top].lod) || u32(top) == package_pathtrace_trace__2MAX_TRACE_DEPTH - 1 {
                ret = package_pathtrace_radiance_radiate(stk[top].hit_info.surface_pos, stk[top].lod);
                stk[top] = package_pathtrace_trace_Stack();
                top--;
                continue;
            }
            else {
                stk[top].sampled_rays = package_pathtrace_sample_sampleRays(stk[top].ray.dir, stk[top].hit_info.surface_pos, stk[top].lod);
                stk[top].iter = 0;
                stk[top].in_recurse = true;
            }
        }
        else {
            stk[top].sampled_ray_radiance[stk[top].iter - 1] = ret;
        }
        if stk[top].iter < stk[top].sampled_rays.count {
            stk[top + 1].ray = stk[top].sampled_rays.rays[stk[top].iter];
            stk[top + 1].lod = package_pathtrace_lod_decr(stk[top].lod);
            stk[top].iter += 1;
            top++;
            continue;
        }
        else {
            ret = package_pathtrace_collect_collect(stk[top].ray.dir, stk[top].sampled_rays, stk[top].sampled_ray_radiance);
            stk[top] = package_pathtrace_trace_Stack();
            top--;
            continue;
        }
    }
    return ret;
}

fn package_pathtrace_trace_environmentRadiance(dir: vec3f) -> package_pathtrace_radiance_Radiance {
    let t = 0.5 * (dir.y + 1.0);
    return mix(package_pathtrace_radiance_Radiance(0.7, 0.8, 1.0), package_pathtrace_radiance_Radiance(0.05, 0.05, 0.06), 1.0 - t);
}

struct package_pathtrace_lod_LOD {
    depth: i32
}

fn package_pathtrace_lod_reachEnd(lod: package_pathtrace_lod_LOD) -> bool {
    return lod.depth >= 4;
}

fn package_pathtrace_lod_decr(lod: package_pathtrace_lod_LOD) -> package_pathtrace_lod_LOD {
    let decr_lod = package_pathtrace_lod_LOD(lod.depth + 1);
    return decr_lod;
}

fn package_pathtrace_lod_heuristic(lod: package_pathtrace_lod_LOD) -> f32 {
    return f32(4 - lod.depth) / 4.0;
}

alias package_pathtrace_structural_Pos = vec3f;

alias package_pathtrace_structural_Vec = vec3f;

struct package_pathtrace_structural_TravelInfo {
    hit: bool,
    dist: f32
}

struct package_pathtrace_structural_SurfacePos {
    pos: package_pathtrace_structural_Pos,
    norm: package_pathtrace_structural_Vec,
    material_idx: u32,
    uv: vec2f
}

struct package_pathtrace_structural_HitInfo {
    travel_info: package_pathtrace_structural_TravelInfo,
    surface_pos: package_pathtrace_structural_SurfacePos
}

const package_pathtrace_sample__2MAX_SAMPLE_COUNT: i32 = 5;

const package_pathtrace_sample__2MAX_LAMBERTIAN_COUNT: i32 = 4;

const package_pathtrace_sample__2MAX_SPECULAR_COUNT: i32 = 1;

const_assert package_pathtrace_sample__2MAX_SAMPLE_COUNT >= package_pathtrace_sample__2MAX_LAMBERTIAN_COUNT + package_pathtrace_sample__2MAX_SPECULAR_COUNT;

struct package_pathtrace_sample_SampledRays {
    count: i32,
    rays: array<package_pathtrace_math_Ray, package_pathtrace_sample__2MAX_SAMPLE_COUNT>,
    ray_coeff: array<vec3f, package_pathtrace_sample__2MAX_SAMPLE_COUNT>
}

fn package_pathtrace_sample_sampleRays(out_dir: vec3f, surface_pos: package_pathtrace_structural_SurfacePos, lod: package_pathtrace_lod_LOD) -> package_pathtrace_sample_SampledRays {
    var sample: package_pathtrace_sample_SampledRays = package_pathtrace_sample_SampledRays();
    let material: package_pathtrace_material_Material = package_pathtrace_material_materials[surface_pos.material_idx];
    sample = package_pathtrace_sample_sampleReflection(sample, out_dir, surface_pos, lod, material);
    return sample;
}

fn package_pathtrace_sample_sampleReflection(sample: package_pathtrace_sample_SampledRays, out_dir: vec3f, surface_pos: package_pathtrace_structural_SurfacePos, lod: package_pathtrace_lod_LOD, material: package_pathtrace_material_Material) -> package_pathtrace_sample_SampledRays {
    var new_sample = sample;
    let diffuse_count: i32 = i32(4 * package_pathtrace_lod_heuristic(lod));
    for (var i: i32 = new_sample.count; i < diffuse_count; i++) {
        let dir = package_pathtrace_reflect_diffuse_diffuseDir(surface_pos, out_dir, material);
        new_sample.rays[i] = package_pathtrace_math_Ray(surface_pos.pos + dir * package_pathtrace_math__1RAY_OFFSET, dir);
        new_sample.ray_coeff[i] = package_pathtrace_reflect_diffuse_diffuseDist(surface_pos, new_sample.rays[i].dir, material) / f32(diffuse_count);
    }
    new_sample.count += diffuse_count;
    return new_sample;
}

struct package_pathtrace_bvh_TLASNode {
    lb: vec3f,
    child: u32,
    ub: vec3f,
    inst_idx: u32
}

struct package_pathtrace_bvh_Instance {
    inv_trans: mat4x4f,
    blas_root_idx: u32,
    material_idx: u32
}

struct package_pathtrace_bvh_BLASNode {
    lb: vec3f,
    child: u32,
    ub: vec3f,
    triangle_count: u32
}

alias package_pathtrace_bvh_Index = vec3i;

struct package_pathtrace_bvh_Vertex {
    position: vec3f,
    normal: vec3f,
    tex_coord: vec2f
}

@group(2) @binding(0)
var<storage, read> package_pathtrace_bvh_tlas: array<package_pathtrace_bvh_TLASNode>;

@group(2) @binding(1)
var<storage, read> package_pathtrace_bvh_instances: array<package_pathtrace_bvh_Instance>;

@group(2) @binding(2)
var<storage, read> package_pathtrace_bvh_blas: array<package_pathtrace_bvh_BLASNode>;

@group(2) @binding(3)
var<storage, read> package_pathtrace_bvh_indices: array<package_pathtrace_bvh_Index>;

@group(2) @binding(4)
var<storage, read> package_pathtrace_bvh_vertices: array<package_pathtrace_bvh_Vertex>;

struct package_pathtrace_bvh_BlasHitInfo {
    global_distance: f32,
    local_normal: vec3f,
    local_uv: vec2f
}

struct package_pathtrace_bvh_TriangleHit {
    t: f32,
    u: f32,
    v: f32
}

fn package_pathtrace_bvh_rayHit(ray: package_pathtrace_math_Ray) -> package_pathtrace_structural_HitInfo {
    return package_pathtrace_bvh_rayTraverseTlas(ray);
}

fn package_pathtrace_bvh_rayTraverseTlas(ray: package_pathtrace_math_Ray) -> package_pathtrace_structural_HitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = 0u;
    var dis: f32 = package_pathtrace_math_MAX;
    var local_normal: vec3f;
    var local_uv: vec2f;
    var inv_trans: mat4x4f;
    var material_idx: u32;
    while (stack_top >= 0) {
        let node_idx = stack[stack_top];
        stack_top--;
        let node = package_pathtrace_bvh_tlas[node_idx];
        let lb = node.lb;
        let ub = node.ub;
        if package_pathtrace_math_rayIntersectAabb(ray, lb, ub) {
            if node.child != 0u {
                stack_top++;
                stack[stack_top] = node.child;
                stack_top++;
                stack[stack_top] = node.child + 1u;
            }
            else {
                let instance = package_pathtrace_bvh_instances[node.inst_idx];
                let local_ray = package_pathtrace_math_Ray((instance.inv_trans * vec4f(ray.pos, 1f)).xyz, (instance.inv_trans * vec4f(ray.dir, 0f)).xyz);
                let blas_hit_info = package_pathtrace_bvh_rayTraverseBlas(local_ray, instance.blas_root_idx);
                if blas_hit_info.global_distance < dis {
                    dis = blas_hit_info.global_distance;
                    local_normal = blas_hit_info.local_normal;
                    local_uv = blas_hit_info.local_uv;
                    inv_trans = instance.inv_trans;
                    material_idx = instance.material_idx;
                }
            }
        }
    }
    if dis >= package_pathtrace_math_MAX {
        return package_pathtrace_structural_HitInfo(package_pathtrace_structural_TravelInfo(false, package_pathtrace_math_MAX), package_pathtrace_structural_SurfacePos(vec3f(), vec3f(), 0, vec2f()));
    }
    return package_pathtrace_structural_HitInfo(package_pathtrace_structural_TravelInfo(true, dis), package_pathtrace_structural_SurfacePos(ray.pos + dis * ray.dir, normalize((transpose(inv_trans) * vec4f(local_normal, 0f)).xyz), material_idx, local_uv));
}

fn package_pathtrace_bvh_rayTraverseBlas(ray: package_pathtrace_math_Ray, root_idx: u32) -> package_pathtrace_bvh_BlasHitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = root_idx;
    var closest_t: f32 = package_pathtrace_math_MAX;
    var normal: vec3f = vec3f();
    var uv: vec2f = vec2f();
    while (stack_top >= 0) {
        let node_idx = stack[stack_top];
        stack_top--;
        let node = package_pathtrace_bvh_blas[node_idx];
        if package_pathtrace_math_rayIntersectAabb(ray, node.lb, node.ub) {
            if node.triangle_count == 0u {
                stack_top++;
                stack[stack_top] = node.child;
                stack_top++;
                stack[stack_top] = node.child + 1u;
            }
            else {
                let index_offset = node.child;
                for (var i = 0u; i < node.triangle_count; i++) {
                    let index = package_pathtrace_bvh_indices[index_offset + i];
                    let v0 = package_pathtrace_bvh_vertices[index[0]].position;
                    let v1 = package_pathtrace_bvh_vertices[index[1]].position;
                    let v2 = package_pathtrace_bvh_vertices[index[2]].position;
                    let hit = package_pathtrace_bvh_rayIntersectTriangle(ray, v0, v1, v2);
                    if hit.t > 0.0 && hit.t < closest_t {
                        closest_t = hit.t;
                        let n0 = package_pathtrace_bvh_vertices[index[0]].normal;
                        let n1 = package_pathtrace_bvh_vertices[index[1]].normal;
                        let n2 = package_pathtrace_bvh_vertices[index[2]].normal;
                        let uv0 = package_pathtrace_bvh_vertices[index[0]].tex_coord;
                        let uv1 = package_pathtrace_bvh_vertices[index[1]].tex_coord;
                        let uv2 = package_pathtrace_bvh_vertices[index[2]].tex_coord;
                        let w0 = 1f - hit.u - hit.v;
                        normal = normalize(w0 * n0 + hit.u * n1 + hit.v * n2);
                        uv = w0 * uv0 + hit.u * uv1 + hit.v * uv2;
                    }
                }
            }
        }
    }
    return package_pathtrace_bvh_BlasHitInfo(closest_t, normal, uv);
}

fn package_pathtrace_bvh_rayIntersectTriangle(ray: package_pathtrace_math_Ray, v0: vec3f, v1: vec3f, v2: vec3f) -> package_pathtrace_bvh_TriangleHit {
    let edge1 = v1 - v0;
    let edge2 = v2 - v0;
    let h = cross(ray.dir, edge2);
    let a = dot(edge1, h);
    if a > -package_pathtrace_math_EPSILON && a < package_pathtrace_math_EPSILON {
        return package_pathtrace_bvh_TriangleHit(-1.0, 0.0, 0.0);
    }
    let f = 1.0 / a;
    let s = ray.pos - v0;
    let u = f * dot(s, h);
    if u < 0.0 || u > 1.0 {
        return package_pathtrace_bvh_TriangleHit(-1.0, 0.0, 0.0);
    }
    let q = cross(s, edge1);
    let v = f * dot(ray.dir, q);
    if v < 0.0 || u + v > 1.0 {
        return package_pathtrace_bvh_TriangleHit(-1.0, 0.0, 0.0);
    }
    let t = f * dot(edge2, q);
    if t > package_pathtrace_math_EPSILON {
        return package_pathtrace_bvh_TriangleHit(t, u, v);
    }
    return package_pathtrace_bvh_TriangleHit(-1.0, 0.0, 0.0);
}

struct package_pathtrace_material_MaterialTextureInfo {
    index: i32,
    tex_coord: u32,
    scale: f32,
    strength: f32
}

fn package_pathtrace_material_baseColor(material: package_pathtrace_material_Material, uv: vec2f) -> vec4f {
    var color = material.base_color_factor;
    if material.base_color_texture.index >= 0 {
        color *= textureSampleLevel(package_pathtrace_material__1material_textures, package_pathtrace_material__2material_texture_sampler, uv, material.base_color_texture.index, 0f);
    }
    return color;
}

struct package_pathtrace_material_Material {
    base_color_factor: vec4f,
    emissive_factor: vec3f,
    emissive_strength: f32,
    metallic_factor: f32,
    roughness_factor: f32,
    alpha_cutoff: f32,
    alpha_mode: u32,
    ior: f32,
    dispersion: f32,
    transmission_factor: f32,
    thickness_factor: f32,
    attenuation_color: vec3f,
    attenuation_distance: f32,
    anisotropy_strength: f32,
    anisotropy_rotation: f32,
    clearcoat_factor: f32,
    clearcoat_roughness_factor: f32,
    iridescence_factor: f32,
    iridescence_ior: f32,
    iridescence_thickness_minimum: f32,
    iridescence_thickness_maximum: f32,
    sheen_color_factor: vec3f,
    sheen_roughness_factor: f32,
    specular_color_factor: vec3f,
    specular_factor: f32,
    flags: u32,
    _padding0: u32,
    _padding1: u32,
    _padding2: u32,
    base_color_texture: package_pathtrace_material_MaterialTextureInfo,
    metallic_roughness_texture: package_pathtrace_material_MaterialTextureInfo,
    normal_texture: package_pathtrace_material_MaterialTextureInfo,
    occlusion_texture: package_pathtrace_material_MaterialTextureInfo,
    emissive_texture: package_pathtrace_material_MaterialTextureInfo,
    anisotropy_texture: package_pathtrace_material_MaterialTextureInfo,
    clearcoat_texture: package_pathtrace_material_MaterialTextureInfo,
    clearcoat_roughness_texture: package_pathtrace_material_MaterialTextureInfo,
    clearcoat_normal_texture: package_pathtrace_material_MaterialTextureInfo,
    iridescence_texture: package_pathtrace_material_MaterialTextureInfo,
    iridescence_thickness_texture: package_pathtrace_material_MaterialTextureInfo,
    sheen_color_texture: package_pathtrace_material_MaterialTextureInfo,
    sheen_roughness_texture: package_pathtrace_material_MaterialTextureInfo,
    specular_texture: package_pathtrace_material_MaterialTextureInfo,
    specular_color_texture: package_pathtrace_material_MaterialTextureInfo,
    transmission_texture: package_pathtrace_material_MaterialTextureInfo,
    thickness_texture: package_pathtrace_material_MaterialTextureInfo
}

@group(2) @binding(5)
var<storage, read> package_pathtrace_material_materials: array<package_pathtrace_material_Material>;

@group(2) @binding(6)
var package_pathtrace_material__1material_textures: texture_2d_array<f32>;

@group(2) @binding(7)
var package_pathtrace_material__2material_texture_sampler: sampler;

fn package_pathtrace_reflect_diffuse_diffuseDir(surface_pos: package_pathtrace_structural_SurfacePos, out_dir: vec3f, material: package_pathtrace_material_Material) -> vec3f {
    let in_dir = package_pathtrace_math_makeTangentSpace(surface_pos.norm) * package_pathtrace_reflect_diffuse_cosineWeightedSample();
    return in_dir;
}

fn package_pathtrace_reflect_diffuse_cosineWeightedSample() -> vec3f {
    let r = sqrt(package_pathtrace_math_randFloat());
    let angle = 2 * package_pathtrace_math_PI * package_pathtrace_math_randFloat();
    let dir = vec3f(r * cos(angle), r * sin(angle), sqrt(max(0f, 1 - r * r)));
    return dir;
}

fn package_pathtrace_reflect_diffuse_diffuseDist(surface_pos: package_pathtrace_structural_SurfacePos, in_dir: vec3f, material: package_pathtrace_material_Material) -> vec3f {
    let projection_factor = dot(in_dir, surface_pos.norm);
    return package_pathtrace_reflect_diffuse_lambertianDist(material, surface_pos) * projection_factor;
}

fn package_pathtrace_reflect_diffuse_lambertianDist(material: package_pathtrace_material_Material, surface_pos: package_pathtrace_structural_SurfacePos) -> vec3f {
    return package_pathtrace_material_baseColor(material, surface_pos.uv).rgb;
}

fn package_pathtrace_collect_collect(dir: vec3f, sampled_rays: package_pathtrace_sample_SampledRays, ray_radiance: array<package_pathtrace_radiance_Radiance, package_pathtrace_sample__2MAX_SAMPLE_COUNT>) -> package_pathtrace_radiance_Radiance {
    var radiance = package_pathtrace_radiance_Radiance();
    for (var i: i32; i < sampled_rays.count; i++) {
        radiance += ray_radiance[i] * sampled_rays.ray_coeff[i];
    }
    return radiance;
}

