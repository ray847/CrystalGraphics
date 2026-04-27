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
    package_pathtrace_math__1seed_rng(pixel.x + pixel.y * dims.x);
    let camera_ray = camera_ray(coord);
    let color: vec3f = package_pathtrace_render_render(camera_ray);
    textureStore(target_texture, pixel, vec4f(color, 1f));
}

fn camera_ray(coord: vec2f) -> package_pathtrace_math_Ray {
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

fn package_pathtrace_math__1dir_inv(dir: vec3f) -> vec3f {
    let is_zero = dir == vec3f(0.0);
    let safe_dir = select(dir, vec3f(1e-7), is_zero);
    return 1.0 / safe_dir;
}

fn package_pathtrace_math__2ray_intersect_aabb(ray: package_pathtrace_math_Ray, lb: vec3f, ub: vec3f) -> bool {
    let inv_dir = package_pathtrace_math__1dir_inv(ray.dir);
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

fn package_pathtrace_math__1seed_rng(seed: u32) {
    package_pathtrace_math__1rng_state = seed;
    package_pathtrace_math__1step_rng();
}

fn package_pathtrace_math__1step_rng() {
    package_pathtrace_math__1rng_state = package_pathtrace_math__1rng_state * 747796405u + 2891336453u;
    var word: u32 = ((package_pathtrace_math__1rng_state >> ((package_pathtrace_math__1rng_state >> 28u) + 4u)) ^ package_pathtrace_math__1rng_state) * 277803737u;
    package_pathtrace_math__1rng_state = (word >> 22u) ^ word;
}

fn package_pathtrace_math__1rand_float() -> f32 {
    package_pathtrace_math__1step_rng();
    return f32(package_pathtrace_math__1rng_state) / 4294967296.0;
}

fn package_pathtrace_render_render(ray: package_pathtrace_math_Ray) -> package_pathtrace_luminance_Luminance {
    const SAMPLES: i32 = 4;
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
    generated_rays: package_pathtrace_sample_GeneratedRays,
    generated_ray_radiance: array<package_pathtrace_radiance_Radiance, package_pathtrace_sample__2MAX_GENERATED_COUNT>,
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
            stk[top].hit_info = package_pathtrace_bvh__1ray_hit(stk[top].ray);
            if !stk[top].hit_info.travel_info.hit {
                ret = package_pathtrace_trace__1environment_radiance(stk[top].ray.dir);
                stk[top] = package_pathtrace_trace_Stack();
                top--;
                continue;
            }
            if package_pathtrace_lod__1reach_end(stk[top].lod) || u32(top) == package_pathtrace_trace__2MAX_TRACE_DEPTH - 1 {
                ret = package_pathtrace_radiance_radiate(stk[top].hit_info.surface_pos, stk[top].lod);
                stk[top] = package_pathtrace_trace_Stack();
                top--;
                continue;
            }
            else {
                stk[top].generated_rays = package_pathtrace_sample_generateRays(stk[top].ray.dir, stk[top].hit_info.surface_pos, stk[top].lod);
                stk[top].iter = 0;
                stk[top].in_recurse = true;
            }
        }
        else {
            stk[top].generated_ray_radiance[stk[top].iter - 1] = ret;
        }
        if stk[top].iter < stk[top].generated_rays.count {
            stk[top + 1].ray = stk[top].generated_rays.rays[stk[top].iter];
            stk[top + 1].lod = package_pathtrace_lod_decr(stk[top].lod);
            stk[top].iter += 1;
            top++;
            continue;
        }
        else {
            ret = package_pathtrace_reflect_reflect(stk[top].ray.dir, stk[top].generated_rays, stk[top].generated_ray_radiance);
            stk[top] = package_pathtrace_trace_Stack();
            top--;
            continue;
        }
    }
    return ret;
}

fn package_pathtrace_trace__1environment_radiance(dir: vec3f) -> package_pathtrace_radiance_Radiance {
    let t = 0.5 * (dir.y + 1.0);
    return mix(package_pathtrace_radiance_Radiance(0.7, 0.8, 1.0), package_pathtrace_radiance_Radiance(0.05, 0.05, 0.06), 1.0 - t);
}

struct package_pathtrace_lod_LOD {
    depth: i32
}

