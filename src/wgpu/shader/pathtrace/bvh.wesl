import package::pathtrace::math::{Ray, ray_intersect_aabb, EPSILON, MAX};
import package::pathtrace::structural::{HitInfo, SurfacePos, TravelInfo};

/* Types */
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

/* Data Flow */
@group(2) @binding(0) var<storage, read> tlas: array<TLASNode>;
@group(2) @binding(1) var<storage, read> instances: array<Instance>;
@group(2) @binding(2) var<storage, read> blas: array<BLASNode>;
@group(2) @binding(3) var<storage, read> indices: array<Index>;
@group(2) @binding(4) var<storage, read> vertices: array<Vertex>;

struct BlasHitInfo {
    global_distance: f32,
    local_normal: vec3f,
}

fn ray_hit(ray: Ray) -> HitInfo {
    return ray_traverse_tlas(ray);
}

fn ray_traverse_tlas(ray: Ray) -> HitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = 0u;

    var dis: f32 = MAX;
    var local_normal: vec3f;
    var inv_trans: mat4x4f;
    var material_idx: u32;

    while stack_top >= 0 {
        let node_idx = stack[stack_top];
        stack_top--;

        let node = tlas[node_idx];
        let lb = node.lb;
        let ub = node.ub;

        if ray_intersect_aabb(ray, lb, ub) {
            if node.child != 0u {
                stack_top++; stack[stack_top] = node.child;
                stack_top++; stack[stack_top] = node.child + 1u;
            } else {
                let instance = instances[node.inst_idx];
                let local_ray = Ray(
                    (instance.inv_trans * vec4f(ray.pos, 1.0f)).xyz,
                    (instance.inv_trans * vec4f(ray.dir, 0.0f)).xyz,
                );
                let blas_hit_info = ray_traverse_blas(
                    local_ray,
                    instance.blas_root_idx,
                );
                if blas_hit_info.global_distance < dis {
                    dis = blas_hit_info.global_distance;
                    local_normal = blas_hit_info.local_normal;
                    inv_trans = instance.inv_trans;
                    material_idx = instance.material_idx;
                }
            }
        }
    }
    if dis >= MAX {
        return HitInfo(TravelInfo(false, MAX), SurfacePos(vec3f(), vec3f(), 0));
    }

    return HitInfo(
        TravelInfo(
            true,
            dis,
        ),
        SurfacePos(
            ray.pos + dis * ray.dir,
            normalize((transpose(inv_trans) * vec4f(local_normal, 0.0f)).xyz),
            material_idx,
        )
    );
}
fn ray_traverse_blas(ray: Ray, root_idx: u32) -> BlasHitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = root_idx;

    var closest_t: f32 = MAX;
    var normal: vec3f;

    while stack_top >= 0 {
        let node_idx = stack[stack_top];
        stack_top--;

        let node = blas[node_idx];

        if ray_intersect_aabb(ray, node.lb, node.ub) {

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

                    let t = ray_intersect_triangle(ray, v0, v1, v2);

                    if t > 0.0 && t < closest_t {
                        closest_t = t;
                        normal = normalize(vertices[index[0]].normal
                            + vertices[index[1]].normal
                            + vertices[index[2]].normal);
                    }
                }
            }
        }
    }
    return BlasHitInfo(closest_t, normal);
}

fn ray_intersect_triangle(ray: Ray, v0: vec3f, v1: vec3f, v2: vec3f) -> f32 {
    let edge1 = v1 - v0;
    let edge2 = v2 - v0;
    let h = cross(ray.dir, edge2);
    let a = dot(edge1, h);

    // If a is near zero, the ray is parallel to the triangle
    if a > -EPSILON && a < EPSILON {
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

    if t > EPSILON {
        return t; // Hit!
    }

    return -1.0; // Line intersects, but behind the ray origin
}
