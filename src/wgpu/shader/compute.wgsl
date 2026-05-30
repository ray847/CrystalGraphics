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
    package_compute_math_rng_seedRng(pixel.x + pixel.y * dims.x);
    let ray = cameraRay(coord);
    let color: vec3f = package_compute_render_render(ray);
    textureStore(target_texture, pixel, vec4f(color, 1f));
}

fn cameraRay(coord: vec2f) -> package_compute_math_ray_Ray {
    let dx: vec3f = normalize(cross(vec3f(0, 0, 1), camera.dir)) * camera.viewport.x;
    let dy: vec3f = normalize(cross(dx, camera.dir)) * camera.viewport.y;
    return package_compute_math_ray_Ray(camera.pos, normalize(camera.dir + coord.x * dx + coord.y * dy));
}

var<private> package_compute_math_rng__1rng_state: u32;

fn package_compute_math_rng_seedRng(seed: u32) {
    package_compute_math_rng__1rng_state = seed;
    package_compute_math_rng_stepRng();
}

fn package_compute_math_rng_stepRng() {
    package_compute_math_rng__1rng_state = package_compute_math_rng__1rng_state * 747796405u + 2891336453u;
    var word: u32 = ((package_compute_math_rng__1rng_state >> ((package_compute_math_rng__1rng_state >> 28u) + 4u)) ^ package_compute_math_rng__1rng_state) * 277803737u;
    package_compute_math_rng__1rng_state = (word >> 22u) ^ word;
}

fn package_compute_math_rng_randFloat() -> f32 {
    package_compute_math_rng_stepRng();
    return f32(package_compute_math_rng__1rng_state) / 4294967296.0;
}

struct package_compute_math_ray_Ray {
    pos: package_compute_math_types_Vec,
    dir: package_compute_math_types_Vec
}

alias package_compute_math_types_Pos = vec3f;

alias package_compute_math_types_Vec = vec3f;

fn package_compute_render_render(ray: package_compute_math_ray_Ray) -> package_compute_physics_luminance_Luminance {
    const SAMPLES: u32 = 4;
    var radiance = package_compute_physics_radiance_Radiance();
    for (var i: u32; i < SAMPLES; i++) {
        radiance += package_compute_trace_trace(ray);
    }
    radiance /= f32(SAMPLES);
    return package_compute_physics_luminance_illuminate(radiance);
}

alias package_compute_physics_luminance_Luminance = vec3f;

fn package_compute_physics_luminance_illuminate(radiance: package_compute_physics_radiance_Radiance) -> package_compute_physics_luminance_Luminance {
    let mapped = vec3f(1f) - exp(-max(radiance, vec3f(0f)));
    return package_compute_physics_luminance_Luminance(pow(mapped, vec3f(1f / 2.2f)));
}

alias package_compute_physics_radiance_Radiance = vec3f;

const package_compute_trace__2MAX_TRACE_DEPTH: u32 = 8;

struct package_compute_trace_Stack {
    ray: package_compute_math_ray_Ray,
    lod: package_compute_lod_LOD,
    hit_info: package_compute_structural_types_HitInfo,
    sampled_rays: package_compute_shading_sample_SampledRays,
    iter: u32,
    in_recurse: bool,
    surface_radiance: package_compute_physics_radiance_Radiance
}

fn package_compute_trace_trace(ray: package_compute_math_ray_Ray) -> package_compute_physics_radiance_Radiance {
    var ret: package_compute_physics_radiance_Radiance;
    var stk = array<package_compute_trace_Stack, package_compute_trace__2MAX_TRACE_DEPTH>();
    var top: i32 = 0;
    stk[top].ray = ray;
    stk[top].lod = package_compute_lod_LOD(0);
    while (top >= 0) {
        if !stk[top].in_recurse {
            stk[top].hit_info = package_compute_structural_bvh_rayHit(stk[top].ray);
            if !stk[top].hit_info.travel_info.hit {
                ret = package_compute_trace_environmentRadiance(stk[top].ray.dir);
                stk[top] = package_compute_trace_Stack();
                top--;
                continue;
            }
            if package_compute_lod_reachEnd(stk[top].lod) || u32(top) == package_compute_trace__2MAX_TRACE_DEPTH - 1 {
                ret = package_compute_physics_radiance_Radiance();
                stk[top] = package_compute_trace_Stack();
                top--;
                continue;
            }
            else {
                let curr_dir = -stk[top].ray.dir;
                stk[top].sampled_rays = package_compute_shading_sample_sampleRays(curr_dir, stk[top].hit_info.surface_pos, stk[top].lod);
                stk[top].iter = 0;
                stk[top].in_recurse = true;
            }
        }
        else {
            stk[top].sampled_rays.ray_radiance[stk[top].iter - 1] = ret;
        }
        if stk[top].iter < stk[top].sampled_rays.count {
            stk[top + 1].ray = stk[top].sampled_rays.rays[stk[top].iter];
            stk[top + 1].lod = package_compute_lod_decr(stk[top].lod);
            stk[top].iter += 1;
            top++;
            continue;
        }
        else {
            let curr_dir = -stk[top].ray.dir;
            ret = package_compute_shading_collect_collect(curr_dir, stk[top].sampled_rays);
            stk[top] = package_compute_trace_Stack();
            top--;
            continue;
        }
    }
    return ret;
}

fn package_compute_trace_environmentRadiance(dir: vec3f) -> package_compute_physics_radiance_Radiance {
    let t = 0.5 * (dir.y + 1.0);
    return mix(package_compute_physics_radiance_Radiance(0.7, 0.8, 1.0), package_compute_physics_radiance_Radiance(0.05, 0.05, 0.06), 1.0 - t);
}

const package_compute_lod__1MAX_DEPTH = 4;

struct package_compute_lod_LOD {
    depth: i32
}

