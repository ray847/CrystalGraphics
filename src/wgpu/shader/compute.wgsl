/* Data Flow */
@group(0) @binding(0) var target_texture: texture_storage_2d<rgba8unorm, write>;
@group(1) @binding(0) var<uniform> camera: Camera;
@group(2) @binding(0) var<storage, read> tlas: array<TLASNode>;

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
  lchild_primitive_offset: u32,
  ub: vec3f,
  primitive_count: u32,
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
  let dx: vec3f = normalize(
    cross(
      vec3f(0, 0, 1),
      camera.dir
    )
  ) * camera.viewport.x;
  let dy: vec3f = normalize(cross(dx, camera.dir)) * camera.viewport.y;
  return Ray(camera.pos, normalize(camera.dir + coord.x * dx + coord.y * dy));
}
fn RayColor(ray: Ray) -> vec3f {
  let hit_info = RayHit(ray);
  let brightness: f32 = clamp(hit_info.val / 50.0, 0.0, 1.0);
  return vec3f(brightness, brightness, brightness);
}
struct RayHitInfo {
  val: f32,
};
/**
 * Detect if the input ray hits anything. if so return the hit distance,
 * otherwise return 1e6.
 */
fn RayHit(ray: Ray) -> RayHitInfo {
  return RayHitInfo(RayTraverseTLAS(ray));
}
fn RayIntersectAABB(ray: Ray, lb: vec3f, ub: vec3f) -> bool {
  let t0 = (lb - ray.pos) / ray.dir;
  let t1 = (ub - ray.pos) / ray.dir;

  let tmin = min(t0, t1);
  let tmax = max(t0, t1);

  let t_near = max(max(tmin.x, tmin.y), tmin.z);
  let t_far = min(min(tmax.x, tmax.y), tmax.z);

  return t_far >= t_near && t_far > 0.0;
}
fn RayTraverseTLAS(ray: Ray) -> f32 {
  var stack: array<u32, 32>;
  var stack_top: i32 = 0;
  stack[0] = 0u;
  var nodes_visited: f32 = 0.0;
  while (stack_top >= 0) {
    let node_idx = stack[stack_top];
    stack_top--;
    
    let node = tlas[node_idx];
    
    if (RayIntersectAABB(ray, node.lb, node.ub)) {
      nodes_visited += 1.0;
      
      if (node.primitive_count == 0u) {
        let lchild = node.lchild_primitive_offset;
        let rchild = lchild + 1;
        
        stack_top++;
        stack[stack_top] = lchild;
        stack_top++;
        stack[stack_top] = rchild;
      }
    }
  }
  return nodes_visited;
}