fn package_pathtrace_lod__1reach_end(lod: package_pathtrace_lod_LOD) -> bool {
    return lod.depth > 4;
}

fn package_pathtrace_lod_decr(lod: package_pathtrace_lod_LOD) -> package_pathtrace_lod_LOD {
    let decr_lod = package_pathtrace_lod_LOD(lod.depth + 1);
    return decr_lod;
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
    material_idx: u32
}

struct package_pathtrace_structural_HitInfo {
    travel_info: package_pathtrace_structural_TravelInfo,
    surface_pos: package_pathtrace_structural_SurfacePos
}

struct package_pathtrace_sample_Material {
    base_color: vec3f,
    emission_strength: f32,
    emission_color: vec3f,
    roughness: f32,
    metallic: f32,
    transmission: f32,
    ior: f32,
    flags: u32
}

@group(2) @binding(5)
var<storage, read> package_pathtrace_sample_materials: array<package_pathtrace_sample_Material>;

const package_pathtrace_sample__2MAX_GENERATED_COUNT: i32 = 4;

struct package_pathtrace_sample_GeneratedRays {
    count: i32,
    rays: array<package_pathtrace_math_Ray, package_pathtrace_sample__2MAX_GENERATED_COUNT>,
    ray_coeff: array<vec3f, package_pathtrace_sample__2MAX_GENERATED_COUNT>
}

fn package_pathtrace_sample_generateRays(in_dir: vec3f, surface_pos: package_pathtrace_structural_SurfacePos, lod: package_pathtrace_lod_LOD) -> package_pathtrace_sample_GeneratedRays {
    var res: package_pathtrace_sample_GeneratedRays;
    res.count = max(1, package_pathtrace_sample__2MAX_GENERATED_COUNT - lod.depth);
    for (var i: i32 = 0; i < res.count; i++) {
        let material: package_pathtrace_sample_Material = package_pathtrace_sample_materials[surface_pos.material_idx];
        let sampled = package_pathtrace_sample_sample(in_dir, surface_pos, material);
        res.rays[i] = package_pathtrace_math_Ray(surface_pos.pos + sampled.out_dir * package_pathtrace_math__1RAY_OFFSET, sampled.out_dir);
        let f = package_pathtrace_sample_reflectDist(in_dir, sampled.out_dir, material);
        let projection_coeff: f32 = max(dot(sampled.out_dir, surface_pos.norm), 0f);
        res.ray_coeff[i] = f * projection_coeff / sampled.prob;
    }
    return res;
}

struct package_pathtrace_sample_Sample {
    out_dir: vec3f,
    prob: f32
}

fn package_pathtrace_sample_sample(in_dir: vec3f, surface_pos: package_pathtrace_structural_SurfacePos, material: package_pathtrace_sample_Material) -> package_pathtrace_sample_Sample {
    let r = sqrt(package_pathtrace_math__1rand_float());
    let angle = 2 * package_pathtrace_math_PI * package_pathtrace_math__1rand_float();
    let local = vec3f(r * cos(angle), r * sin(angle), sqrt(max(0f, 1 - r * r)));
    return package_pathtrace_sample_Sample(package_pathtrace_math_makeTangentSpace(surface_pos.norm) * local, local.z / package_pathtrace_math_PI);
}

fn package_pathtrace_sample_reflectDist(in_dir: vec3f, out_dir: vec3f, material: package_pathtrace_sample_Material) -> vec3f {
    return material.base_color / package_pathtrace_math_PI;
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
    normal: vec3f
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
    local_normal: vec3f
}

fn package_pathtrace_bvh__1ray_hit(ray: package_pathtrace_math_Ray) -> package_pathtrace_structural_HitInfo {
    return package_pathtrace_bvh__2ray_traverse_tlas(ray);
}

