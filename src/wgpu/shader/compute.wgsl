/* Data Flow */
@group(0) @binding(0) var target_texture: texture_storage_2d<rgba8unorm, write>;
@group(1) @binding(0) var<uniform> camera: Camera;
@group(2) @binding(0) var<storage, read> tlas: array<TLASNode>;
@group(2) @binding(1) var<storage, read> instances: array<Instance>;
@group(2) @binding(2) var<storage, read> blas: array<BLASNode>;
@group(2) @binding(3) var<storage, read> indices: array<Index>;
@group(2) @binding(4) var<storage, read> vertices: array<Vertex>;

/* Type */
struct Camera {
    pos: vec3f,
    dir: vec3f,
    viewport: vec2f,
};
struct Ray {
    pos: vec3f,
    dir: vec3f,
};
struct TLASNode {
    lb: vec3f,
    child: u32,
    ub: vec3f,
    inst_idx: u32,
};
struct Instance {
    inv_trans: mat4x4f,
    blas_root_idx: u32,
    material_idx: u32,
};
struct BLASNode {
    lb: vec3f,
    child: u32,
    ub: vec3f,
    triangle_count: u32,
};
alias Index = vec3i;
struct Vertex {
    position: vec3f,
    normal: vec3f,
};

/* Constant */
const kPi: f32 = 3.1415926535;
const kEpsilon: f32 = 1e-12;

/* Main */
@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let dims = textureDimensions(target_texture);
    let pixel = global_id.xy;
    let coord: vec2f = (vec2f(pixel) / vec2f(dims)) * 2.0f - vec2f(1, 1);
    if pixel.x >= dims.x || pixel.y >= dims.y {
        return;
    }
    let camera_ray = CameraRay(coord);
    let color: vec3f = RayColor(camera_ray);
    textureStore(
        target_texture,
        pixel,
        vec4f(
            color,
            1.0f
        )
    );
}

/* Functions */
fn CameraRay(coord: vec2f) -> Ray {
    let dx: vec3f = normalize(
        cross(
            vec3f(0, 1, 0),
            //vec3f(0, 0, 1),
            camera.dir
        )
    ) * camera.viewport.x;
    let dy: vec3f = normalize(cross(dx, camera.dir)) * camera.viewport.y;
    return Ray(camera.pos, normalize(camera.dir + coord.x * dx + coord.y * dy));
}
fn RayColor(ray: Ray) -> vec3f {
    let hit_info = RayHit(ray);
    let brightness: f32 = clamp(1 - 1 / hit_info.val, 0.0, 1.0);
    if hit_info.hit {
        let depth_val = 1.0 / (1.0 + hit_info.val);
        return vec3f(depth_val, depth_val, depth_val);
    } else {
        return vec3f(0, 0, 0);
    }
}
struct RayHitInfo {
    val: f32,
    hit: bool,
};
fn RayHit(ray: Ray) -> RayHitInfo {
    return RayTraverseTLAS(ray);
}

fn get_safe_inv_dir(dir: vec3f) -> vec3f {
    // If any component is exactly 0.0, replace it with a microscopic 
    // epsilon so division results in Infinity without causing NaN.
    let is_zero = dir == vec3f(0.0);
    let safe_dir = select(dir, vec3f(1e-7), is_zero);
    return 1.0 / safe_dir;
}

fn RayIntersectAABB(ray: Ray, lb: vec3f, ub: vec3f) -> bool {
    let inv_dir = get_safe_inv_dir(ray.dir);

    let t0 = (lb - ray.pos) * inv_dir;
    let t1 = (ub - ray.pos) * inv_dir;

    let tmin = min(t0, t1);
    let tmax = max(t0, t1);

    let t_near = max(max(tmin.x, tmin.y), tmin.z);
    let t_far = min(min(tmax.x, tmax.y), tmax.z);

    return t_far >= t_near && t_far > 0.0;
}

fn RayTraverseTLAS(ray: Ray) -> RayHitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = 0u;

    var hit_info = RayHitInfo(1e6, false);

    while stack_top >= 0 {
        let node_idx = stack[stack_top];
        stack_top--;

        let node = tlas[node_idx];
        let lb = node.lb;
        let ub = node.ub;

        if RayIntersectAABB(ray, lb, ub) {
            if node.child != 0u {
                stack_top++; stack[stack_top] = node.child;
                stack_top++; stack[stack_top] = node.child + 1u;
            } else {
                let instance = instances[node.inst_idx];

                // Transform to local space. DO NOT NORMALIZE ray.dir!
                let local_ray = Ray(
                    (instance.inv_trans * vec4f(ray.pos, 1.0f)).xyz,
                    (instance.inv_trans * vec4f(ray.dir, 0.0f)).xyz,
                    // Note: Update inv_dir inside your RayIntersectAABB 
                    // dynamically, or recalculate it here for the local_ray
                );

                // Pass current closest hit to BLAS to allow early-out logic later
                let blas_t = RayTraverseBLAS(local_ray, instance.blas_root_idx, hit_info.val);

                if blas_t < hit_info.val {
                    hit_info.val = blas_t;
                    hit_info.hit = true;
                }
            }
        }
    }
    return hit_info;
}
fn RayTraverseBLAS(ray: Ray, root_idx: u32, max_t: f32) -> f32 {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = root_idx;

    var closest_t: f32 = max_t;

    while stack_top >= 0 {
        let node_idx = stack[stack_top];
        stack_top--;

        let node = blas[node_idx];

        if RayIntersectAABB(ray, node.lb, node.ub) {

            if node.triangle_count == 0u {
                // INTERIOR NODE
                stack_top++; stack[stack_top] = node.child;
                stack_top++; stack[stack_top] = node.child + 1u;
            } else {
                let index_offset = node.child;
                for (var i = 0u; i < node.triangle_count; i++) {
                    let index = indices[index_offset + i];
                    let v0 = vertices[index[0]].position;
                    let v1 = vertices[index[1]].position;
                    let v2 = vertices[index[2]].position;

                    let t = RayIntersectTriangle(ray, v0, v1, v2);

                    if t > 0.0 && t < closest_t {
                        closest_t = t;
                    }
                }
            }
        }
    }
    return closest_t;
}
fn RayIntersectTriangle(ray: Ray, v0: vec3f, v1: vec3f, v2: vec3f) -> f32 {
    let edge1 = v1 - v0;
    let edge2 = v2 - v0;
    let h = cross(ray.dir, edge2);
    let a = dot(edge1, h);

    // If a is near zero, the ray is parallel to the triangle
    if a > -kEpsilon && a < kEpsilon {
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

    if t > kEpsilon {
        return t; // Hit!
    }

    return -1.0; // Line intersects, but behind the ray origin
}