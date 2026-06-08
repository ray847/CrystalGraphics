import package::compute::math;
import package::compute::physics;
import package::compute::render;

@id(0) override RENDER_WIDTH: u32;
@id(1) override RENDER_HEIGHT: u32;

/* Type */
struct UniformData {
    camera: Camera,
    iter_count: u32,
};
struct Camera {
    pos: vec3f,
    dir: vec3f,
    viewport: vec2f,
};
struct HistoryPixel {
    luminance: physics::Luminance,
    rand_seed: u32,
};
struct HistoryFrame {
    pixels: array<HistoryPixel>,
};

/* DataFlow */
@group(0) @binding(0) var target_texture: texture_storage_2d<rgba8unorm, write>;
@group(1) @binding(0) var<uniform> uniform_data: UniformData;
@group(0) @binding(1) var<storage, read_write> history_frame: HistoryFrame;

/* Main */
@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let pixel = global_id.xy;
    if pixel.x >= RENDER_WIDTH || pixel.y >= RENDER_HEIGHT {
        return;
    }

    let ray = cameraRay(uniform_data.camera, pixel);
    let history_idx = pixel.x + pixel.y * RENDER_WIDTH;
    let sample_weight = 1.0f / f32(uniform_data.iter_count + 1u);

    var luminance = physics::Luminance();
    var rand_seed = pixel.x + pixel.y * RENDER_WIDTH;
    if uniform_data.iter_count > 0u {
        luminance = history_frame.pixels[history_idx].luminance *
            (1.0f - sample_weight);
        rand_seed = history_frame.pixels[history_idx].rand_seed;
    }

    math::seedRng(rand_seed);
    luminance += render::render(ray) * sample_weight;
    history_frame.pixels[history_idx] = HistoryPixel(
        luminance,
        math::randUInt()
    );
    textureStore(
        target_texture,
        pixel,
        vec4f(physics::luminanceToSrgb(luminance), 1.0f)
    );
}

fn cameraRay(camera: Camera, pixel: vec2u) -> math::Ray {
    let coord: vec2f = \
        (vec2f(pixel) / vec2f(f32(RENDER_WIDTH), f32(RENDER_HEIGHT))) * 2.0f -
        vec2f(1, 1);
    let dx: vec3f = normalize(
        cross(
            camera.dir,
            vec3f(0, 0, 1)
        )
    ) * camera.viewport.x;
    let dy: vec3f = normalize(cross(camera.dir, dx)) * camera.viewport.y;
    return math::Ray(
        camera.pos,
        normalize(camera.dir + coord.x * dx + coord.y * dy)
    );
}
