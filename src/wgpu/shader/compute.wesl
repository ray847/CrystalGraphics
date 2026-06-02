import package::compute::math;
import package::compute::render;

/* Type */
struct Camera {
    pos: vec3f,
    dir: vec3f,
    viewport: vec2f,
};

/* DataFlow */
@group(0) @binding(0) var target_texture: texture_storage_2d<rgba8unorm, write>;
@group(1) @binding(0) var<uniform> camera: Camera;

/* Main */
@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let dims = textureDimensions(target_texture);
    let pixel = global_id.xy;
    let coord: vec2f = (vec2f(pixel) / vec2f(dims)) * 2.0f - vec2f(1, 1);
    if pixel.x >= dims.x || pixel.y >= dims.y {
        return;
    }
    math::seedRng(pixel.x + pixel.y *dims.x);
    let ray = cameraRay(coord);
    let color: vec3f = render::render(ray);
    textureStore(
        target_texture,
        pixel,
        vec4f(
            color,
            1.0f
        )
    );
}

fn cameraRay(coord: vec2f) -> math::Ray {
    let dx: vec3f = normalize(
        cross(
            camera.dir,
            vec3f(0, 0, 1)
        )
    ) * camera.viewport.x;
    let dy: vec3f = normalize(cross(camera.dir, dx)) * camera.viewport.y;
    return math::Ray(camera.pos, normalize(camera.dir + coord.x * dx + coord.y * dy));
}