fn package_compute_lod_reachEnd(lod: package_compute_lod_LOD) -> bool {
    return lod.depth >= package_compute_lod__1MAX_DEPTH;
}

fn package_compute_lod_decr(lod: package_compute_lod_LOD) -> package_compute_lod_LOD {
    let decr_lod = package_compute_lod_LOD(lod.depth + 1);
    return decr_lod;
}

fn package_compute_lod_heuristic(lod: package_compute_lod_LOD) -> f32 {
    return f32(package_compute_lod__1MAX_DEPTH - lod.depth) / package_compute_lod__1MAX_DEPTH;
}

struct package_compute_structural_types_TravelInfo {
    hit: bool,
    dist: f32
}

struct package_compute_structural_types_SurfacePos {
    pos: package_compute_math_types_Pos,
    shading_norm: package_compute_math_types_Vec,
    geometry_norm: package_compute_math_types_Vec,
    material_idx: u32,
    uvs: array<vec2f, 2>
}

struct package_compute_structural_types_HitInfo {
    travel_info: package_compute_structural_types_TravelInfo,
    surface_pos: package_compute_structural_types_SurfacePos
}

const package_compute_shading_sample__2MAX_SAMPLE_COUNT: u32 = 6u;

const package_compute_shading_sample__2MAX_DIFFUSE_COUNT: u32 = 4u;

const package_compute_shading_sample__2MAX_ROUGH_COUNT: u32 = 4u;

const package_compute_shading_sample__2MIN_MODEL_WEIGHT: f32 = 1e-6;

const package_compute_shading_sample__1RAY_OFFSET: f32 = 0.001;

struct package_compute_shading_sample_SampledRays {
    count: u32,
    rays: array<package_compute_math_ray_Ray, package_compute_shading_sample__2MAX_SAMPLE_COUNT>,
    ray_coeff: array<vec3f, package_compute_shading_sample__2MAX_SAMPLE_COUNT>,
    ray_radiance: array<package_compute_physics_radiance_Radiance, package_compute_shading_sample__2MAX_SAMPLE_COUNT>
}

struct package_compute_shading_sample_ModelHeuristics {
    diffuse: f32,
    specular: f32,
    rough: f32
}

struct package_compute_shading_sample_ModelSampleCounts {
    diffuse: u32,
    specular: u32,
    rough: u32
}

fn package_compute_shading_sample_sampleRays(curr_dir: vec3f, surface_pos: package_compute_structural_types_SurfacePos, lod: package_compute_lod_LOD) -> package_compute_shading_sample_SampledRays {
    let material: package_compute_shading_material_Material = package_compute_shading_material_material(surface_pos.material_idx, surface_pos.uvs);
    let heuristics = package_compute_shading_sample_modelHeuristics(material);
    let counts = package_compute_shading_sample_modelSampleCounts(u32(package_compute_lod_heuristic(lod) * f32(package_compute_shading_sample__2MAX_SAMPLE_COUNT)), heuristics);
    let sample = package_compute_shading_sample_sampleModels(curr_dir, surface_pos, material, counts);
    return sample;
}

fn package_compute_shading_sample_sampleModels(curr_dir: package_compute_math_types_Vec, surface_pos: package_compute_structural_types_SurfacePos, material: package_compute_shading_material_Material, counts: package_compute_shading_sample_ModelSampleCounts) -> package_compute_shading_sample_SampledRays {
    var new_sample = package_compute_shading_sample_SampledRays();
    for (var i: u32 = 0; i < counts.diffuse; i++) {
        let diffuse_sample = package_compute_shading_model_diffuse_diffuseSample(surface_pos, curr_dir, material);
        new_sample.rays[new_sample.count] = package_compute_shading_sample_spawnSurfaceRay(surface_pos, diffuse_sample.next_dir);
        new_sample.ray_coeff[new_sample.count] = diffuse_sample.dist / f32(counts.diffuse);
        new_sample.count += 1;
    }
    for (var i: u32 = 0; i < counts.specular; i++) {
        let specular_sample = package_compute_shading_model_specular_specularSample(surface_pos, curr_dir, material);
        new_sample.rays[new_sample.count] = package_compute_shading_sample_spawnSurfaceRay(surface_pos, specular_sample.next_dir);
        new_sample.ray_coeff[new_sample.count] = specular_sample.dist;
        new_sample.count += 1;
    }
    for (var i: u32 = 0; i < counts.rough; i++) {
        let rough_sample = package_compute_shading_model_rough_roughSample(surface_pos, curr_dir, material);
        new_sample.rays[new_sample.count] = package_compute_shading_sample_spawnSurfaceRay(surface_pos, rough_sample.next_dir);
        new_sample.ray_coeff[new_sample.count] = rough_sample.dist / f32(counts.rough);
        new_sample.count += 1;
    }
    return new_sample;
}

fn package_compute_shading_sample_modelHeuristics(material: package_compute_shading_material_Material) -> package_compute_shading_sample_ModelHeuristics {
    let diffuse_weight: f32 = max(material.diffuse_reflectance.r, max(material.diffuse_reflectance.g, material.diffuse_reflectance.b));
    let diffuse_heuristic = select(0f, f32(package_compute_shading_sample__2MAX_DIFFUSE_COUNT), diffuse_weight > package_compute_shading_sample__2MIN_MODEL_WEIGHT);
    let specular_heuristic = select(0f, 1f, material.microfacet_roughness == 0f);
    let rough_heuristic = select(0f, f32(package_compute_shading_sample__2MAX_ROUGH_COUNT), material.microfacet_roughness > 0f);
    return package_compute_shading_sample_ModelHeuristics(diffuse_heuristic, specular_heuristic, rough_heuristic);
}