fn package_pathtrace_bvh__2ray_traverse_tlas(ray: package_pathtrace_math_Ray) -> package_pathtrace_structural_HitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = 0u;
    var dis: f32 = package_pathtrace_math_MAX;
    var local_normal: vec3f;
    var inv_trans: mat4x4f;
    var material_idx: u32;
    while (stack_top >= 0) {
        let node_idx = stack[stack_top];
        stack_top--;
        let node = package_pathtrace_bvh_tlas[node_idx];
        let lb = node.lb;
        let ub = node.ub;
        if package_pathtrace_math__2ray_intersect_aabb(ray, lb, ub) {
            if node.child != 0u {
                stack_top++;
                stack[stack_top] = node.child;
                stack_top++;
                stack[stack_top] = node.child + 1u;
            }
            else {
                let instance = package_pathtrace_bvh_instances[node.inst_idx];
                let local_ray = package_pathtrace_math_Ray((instance.inv_trans * vec4f(ray.pos, 1f)).xyz, (instance.inv_trans * vec4f(ray.dir, 0f)).xyz);
                let blas_hit_info = package_pathtrace_bvh__2ray_traverse_blas(local_ray, instance.blas_root_idx);
                if blas_hit_info.global_distance < dis {
                    dis = blas_hit_info.global_distance;
                    local_normal = blas_hit_info.local_normal;
                    inv_trans = instance.inv_trans;
                    material_idx = instance.material_idx;
                }
            }
        }
    }
    if dis >= package_pathtrace_math_MAX {
        return package_pathtrace_structural_HitInfo(package_pathtrace_structural_TravelInfo(false, package_pathtrace_math_MAX), package_pathtrace_structural_SurfacePos(vec3f(), vec3f(), 0));
    }
    return package_pathtrace_structural_HitInfo(package_pathtrace_structural_TravelInfo(true, dis), package_pathtrace_structural_SurfacePos(ray.pos + dis * ray.dir, normalize((transpose(inv_trans) * vec4f(local_normal, 0f)).xyz), material_idx));
}

fn package_pathtrace_bvh__2ray_traverse_blas(ray: package_pathtrace_math_Ray, root_idx: u32) -> package_pathtrace_bvh_BlasHitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = root_idx;
    var closest_t: f32 = package_pathtrace_math_MAX;
    var normal: vec3f;
    while (stack_top >= 0) {
        let node_idx = stack[stack_top];
        stack_top--;
        let node = package_pathtrace_bvh_blas[node_idx];
        if package_pathtrace_math__2ray_intersect_aabb(ray, node.lb, node.ub) {
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
                    let t = package_pathtrace_bvh__2ray_intersect_triangle(ray, v0, v1, v2);
                    if t > 0.0 && t < closest_t {
                        closest_t = t;
                        normal = normalize(package_pathtrace_bvh_vertices[index[0]].normal + package_pathtrace_bvh_vertices[index[1]].normal + package_pathtrace_bvh_vertices[index[2]].normal);
                    }
                }
            }
        }
    }
    return package_pathtrace_bvh_BlasHitInfo(closest_t, normal);
}

fn package_pathtrace_bvh__2ray_intersect_triangle(ray: package_pathtrace_math_Ray, v0: vec3f, v1: vec3f, v2: vec3f) -> f32 {
    let edge1 = v1 - v0;
    let edge2 = v2 - v0;
    let h = cross(ray.dir, edge2);
    let a = dot(edge1, h);
    if a > -package_pathtrace_math_EPSILON && a < package_pathtrace_math_EPSILON {
        return -1.0;
    }
    let f = 1.0 / a;
    let s = ray.pos - v0;
    let u = f * dot(s, h);
    if u < 0.0 || u > 1.0 {
        return -1.0;
    }
    let q = cross(s, edge1);
    let v = f * dot(ray.dir, q);
    if v < 0.0 || u + v > 1.0 {
        return -1.0;
    }
    let t = f * dot(edge2, q);
    if t > package_pathtrace_math_EPSILON {
        return t;
    }
    return -1.0;
}

fn package_pathtrace_reflect_reflect(dir: vec3f, generated_rays: package_pathtrace_sample_GeneratedRays, ray_radiance: array<package_pathtrace_radiance_Radiance, package_pathtrace_sample__2MAX_GENERATED_COUNT>) -> package_pathtrace_radiance_Radiance {
    var radiance = package_pathtrace_radiance_Radiance();
    for (var i: i32; i < generated_rays.count; i++) {
        radiance += ray_radiance[i] * generated_rays.ray_coeff[i];
    }
    radiance /= f32(generated_rays.count);
    return radiance;
}

