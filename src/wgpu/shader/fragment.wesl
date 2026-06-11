@group(0) @binding(0) var my_texture: texture_2d<f32>;
@group(0) @binding(1) var my_sampler: sampler;

@fragment
fn fragMain(@location(0) uv: vec2f) -> @location(0) vec4f {
    return textureSample(my_texture, my_sampler, uv); 
}