fn package_compute_shading_sample_modelSampleCounts(available_count: u32, heuristics: package_compute_shading_sample_ModelHeuristics) -> package_compute_shading_sample_ModelSampleCounts {
    let diffuse_desired = package_compute_shading_sample_desiredSampleCount(heuristics.diffuse, package_compute_shading_sample__2MAX_DIFFUSE_COUNT);
    let specular_desired = package_compute_shading_sample_desiredSampleCount(heuristics.specular, 1u);
    let rough_desired = package_compute_shading_sample_desiredSampleCount(heuristics.rough, package_compute_shading_sample__2MAX_ROUGH_COUNT);
    let desired_count = diffuse_desired + specular_desired + rough_desired;
    if desired_count <= available_count {
        return package_compute_shading_sample_ModelSampleCounts(diffuse_desired, specular_desired, rough_desired);
    }
    var counts = package_compute_shading_sample_ModelSampleCounts();
    var remaining_count = available_count;
    if diffuse_desired > 0u && remaining_count > 0u {
        counts.diffuse = 1u;
        remaining_count -= 1u;
    }
    if specular_desired > 0u && remaining_count > 0u {
        counts.specular = 1u;
        remaining_count -= 1u;
    }
    if rough_desired > 0u && remaining_count > 0u {
        counts.rough = 1u;
        remaining_count -= 1u;
    }
    var diffuse_extra_cap = diffuse_desired - counts.diffuse;
    var specular_extra_cap = specular_desired - counts.specular;
    var rough_extra_cap = rough_desired - counts.rough;
    let extra_heuristic = select(0f, heuristics.diffuse, diffuse_extra_cap > 0u) + select(0f, heuristics.specular, specular_extra_cap > 0u) + select(0f, heuristics.rough, rough_extra_cap > 0u);
    if remaining_count > 0u && extra_heuristic > 0f {
        let diffuse_share = f32(remaining_count) * heuristics.diffuse / extra_heuristic;
        let specular_share = f32(remaining_count) * heuristics.specular / extra_heuristic;
        let rough_share = f32(remaining_count) * heuristics.rough / extra_heuristic;
        let diffuse_extra = min(diffuse_extra_cap, u32(diffuse_share));
        let specular_extra = min(specular_extra_cap, u32(specular_share));
        let rough_extra = min(rough_extra_cap, u32(rough_share));
        counts.diffuse += diffuse_extra;
        counts.specular += specular_extra;
        counts.rough += rough_extra;
        diffuse_extra_cap -= diffuse_extra;
        specular_extra_cap -= specular_extra;
        rough_extra_cap -= rough_extra;
        remaining_count -= diffuse_extra + specular_extra + rough_extra;
        var diffuse_remainder = select(-1f, diffuse_share - f32(diffuse_extra), diffuse_extra_cap > 0u);
        var specular_remainder = select(-1f, specular_share - f32(specular_extra), specular_extra_cap > 0u);
        var rough_remainder = select(-1f, rough_share - f32(rough_extra), rough_extra_cap > 0u);
        while (remaining_count > 0u) {
            if diffuse_remainder >= specular_remainder && diffuse_remainder >= rough_remainder && diffuse_extra_cap > 0u {
                counts.diffuse += 1u;
                diffuse_extra_cap -= 1u;
                diffuse_remainder = -1f;
            }
            else if specular_remainder >= rough_remainder && specular_extra_cap > 0u {
                counts.specular += 1u;
                specular_extra_cap -= 1u;
                specular_remainder = -1f;
            }
            else if rough_extra_cap > 0u {
                counts.rough += 1u;
                rough_extra_cap -= 1u;
                rough_remainder = -1f;
            }
            else {
                break;
            }
            remaining_count -= 1u;
        }
    }
    return counts;
}

fn package_compute_shading_sample_desiredSampleCount(heuristic: f32, max_count: u32) -> u32 {
    if heuristic <= 0f {
        return 0u;
    }
    return min(max(1u, u32(ceil(heuristic))), max_count);
}

fn package_compute_shading_sample_spawnSurfaceRay(surface_pos: package_compute_structural_types_SurfacePos, next_dir: package_compute_math_types_Vec) -> package_compute_math_ray_Ray {
    let offset_normal = select(-surface_pos.geometry_norm, surface_pos.geometry_norm, dot(next_dir, surface_pos.geometry_norm) > 0f);
    return package_compute_math_ray_Ray(surface_pos.pos + offset_normal * package_compute_shading_sample__1RAY_OFFSET, next_dir);
}

const package_compute_structural_bvh__1INTERSECTION_EPSILON: f32 = 1e-6;

struct package_compute_structural_bvh_TLASNode {
    lb: vec3f,
    child: u32,
    ub: vec3f,
    inst_idx: u32
}

struct package_compute_structural_bvh_Instance {
    inv_trans: mat4x4f,
    blas_root_idx: u32,
    material_idx: u32
}

struct package_compute_structural_bvh_BLASNode {
    lb: vec3f,
    child: u32,
    ub: vec3f,
    triangle_count: u32
}

alias package_compute_structural_bvh_Index = vec3i;

struct package_compute_structural_bvh_Vertex {
    position: vec3f,
    normal: vec3f,
    tex_coord0: vec2f,
    tex_coord1: vec2f
}

@group(2) @binding(0)
var<storage, read> package_compute_structural_bvh_tlas: array<package_compute_structural_bvh_TLASNode>;

@group(2) @binding(1)
var<storage, read> package_compute_structural_bvh_instances: array<package_compute_structural_bvh_Instance>;

@group(2) @binding(2)
var<storage, read> package_compute_structural_bvh_blas: array<package_compute_structural_bvh_BLASNode>;

@group(2) @binding(3)
var<storage, read> package_compute_structural_bvh_indices: array<package_compute_structural_bvh_Index>;

@group(2) @binding(4)
var<storage, read> package_compute_structural_bvh_vertices: array<package_compute_structural_bvh_Vertex>;

