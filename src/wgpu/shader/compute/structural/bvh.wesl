import package::compute::math;
import super::types::{HitInfo, SurfacePos, TravelInfo};

const INTERSECTION_EPSILON: f32 = 1e-6;

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
    tex_coord0: vec2f,
    tex_coord1: vec2f,
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
    local_geometry_normal: vec3f,
    local_uv0: vec2f,
    local_uv1: vec2f,
}
struct TriangleHit {
    t: f32,
    u: f32,
    v: f32,
}

fn rayHit(ray: math::Ray) -> HitInfo {
    return rayTraverseTlas(ray);
}

fn rayTraverseTlas(ray: math::Ray) -> HitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = 0u;

    var dis: f32 = math::MAX;
    var local_normal: vec3f;
    var local_geometry_normal: vec3f;
    var local_uv0: vec2f;
    var local_uv1: vec2f;
    var inv_trans: mat4x4f;
    var material_idx: u32;

    while stack_top >= 0 {
        let node_idx = stack[stack_top];
        stack_top--;

        let node = tlas[node_idx];
        let lb = node.lb;
        let ub = node.ub;

        if math::rayIntersectAABB(ray, lb, ub) {
            if node.child != 0u {
                stack_top++; stack[stack_top] = node.child;
                stack_top++; stack[stack_top] = node.child + 1u;
            } else {
                let instance = instances[node.inst_idx];
                let local_ray = math::Ray(
                    (instance.inv_trans * vec4f(ray.pos, 1.0f)).xyz,
                    (instance.inv_trans * vec4f(ray.dir, 0.0f)).xyz,
                );
                let blas_hit_info = rayTraverseBlas(
                    local_ray,
                    instance.blas_root_idx,
                );
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
    if dis >= math::MAX {
        return HitInfo(
            TravelInfo(false, math::MAX),
            SurfacePos(vec3f(), vec3f(), vec3f(), 0, array<vec2f, 2>(vec2f(), vec2f()))
        );
    }

    let world_geometry_normal = normalize((transpose(inv_trans) * vec4f(local_geometry_normal, 0.0f)).xyz);
    let world_shading_normal = normalize((transpose(inv_trans) * vec4f(local_normal, 0.0f)).xyz);

    return HitInfo(
        TravelInfo(
            true,
            dis,
        ),
        SurfacePos(
            ray.pos + dis * ray.dir,
            select(
                -world_shading_normal,
                world_shading_normal,
                dot(world_shading_normal, world_geometry_normal) >= 0.0f,
            ),
            world_geometry_normal,
            material_idx,
            array<vec2f, 2>(local_uv0, local_uv1),
        )
    );
}
fn rayTraverseBlas(ray: math::Ray, root_idx: u32) -> BlasHitInfo {
    var stack: array<u32, 32>;
    var stack_top: i32 = 0;
    stack[0] = root_idx;

    var closest_t: f32 = math::MAX;
    var normal: vec3f = vec3f();
    var geometry_normal: vec3f = vec3f();
    var uv0: vec2f = vec2f();
    var uv1: vec2f = vec2f();

    while stack_top >= 0 {
        let node_idx = stack[stack_top];
        stack_top--;

        let node = blas[node_idx];

        if math::rayIntersectAABB(ray, node.lb, node.ub) {

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

                    let hit = rayIntersectTriangle(ray, v0, v1, v2);

                    if hit.t > 0.0 && hit.t < closest_t {
                        closest_t = hit.t;
                        let n0 = vertices[index[0]].normal;
                        let n1 = vertices[index[1]].normal;
                        let n2 = vertices[index[2]].normal;
                        let uv00 = vertices[index[0]].tex_coord0;
                        let uv01 = vertices[index[1]].tex_coord0;
                        let uv02 = vertices[index[2]].tex_coord0;
                        let uv10 = vertices[index[0]].tex_coord1;
                        let uv11 = vertices[index[1]].tex_coord1;
                        let uv12 = vertices[index[2]].tex_coord1;
                        let w0 = 1.0f - hit.u - hit.v;
                        normal = normalize(w0 * n0 + hit.u * n1 + hit.v * n2);
                        geometry_normal = normalize(cross(v1 - v0, v2 - v0));
                        uv0 = w0 * uv00 + hit.u * uv01 + hit.v * uv02;
                        uv1 = w0 * uv10 + hit.u * uv11 + hit.v * uv12;
                    }
                }
            }
        }
    }
    return BlasHitInfo(closest_t, normal, geometry_normal, uv0, uv1);
}

fn rayIntersectTriangle(ray: math::Ray, v0: vec3f, v1: vec3f, v2: vec3f) -> TriangleHit {
    let edge1 = v1 - v0;
    let edge2 = v2 - v0;
    let h = cross(ray.dir, edge2);
    let a = dot(edge1, h);

    // If a is near zero, the ray is parallel to the triangle
    if a > -INTERSECTION_EPSILON && a < INTERSECTION_EPSILON {
        return TriangleHit(-1.0, 0.0, 0.0);
    }

    let f = 1.0 / a;
    let s = ray.pos - v0;
    let u = f * dot(s, h);

    if u < 0.0 || u > 1.0 {
        return TriangleHit(-1.0, 0.0, 0.0);
    }

    let q = cross(s, edge1);
    let v = f * dot(ray.dir, q);

    if v < 0.0 || u + v > 1.0 {
        return TriangleHit(-1.0, 0.0, 0.0);
    }

    let t = f * dot(edge2, q);

    if t > INTERSECTION_EPSILON {
        return TriangleHit(t, u, v);
    }

    return TriangleHit(-1.0, 0.0, 0.0); // Line intersects, but behind the ray origin
}
