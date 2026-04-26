@group(0) @binding(0)
var my_texture: texture_2d<f32>;

@group(0) @binding(1)
var my_sampler: sampler;

@fragment
fn frag_main(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    let dims = vec2f(textureDimensions(my_texture));
    let uv = pos.xy / dims;
    return textureSample(my_texture, my_sampler, uv);
}