struct package_compute_structural_bvh_BlasHitInfo {
    global_distance: f32,
    local_normal: vec3f,
    local_geometry_normal: vec3f,
    local_uv0: vec2f,
    local_uv1: vec2f
}

struct package_compute_structural_bvh_TriangleHit {
    t: f32,
    u: f32,
    v: f32
}

fn package_compute_structural_bvh_rayHit(ray: package_compute_math_ray_Ray) -> package_compute_structural_types_HitInfo {
    return package_compute_structural_bvh_rayTraverseTlas(ray);
}

fn package_compute_structural_bvh_rayTraverseTlas(ray: package_compute_math_ray_Ray) -> package_compute_structural_types_HitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = 0u;
    var dis: f32 = package_compute_math_constant_MAX;
    var local_normal: vec3f;
    var local_geometry_normal: vec3f;
    var local_uv0: vec2f;
    var local_uv1: vec2f;
    var inv_trans: mat4x4f;
    var material_idx: u32;
    while (stack_top >= 0) {
        let node_idx = stack[stack_top];
        stack_top--;
        let node = package_compute_structural_bvh_tlas[node_idx];
        let lb = node.lb;
        let ub = node.ub;
        if package_compute_math_geometric_rayIntersectAABB(ray, lb, ub) {
            if node.child != 0u {
                stack_top++;
                stack[stack_top] = node.child;
                stack_top++;
                stack[stack_top] = node.child + 1u;
            }
            else {
                let instance = package_compute_structural_bvh_instances[node.inst_idx];
                let local_ray = package_compute_math_ray_Ray((instance.inv_trans * vec4f(ray.pos, 1f)).xyz, (instance.inv_trans * vec4f(ray.dir, 0f)).xyz);
                let blas_hit_info = package_compute_structural_bvh_rayTraverseBlas(local_ray, instance.blas_root_idx);
                if blas_hit_info.global_distance < dis {
                    dis = blas_hit_info.global_distance;
                    local_normal = blas_hit_info.local_normal;
                    local_geometry_normal = blas_hit_info.local_geometry_normal;
                    local_uv0 = blas_hit_info.local_uv0;
                    local_uv1 = blas_hit_info.local_uv1;
                    inv_trans = instance.inv_trans;
                    material_idx = instance.material_idx;
                }
            }
        }
    }
    if dis >= package_compute_math_constant_MAX {
        return package_compute_structural_types_HitInfo(package_compute_structural_types_TravelInfo(false, package_compute_math_constant_MAX), package_compute_structural_types_SurfacePos(vec3f(), vec3f(), vec3f(), 0, array<vec2f, 2>(vec2f(), vec2f())));
    }
    let world_geometry_normal = normalize((transpose(inv_trans) * vec4f(local_geometry_normal, 0f)).xyz);
    let world_shading_normal = normalize((transpose(inv_trans) * vec4f(local_normal, 0f)).xyz);
    return package_compute_structural_types_HitInfo(package_compute_structural_types_TravelInfo(true, dis), package_compute_structural_types_SurfacePos(ray.pos + dis * ray.dir, select(-world_shading_normal, world_shading_normal, dot(world_shading_normal, world_geometry_normal) >= 0f), world_geometry_normal, material_idx, array<vec2f, 2>(local_uv0, local_uv1)));
}

fn package_compute_structural_bvh_rayTraverseBlas(ray: package_compute_math_ray_Ray, root_idx: u32) -> package_compute_structural_bvh_BlasHitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = root_idx;
    var closest_t: f32 = package_compute_math_constant_MAX;
    var normal: vec3f = vec3f();
    var geometry_normal: vec3f = vec3f();
    var uv0: vec2f = vec2f();
    var uv1: vec2f = vec2f();
    while (stack_top >= 0) {
        let node_idx = stack[stack_top];
        stack_top--;
        let node = package_compute_structural_bvh_blas[node_idx];
        if package_compute_math_geometric_rayIntersectAABB(ray, node.lb, node.ub) {
            if node.triangle_count == 0u {
                stack_top++;
                stack[stack_top] = node.child;
                stack_top++;
                stack[stack_top] = node.child + 1u;
            }
            else {
                let index_offset = node.child;
                for (var i = 0u; i < node.triangle_count; i++) {
                    let index = package_compute_structural_bvh_indices[index_offset + i];
                    let v0 = package_compute_structural_bvh_vertices[index[0]].position;
                    let v1 = package_compute_structural_bvh_vertices[index[1]].position;
                    let v2 = package_compute_structural_bvh_vertices[index[2]].position;
                    let hit = package_compute_structural_bvh_rayIntersectTriangle(ray, v0, v1, v2);
                    if hit.t > 0.0 && hit.t < closest_t {
                        closest_t = hit.t;
                        let n0 = package_compute_structural_bvh_vertices[index[0]].normal;
                        let n1 = package_compute_structural_bvh_vertices[index[1]].normal;
                        let n2 = package_compute_structural_bvh_vertices[index[2]].normal;
                        let uv00 = package_compute_structural_bvh_vertices[index[0]].tex_coord0;
                        let uv01 = package_compute_structural_bvh_vertices[index[1]].tex_coord0;
                        let uv02 = package_compute_structural_bvh_vertices[index[2]].tex_coord0;
                        let uv10 = package_compute_structural_bvh_vertices[index[0]].tex_coord1;
                        let uv11 = package_compute_structural_bvh_vertices[index[1]].tex_coord1;
                        let uv12 = package_compute_structural_bvh_vertices[index[2]].tex_coord1;
                        let w0 = 1f - hit.u - hit.v;
                        normal = normalize(w0 * n0 + hit.u * n1 + hit.v * n2);
                        geometry_normal = normalize(cross(v1 - v0, v2 - v0));
                        uv0 = w0 * uv00 + hit.u * uv01 + hit.v * uv02;
                        uv1 = w0 * uv10 + hit.u * uv11 + hit.v * uv12;
                    }
                }
            }
        }
    }
    return package_compute_structural_bvh_BlasHitInfo(closest_t, normal, geometry_normal, uv0, uv1);
}

