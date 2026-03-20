/* Data Flow */
@group(0) @binding(0) var target_texture: texture_storage_2d<rgba8unorm, write>;

/* Type */
struct Primitive {
  vert_idx: vec3u,
};
struct Camera {
  pos: vec3f,
  dir: vec3f,
  viewport: vec2f,
};
struct Ray {
  pos: vec3f,
  dir: vec3f,
};

/* Constant */
const kPi: f32 = 3.1415926535;
const kEpsilon: f32 = 1e-6;
/* Vertex */
const kCubePos: vec3f = vec3f(8, 0, 0);
const kCubeRot: vec3f = vec3f(kPi / 4, kPi / 4, kPi / 4);
const kCubeRotMat: mat3x3f = mat3x3f(
  1, 0,               0,
  0, cos(kCubeRot.x), -sin(kCubeRot.x),
  0, sin(kCubeRot.x), cos(kCubeRot.x),
) * mat3x3f(
  cos(kCubeRot.y),  0, sin(kCubeRot.y),
  0,                1, 0,
  -sin(kCubeRot.y), 0, cos(kCubeRot.y),
) * mat3x3f(
  cos(kCubeRot.z), -sin(kCubeRot.z), 0,
  sin(kCubeRot.z), cos(kCubeRot.z),  0,
  0,               0,                1,
);
const kVertArr: array<vec3f, 8> = array<vec3f, 8>(
  kCubePos + kCubeRotMat * vec3f(-1, -1, -1),
  kCubePos + kCubeRotMat * vec3f(-1, -1,  1),
  kCubePos + kCubeRotMat * vec3f(-1,  1, -1),
  kCubePos + kCubeRotMat * vec3f(-1,  1,  1),
  kCubePos + kCubeRotMat * vec3f( 1, -1, -1),
  kCubePos + kCubeRotMat * vec3f( 1, -1,  1),
  kCubePos + kCubeRotMat * vec3f( 1,  1, -1),
  kCubePos + kCubeRotMat * vec3f( 1,  1,  1),
);
const kPrimArr: array<Primitive, 12> = array<Primitive, 12>(
  Primitive(vec3u(0, 1, 2)), Primitive(vec3u(1, 2, 3)),
  Primitive(vec3u(4, 5, 6)), Primitive(vec3u(5, 6, 7)),
  Primitive(vec3u(2, 3, 6)), Primitive(vec3u(3, 6, 7)),
  Primitive(vec3u(0, 1, 4)), Primitive(vec3u(1, 4, 5)),
  Primitive(vec3u(0, 2, 4)), Primitive(vec3u(2, 4, 6)),
  Primitive(vec3u(1, 3, 5)), Primitive(vec3u(3, 5, 7)),
);
/* Camera */
const kCamera: Camera = Camera(
  vec3f(0.0f, 0.0f, 0.0f),
  vec3f(2.0f, 0.0f, 0.0f),
  vec2f(0.640f, 0.480f),
);

/* Main */
@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
  let dims = textureDimensions(target_texture);
  let pixel = global_id.xy;
  let coord: vec2f = (vec2f(pixel) / vec2f(dims)) * 2.0f - vec2f(1, 1);
  if (pixel.x >= dims.x || pixel.y >= dims.y) {
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
  const dx: vec3f = normalize(
    cross(
      vec3f(0, 0, 1),
      kCamera.dir
    )
  ) * kCamera.viewport.x;
  const dy: vec3f = normalize(cross(dx, kCamera.dir)) * kCamera.viewport.y;
  return Ray(kCamera.pos, normalize(kCamera.dir + coord.x * dx + coord.y * dy));
}
fn RayColor(ray: Ray) -> vec3f {
  let hit_info = RayHit(ray);
  let brightness: f32 = 1.0f - clamp(hit_info.dis / 10, 0.0f, 1.0f);
  return vec3f(brightness, brightness, brightness);
}
struct RayHitInfo {
  hit: bool,
  pos: vec3f,
  dis: f32,
};
/**
 * Detect if the input ray hits anything. if so return the hit distance,
 * otherwise return 1e6.
 */
fn RayHit(ray: Ray) -> RayHitInfo {
  var info: RayHitInfo = RayHitInfo(false, ray.pos, 1e6);
  for (var i: u32 = 0;
       i < 12;
       i += 1) {
    let prim = kPrimArr[i];
    let v1 = kVertArr[prim.vert_idx[0]];
    let v2 = kVertArr[prim.vert_idx[1]];
    let v3 = kVertArr[prim.vert_idx[2]];
    let e1 = v2 - v1;
    let e2 = v3 - v1;
    let norm = cross(e1, e2);
    let dir_cross_e2 = cross(ray.dir, e2);
    let det: f32 = dot(e1, dir_cross_e2);
    if (abs(det) < kEpsilon) {
      continue;
    }
    let inv_det = 1 / det;
    let s = ray.pos - v1;
    let u = inv_det * dot (s, dir_cross_e2);
    if (u < 0 || u > 1) {
      continue;
    }
    let s_cross_e1 = cross(s, e1);
    let v = inv_det * dot(ray.dir, s_cross_e1);
    if (v < 0 || u + v > 1) {
      continue;
    }
    let t = inv_det * dot(e2, s_cross_e1);
    if (t > kEpsilon) {
      let p = ray.pos + t * ray.dir;
      let new_dis = t * length(ray.dir);
      if (new_dis < info.dis) {
        info = RayHitInfo(true, p, new_dis);
      }
    }
  }
  return info;
}
