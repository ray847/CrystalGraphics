struct Ray {
    pos: vec3f,
    dir: vec3f,
};
/* Constant */
const PI: f32 = 3.1415926535;
const EPSILON: f32 = 1e-12;
const RAY_OFFSET: f32 = 1e-4;
const MAX: f32 = 1e6;

fn dir_inv(dir: vec3f) -> vec3f {
    // If any component is exactly 0.0, replace it with a microscopic 
    // epsilon so division results in Infinity without causing NaN.
    let is_zero = dir == vec3f(0.0);
    let safe_dir = select(dir, vec3f(1e-7), is_zero);
    return 1.0 / safe_dir;
}

fn ray_intersect_aabb(ray: Ray, lb: vec3f, ub: vec3f) -> bool {
    let inv_dir = dir_inv(ray.dir);

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

var<private> rng_state: u32;

fn seed_rng(seed: u32) {
    rng_state = seed;
    step_rng();
}

fn step_rng() {
    rng_state = rng_state * 747796405u + 2891336453u;
    var word: u32 = ((rng_state >> ((rng_state >> 28u) + 4u)) ^ rng_state) * 277803737u;
    rng_state = (word >> 22u) ^ word;
}

fn rand_float() -> f32 {
    step_rng();
    return f32(rng_state) / 4294967296.0;
}