fn package_compute_structural_bvh_rayIntersectTriangle(ray: package_compute_math_ray_Ray, v0: vec3f, v1: vec3f, v2: vec3f) -> package_compute_structural_bvh_TriangleHit {
    let edge1 = v1 - v0;
    let edge2 = v2 - v0;
    let h = cross(ray.dir, edge2);
    let a = dot(edge1, h);
    if a > -package_compute_structural_bvh__1INTERSECTION_EPSILON && a < package_compute_structural_bvh__1INTERSECTION_EPSILON {
        return package_compute_structural_bvh_TriangleHit(-1.0, 0.0, 0.0);
    }
    let f = 1.0 / a;
    let s = ray.pos - v0;
    let u = f * dot(s, h);
    if u < 0.0 || u > 1.0 {
        return package_compute_structural_bvh_TriangleHit(-1.0, 0.0, 0.0);
    }
    let q = cross(s, edge1);
    let v = f * dot(ray.dir, q);
    if v < 0.0 || u + v > 1.0 {
        return package_compute_structural_bvh_TriangleHit(-1.0, 0.0, 0.0);
    }
    let t = f * dot(edge2, q);
    if t > package_compute_structural_bvh__1INTERSECTION_EPSILON {
        return package_compute_structural_bvh_TriangleHit(t, u, v);
    }
    return package_compute_structural_bvh_TriangleHit(-1.0, 0.0, 0.0);
}

const package_compute_math_constant_PI: f32 = 3.1415926535;

const package_compute_math_constant_MAX: f32 = 1000000.0;

const package_compute_math_geometric__2SAFE_DIRECTION_EPSILON: f32 = 1e-7;

fn package_compute_math_geometric_dirInv(dir: vec3f) -> vec3f {
    let is_zero = dir == vec3f(0.0);
    let safe_dir = select(dir, vec3f(package_compute_math_geometric__2SAFE_DIRECTION_EPSILON), is_zero);
    return 1.0 / safe_dir;
}

fn package_compute_math_geometric_rayIntersectAABB(ray: package_compute_math_ray_Ray, lb: vec3f, ub: vec3f) -> bool {
    let inv_dir = package_compute_math_geometric_dirInv(ray.dir);
    let t0 = (lb - ray.pos) * inv_dir;
    let t1 = (ub - ray.pos) * inv_dir;
    let tmin = min(t0, t1);
    let tmax = max(t0, t1);
    let t_near = max(max(tmin.x, tmin.y), tmin.z);
    let t_far = min(min(tmax.x, tmax.y), tmax.z);
    return t_far >= t_near && t_far > 0.0;
}

fn package_compute_math_geometric_makeTangentSpace(n: vec3f) -> mat3x3f {
    let helper = select(vec3f(1.0, 0.0, 0.0), vec3f(0.0, 1.0, 0.0), abs(n.x) > 0.9);
    let tangent = normalize(cross(helper, n));
    let bitangent = cross(n, tangent);
    return mat3x3f(tangent, bitangent, n);
}

fn package_compute_shading_material_material(material_idx: u32, uvs: array<vec2f, 2>) -> package_compute_shading_material_Material {
    return package_compute_shading_material_decode(package_compute_shading_material_materials[material_idx], uvs);
}

fn package_compute_shading_material_decode(packed_material: package_compute_shading_material_PackedMaterial, uvs: array<vec2f, 2>) -> package_compute_shading_material_Material {
    let base_color = package_compute_shading_material_baseColor(packed_material, uvs);
    let roughness = package_compute_shading_material_roughness(packed_material, uvs);
    let metallic = package_compute_shading_material_metallic(packed_material, uvs);
    let raw_ior = packed_material.ior;
    let ior: package_compute_shading_material_IOR = package_compute_shading_material_materialIor(raw_ior, base_color.rgb, metallic);
    return package_compute_shading_material_Material((1f - metallic) * base_color, roughness * roughness, ior);
}

struct package_compute_shading_material_IORChannel {
    re: f32,
    im: f32
}

struct package_compute_shading_material_IOR {
    r: package_compute_shading_material_IORChannel,
    g: package_compute_shading_material_IORChannel,
    b: package_compute_shading_material_IORChannel
}

struct package_compute_shading_material_Material {
    diffuse_reflectance: vec4f,
    microfacet_roughness: f32,
    ior: package_compute_shading_material_IOR
}

fn package_compute_shading_material_materialIor(real_ior: f32, base_color: vec3f, metallic: f32) -> package_compute_shading_material_IOR {
    return package_compute_shading_material_IOR(package_compute_shading_material_IORChannel(real_ior, metallic * package_compute_shading_material_imaginaryIorFromReflectance(real_ior, base_color.r, base_color)), package_compute_shading_material_IORChannel(real_ior, metallic * package_compute_shading_material_imaginaryIorFromReflectance(real_ior, base_color.g, base_color)), package_compute_shading_material_IORChannel(real_ior, metallic * package_compute_shading_material_imaginaryIorFromReflectance(real_ior, base_color.b, base_color)));
}

fn package_compute_shading_material_imaginaryIorFromReflectance(real_ior: f32, channel_color: f32, base_color: vec3f) -> f32 {
    return sqrt(clamp((4 * real_ior) / (1 - channel_color) - pow(real_ior + 1, 2), 0f, 10f));
}

struct package_compute_shading_material_MaterialTextureInfo {
    index: i32,
    tex_coord: u32,
    scale: f32,
    strength: f32
}

