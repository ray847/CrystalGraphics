/* Data Flow */
@group(0) @binding(0) var target_texture: texture_storage_2d<rgba8unorm, write>;
@group(1) @binding(0) var<uniform> camera: Camera;
@group(2) @binding(0) var<storage, read> tlas: array<TLASNode>;
@group(2) @binding(1) var<storage, read> instances: array<Instance>;
@group(2) @binding(2) var<storage, read> blas: array<BLASNode>;

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

/* Constant */
const kPi: f32 = 3.1415926535;
const kEpsilon: f32 = 1e-6;

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
    let brightness: f32 = clamp(hit_info.val / 500.0, 0.0, 1.0);
    if hit_info.hit_blas {
        return vec3f(brightness, 0, 0);
    } else {
        return vec3f(0, brightness, 0);
    }
}
struct RayHitInfo {
    val: f32,
    hit_blas: bool,
};
fn RayHit(ray: Ray) -> RayHitInfo {
    return RayTraverseTLAS(ray);
}
fn get_safe_inv_dir(dir: vec3f) -> vec3f {
    var inv_dir = 1.0 / dir;
    // Prevent NaN/Inf poisoning by capping divisions by zero
    if abs(dir.x) < 1e-6 { inv_dir.x = 1e30 * sign(inv_dir.x); }
    if abs(dir.y) < 1e-6 { inv_dir.y = 1e30 * sign(inv_dir.y); }
    if abs(dir.z) < 1e-6 { inv_dir.z = 1e30 * sign(inv_dir.z); }
    return inv_dir;
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
    var curr: u32 = 0u;
    var nodes_visited: f32 = 0.0;
    var hit_blas: bool = false;

    while stack_top >= 0 {
        let node_idx = stack[stack_top];
        stack_top--;

        let node = tlas[node_idx];

        if RayIntersectAABB(ray, node.lb, node.ub) {
            nodes_visited += 1.0;

            if node.child != 0 {
                // INTERIOR TLAS Node
                let lchild = node.child;
                let rchild = lchild + 1u;

                stack_top++;
                stack[stack_top] = lchild;
                stack_top++;
                stack[stack_top] = rchild;
            } else {
                hit_blas = true;
                //return RayHitInfo(nodes_visited, hit_blas);
                // LEAF TLAS Node!
                // Instantly dive into the BLAS and add its heatmap score to the total.
                let instance = instances[node.inst_idx];
                let local_ray = Ray(
                    (instance.inv_trans * vec4f(ray.pos, 1.0f)).xyz,
                    (instance.inv_trans * vec4f(ray.dir, 0.0f)).xyz,
                );
                nodes_visited += RayTraverseBLAS(local_ray, instance.blas_root_idx);
            }
        }
    }
    return RayHitInfo(nodes_visited, hit_blas);
}
fn RayTraverseBLAS(ray: Ray, root_idx: u32) -> f32 {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;

    // Start at the specific BLAS root given to us by the TLAS
    stack[0] = root_idx;
    var nodes_visited: f32 = 0.0;

    while stack_top >= 0 {
        let node_idx = stack[stack_top];
        stack_top--;

        let node = blas[node_idx];

        if RayIntersectAABB(ray, node.lb, node.ub) {
            nodes_visited += 1.0;

            if node.triangle_count == 0u {
                // It's an INTERIOR BLAS Node
                let lchild = node.child;
                let rchild = lchild + 1u;

                stack_top++;
                stack[stack_top] = lchild;
                stack_top++;
                stack[stack_top] = rchild;
            } else {
                // It's a LEAF BLAS Node! 
                // We've hit the actual geometry bounding box.
                // For the heatmap, let's add the triangle count so denser geometry glows brighter!
                nodes_visited += f32(node.triangle_count);
            }
        }
    }
    return nodes_visited;
}