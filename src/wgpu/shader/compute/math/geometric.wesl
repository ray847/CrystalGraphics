import super::ray::Ray;

const SAFE_DIRECTION_EPSILON: f32 = 1e-7;

fn dirInv(dir: vec3f) -> vec3f {
    // If any component is exactly 0.0, replace it with a microscopic 
    // epsilon so division results in Infinity without causing NaN.
    let is_zero = dir == vec3f(0.0);
    let safe_dir = select(dir, vec3f(SAFE_DIRECTION_EPSILON), is_zero);
    return 1.0 / safe_dir;
}

fn rayIntersectAABB(ray: Ray, lb: vec3f, ub: vec3f) -> bool {
    let inv_dir = dirInv(ray.dir);

    let t0 = (lb - ray.pos) * inv_dir;
    let t1 = (ub - ray.pos) * inv_dir;

    let tmin = min(t0, t1);
    let tmax = max(t0, t1);

    let t_near = max(max(tmin.x, tmin.y), tmin.z);
    let t_far = min(min(tmax.x, tmax.y), tmax.z);

    return t_far >= t_near && t_far > 0.0;
}

fn makeTangentSpace(n: vec3f) -> mat3x3f {
    let helper = select(vec3f(1.0, 0.0, 0.0), vec3f(0.0, 1.0, 0.0), abs(n.x) > 0.9);
    let tangent = normalize(cross(helper, n));
    let bitangent = cross(n, tangent);
    return mat3x3f(tangent, bitangent, n);
}