struct package_compute_shading_material_PackedMaterial {
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
    base_color_texture: package_compute_shading_material_MaterialTextureInfo,
    metallic_roughness_texture: package_compute_shading_material_MaterialTextureInfo,
    normal_texture: package_compute_shading_material_MaterialTextureInfo,
    occlusion_texture: package_compute_shading_material_MaterialTextureInfo,
    emissive_texture: package_compute_shading_material_MaterialTextureInfo,
    anisotropy_texture: package_compute_shading_material_MaterialTextureInfo,
    clearcoat_texture: package_compute_shading_material_MaterialTextureInfo,
    clearcoat_roughness_texture: package_compute_shading_material_MaterialTextureInfo,
    clearcoat_normal_texture: package_compute_shading_material_MaterialTextureInfo,
    iridescence_texture: package_compute_shading_material_MaterialTextureInfo,
    iridescence_thickness_texture: package_compute_shading_material_MaterialTextureInfo,
    sheen_color_texture: package_compute_shading_material_MaterialTextureInfo,
    sheen_roughness_texture: package_compute_shading_material_MaterialTextureInfo,
    specular_texture: package_compute_shading_material_MaterialTextureInfo,
    specular_color_texture: package_compute_shading_material_MaterialTextureInfo,
    transmission_texture: package_compute_shading_material_MaterialTextureInfo,
    thickness_texture: package_compute_shading_material_MaterialTextureInfo
}

@group(2) @binding(5)
var<storage, read> package_compute_shading_material_materials: array<package_compute_shading_material_PackedMaterial>;

@group(2) @binding(6)
var package_compute_shading_material__1material_textures: texture_2d_array<f32>;

@group(2) @binding(7)
var package_compute_shading_material__2material_texture_sampler: sampler;

fn package_compute_shading_material_textureUv(uvs: array<vec2f, 2>, tex_coord: u32) -> vec2f {
    if tex_coord == 1u {
        return uvs[1];
    }
    return uvs[0];
}

fn package_compute_shading_material_baseColor(material: package_compute_shading_material_PackedMaterial, uvs: array<vec2f, 2>) -> vec4f {
    var color = material.base_color_factor;
    if material.base_color_texture.index >= 0 {
        color *= textureSampleLevel(package_compute_shading_material__1material_textures, package_compute_shading_material__2material_texture_sampler, package_compute_shading_material_textureUv(uvs, material.base_color_texture.tex_coord), material.base_color_texture.index, 0f);
    }
    return color;
}

fn package_compute_shading_material_roughness(material: package_compute_shading_material_PackedMaterial, uvs: array<vec2f, 2>) -> f32 {
    var roughness = material.roughness_factor;
    if material.metallic_roughness_texture.index >= 0 {
        roughness *= textureSampleLevel(package_compute_shading_material__1material_textures, package_compute_shading_material__2material_texture_sampler, package_compute_shading_material_textureUv(uvs, material.metallic_roughness_texture.tex_coord), material.metallic_roughness_texture.index, 0f)[1];
    }
    return clamp(roughness, 0f, 1f);
}

fn package_compute_shading_material_metallic(material: package_compute_shading_material_PackedMaterial, uvs: array<vec2f, 2>) -> f32 {
    var metallic = material.metallic_factor;
    if material.metallic_roughness_texture.index >= 0 {
        metallic *= textureSampleLevel(package_compute_shading_material__1material_textures, package_compute_shading_material__2material_texture_sampler, package_compute_shading_material_textureUv(uvs, material.metallic_roughness_texture.tex_coord), material.metallic_roughness_texture.index, 0f)[2];
    }
    return clamp(metallic, 0f, 1f);
}

fn package_compute_shading_model_diffuse_diffuseSample(surface_pos: package_compute_structural_types_SurfacePos, curr_dir: package_compute_math_types_Vec, material: package_compute_shading_material_Material) -> package_compute_shading__1model_sample_ModelSample {
    return package_compute_shading__1model_sample_ModelSample(package_compute_shading_model_diffuse_diffuseDir(surface_pos), package_compute_shading_model_diffuse_diffuseDist(surface_pos, curr_dir, material));
}

fn package_compute_shading_model_diffuse_diffuseDir(surface_pos: package_compute_structural_types_SurfacePos) -> vec3f {
    let next_dir = package_compute_math_geometric_makeTangentSpace(surface_pos.shading_norm) * package_compute_shading_model_diffuse_cosineWeightedSample();
    return next_dir;
}

fn package_compute_shading_model_diffuse_cosineWeightedSample() -> package_compute_math_types_Vec {
    let r = sqrt(package_compute_math_rng_randFloat());
    let angle = 2 * package_compute_math_constant_PI * package_compute_math_rng_randFloat();
    let next_dir = package_compute_math_types_Vec(r * cos(angle), r * sin(angle), sqrt(max(0f, 1 - r * r)));
    return next_dir;
}

fn package_compute_shading_model_diffuse_diffuseDist(surface_pos: package_compute_structural_types_SurfacePos, curr_dir: package_compute_math_types_Vec, material: package_compute_shading_material_Material) -> vec3f {
    return package_compute_shading_model_diffuse_lambertianDist(material, surface_pos);
}

fn package_compute_shading_model_diffuse_lambertianDist(material: package_compute_shading_material_Material, surface_pos: package_compute_structural_types_SurfacePos) -> vec3f {
    return material.diffuse_reflectance.rgb;
}

struct package_compute_shading__1model_sample_ModelSample {
    next_dir: package_compute_math_types_Vec,
    dist: vec3f
}

fn package_compute_shading_model_specular_specularSample(surface_pos: package_compute_structural_types_SurfacePos, curr_dir: package_compute_math_types_Vec, material: package_compute_shading_material_Material) -> package_compute_shading__1model_sample_ModelSample {
    return package_compute_shading__1model_sample_ModelSample(package_compute_shading_model_specular_specularDir(surface_pos, curr_dir), package_compute_shading_model_specular_specularDist(surface_pos, curr_dir, material));
}

fn package_compute_shading_model_specular_specularDir(surface_pos: package_compute_structural_types_SurfacePos, curr_dir: package_compute_math_types_Vec) -> package_compute_math_types_Vec {
    let next_dir = reflect(-curr_dir, package_compute_shading_model_specular_orientedNormal(curr_dir, surface_pos.shading_norm));
    return next_dir;
}

fn package_compute_shading_model_specular_orientedNormal(curr_dir: vec3f, normal: vec3f) -> vec3f {
    return select(-normal, normal, dot(curr_dir, normal) >= 0f);
}

fn package_compute_shading_model_specular_specularDist(surface_pos: package_compute_structural_types_SurfacePos, curr_dir: package_compute_math_types_Vec, material: package_compute_shading_material_Material) -> vec3f {
    let fresnel_reflectance = package_compute_shading_model_util_fresnelReflectance(curr_dir, surface_pos.shading_norm, material.ior);
    return fresnel_reflectance;
}

const package_compute_shading_model_util__1FRESNEL_EPSILON: f32 = 0.000001f;

fn package_compute_shading_model_util_fresnelReflectance(curr_dir: vec3f, normal: vec3f, material_ior: package_compute_shading_material_IOR) -> vec3f {
    return vec3f(package_compute_shading_model_util_fresnelReflectanceChannel(curr_dir, normal, material_ior.r), package_compute_shading_model_util_fresnelReflectanceChannel(curr_dir, normal, material_ior.g), package_compute_shading_model_util_fresnelReflectanceChannel(curr_dir, normal, material_ior.b));
}

fn package_compute_shading_model_util_fresnelReflectanceChannel(curr_dir: vec3f, normal: vec3f, material_ior: package_compute_shading_material_IORChannel) -> f32 {
    let outside = dot(curr_dir, normal) >= 0f;
    let facing_normal = select(-normal, normal, outside);
    let incident_cosine = clamp(dot(curr_dir, facing_normal), 0f, 1f);
    if material_ior.im > package_compute_shading_model_util__1FRESNEL_EPSILON {
        return package_compute_shading_model_util_complexFresnelReflectance(incident_cosine, material_ior);
    }
    let incident_ior = select(material_ior.re, 1f, outside);
    let transmitted_ior = select(1f, material_ior.re, outside);
    let ior_ratio = incident_ior / transmitted_ior;
    let transmitted_sine_squared = ior_ratio * ior_ratio * max(0f, 1f - incident_cosine * incident_cosine);
    if transmitted_sine_squared >= 1f {
        return 1f;
    }
    let transmitted_cosine = sqrt(max(0f, 1f - transmitted_sine_squared));
    let perpendicular_reflectance = (incident_ior * incident_cosine - transmitted_ior * transmitted_cosine) / (incident_ior * incident_cosine + transmitted_ior * transmitted_cosine);
    let parallel_reflectance = (transmitted_ior * incident_cosine - incident_ior * transmitted_cosine) / (transmitted_ior * incident_cosine + incident_ior * transmitted_cosine);
    return clamp(0.5f * (perpendicular_reflectance * perpendicular_reflectance + parallel_reflectance * parallel_reflectance), 0f, 1f);
}

fn package_compute_shading_model_util_complexFresnelReflectance(cosine: f32, material_ior: package_compute_shading_material_IORChannel) -> f32 {
    let real_ior = material_ior.re;
    let imaginary_ior = material_ior.im;
    let real_sq = real_ior * real_ior;
    let imaginary_sq = imaginary_ior * imaginary_ior;
    let cosine_sq = cosine * cosine;
    let ior_norm_sq = real_sq + imaginary_sq;
    let two_real_cosine = 2f * real_ior * cosine;
    let perpendicular_reflectance = (ior_norm_sq - two_real_cosine + cosine_sq) / max(ior_norm_sq + two_real_cosine + cosine_sq, package_compute_shading_model_util__1FRESNEL_EPSILON);
    let parallel_reflectance = (ior_norm_sq * cosine_sq - two_real_cosine + 1f) / max(ior_norm_sq * cosine_sq + two_real_cosine + 1f, package_compute_shading_model_util__1FRESNEL_EPSILON);
    return clamp(0.5f * (perpendicular_reflectance + parallel_reflectance), 0f, 1f);
}

const package_compute_shading_model_rough__1VNDF_EPSILON: f32 = 0.000001f;

fn package_compute_shading_model_rough_roughSample(surface_pos: package_compute_structural_types_SurfacePos, curr_dir: package_compute_math_types_Vec, material: package_compute_shading_material_Material) -> package_compute_shading__1model_sample_ModelSample {
    let micro_norm = package_compute_shading_model_rough_sampleNorm(surface_pos, curr_dir, material);
    let next_dir = reflect(-curr_dir, micro_norm);
    let macro_norm = package_compute_shading_model_specular_orientedNormal(curr_dir, surface_pos.shading_norm);
    if dot(next_dir, macro_norm) <= 0f {
        return package_compute_shading__1model_sample_ModelSample(next_dir, vec3f(0f));
    }
    let dist = package_compute_shading_model_rough_reflectDist(curr_dir, micro_norm, macro_norm, next_dir, material) * package_compute_shading_model_rough_reflectPdfWeight(curr_dir, micro_norm, macro_norm, next_dir, material);
    return package_compute_shading__1model_sample_ModelSample(next_dir, dist);
}

fn package_compute_shading_model_rough_sampleNorm(surface_pos: package_compute_structural_types_SurfacePos, curr_dir: package_compute_math_types_Vec, material: package_compute_shading_material_Material) -> package_compute_math_types_Vec {
    let macro_norm = package_compute_shading_model_specular_orientedNormal(curr_dir, surface_pos.shading_norm);
    let tangent_space = package_compute_math_geometric_makeTangentSpace(macro_norm);
    let local_curr_dir = normalize(transpose(tangent_space) * curr_dir);
    let alpha = material.microfacet_roughness;
    let stretched_curr_dir = normalize(vec3f(alpha * local_curr_dir.x, alpha * local_curr_dir.y, max(local_curr_dir.z, package_compute_shading_model_rough__1VNDF_EPSILON)));
    let tangent_length_sq = stretched_curr_dir.x * stretched_curr_dir.x + stretched_curr_dir.y * stretched_curr_dir.y;
    let tangent_x = select(vec3f(1f, 0f, 0f), vec3f(-stretched_curr_dir.y, stretched_curr_dir.x, 0f) / sqrt(max(tangent_length_sq, package_compute_shading_model_rough__1VNDF_EPSILON)), tangent_length_sq > package_compute_shading_model_rough__1VNDF_EPSILON);
    let tangent_y = cross(stretched_curr_dir, tangent_x);
    let radius = sqrt(package_compute_math_rng_randFloat());
    let angle = 2f * package_compute_math_constant_PI * package_compute_math_rng_randFloat();
    let projected_x = radius * cos(angle);
    var projected_y = radius * sin(angle);
    let visibility_lerp = 0.5f * (1f + stretched_curr_dir.z);
    projected_y = mix(sqrt(max(0f, 1f - projected_x * projected_x)), projected_y, visibility_lerp);
    let local_stretched_micro_norm = projected_x * tangent_x + projected_y * tangent_y + sqrt(max(0f, 1f - projected_x * projected_x - projected_y * projected_y)) * stretched_curr_dir;
    let local_micro_norm = normalize(vec3f(alpha * local_stretched_micro_norm.x, alpha * local_stretched_micro_norm.y, max(local_stretched_micro_norm.z, package_compute_shading_model_rough__1VNDF_EPSILON)));
    return normalize(tangent_space * local_micro_norm);
}

fn package_compute_shading_model_rough_reflectDist(curr_dir: package_compute_math_types_Vec, micro_norm: package_compute_math_types_Vec, macro_norm: package_compute_math_types_Vec, next_dir: package_compute_math_types_Vec, material: package_compute_shading_material_Material) -> vec3f {
    let curr_macro_projection = max(dot(curr_dir, macro_norm), package_compute_shading_model_rough__1VNDF_EPSILON);
    let next_macro_projection = max(dot(next_dir, macro_norm), package_compute_shading_model_rough__1VNDF_EPSILON);
    return package_compute_shading_model_rough_ggxNormDist(micro_norm, macro_norm, material) * package_compute_shading_model_rough_ggxMaskBiDir(acos(curr_macro_projection), acos(next_macro_projection), material) * package_compute_shading_model_util_fresnelReflectance(curr_dir, micro_norm, material.ior) / (4f * curr_macro_projection * next_macro_projection);
}

fn package_compute_shading_model_rough_reflectPdfWeight(curr_dir: package_compute_math_types_Vec, micro_norm: package_compute_math_types_Vec, macro_norm: package_compute_math_types_Vec, next_dir: package_compute_math_types_Vec, material: package_compute_shading_material_Material) -> f32 {
    let curr_macro_projection = max(dot(curr_dir, macro_norm), package_compute_shading_model_rough__1VNDF_EPSILON);
    let next_macro_projection = max(dot(next_dir, macro_norm), 0f);
    let curr_micro_projection = max(dot(curr_dir, micro_norm), package_compute_shading_model_rough__1VNDF_EPSILON);
    let visible_normal_pdf = package_compute_shading_model_rough_ggxNormDist(micro_norm, macro_norm, material) * package_compute_shading_model_rough_ggxMaskUniDir(acos(curr_macro_projection), material) * curr_micro_projection / curr_macro_projection;
    let next_dir_pdf = visible_normal_pdf / (4f * curr_micro_projection);
    return next_macro_projection / max(next_dir_pdf, package_compute_shading_model_rough__1VNDF_EPSILON);
}

fn package_compute_shading_model_rough_ggxMaskUniDir(angle: f32, material: package_compute_shading_material_Material) -> f32 {
    let alpha = material.microfacet_roughness;
    return 1 / (1 + (sqrt(1 + alpha * tan(angle) * tan(angle)) - 1) / 2);
}

fn package_compute_shading_model_rough_ggxMaskBiDir(angle1: f32, angle2: f32, material: package_compute_shading_material_Material) -> f32 {
    let alpha = material.microfacet_roughness;
    return 1 / (1 + (sqrt(1 + alpha * tan(angle1) * tan(angle1)) - 1) / 2 + (sqrt(1 + alpha * tan(angle2) * tan(angle2)) - 1) / 2);
}

fn package_compute_shading_model_rough_ggxNormDist(micro_norm: package_compute_math_types_Vec, macro_norm: package_compute_math_types_Vec, material: package_compute_shading_material_Material) -> f32 {
    let alpha = material.microfacet_roughness;
    let norm_cosine = clamp(dot(micro_norm, macro_norm), 0f, 1f);
    let norm_tangent_sq = max(0f, 1f - norm_cosine * norm_cosine) / max(norm_cosine * norm_cosine, package_compute_shading_model_rough__1VNDF_EPSILON);
    let denominator = package_compute_math_constant_PI * pow(norm_cosine, 4f) * pow(alpha + norm_tangent_sq, 2f);
    return alpha / max(denominator, package_compute_shading_model_rough__1VNDF_EPSILON);
}

fn package_compute_shading_collect_collect(curr_dir: vec3f, sampled_rays: package_compute_shading_sample_SampledRays) -> package_compute_physics_radiance_Radiance {
    var radiance = package_compute_physics_radiance_Radiance();
    for (var i: u32; i < sampled_rays.count; i++) {
        radiance += sampled_rays.ray_coeff[i] * sampled_rays.ray_radiance[i];
    }
    return radiance;
}

