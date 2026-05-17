@group(0) @binding(0) var my_texture: texture_2d<f32>;
@group(0) @binding(1) var my_sampler: sampler;

@fragment
fn fragMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    // Convert screen pixel position to 0.0 -> 1.0 UV coordinates
    let dims = vec2f(textureDimensions(my_texture));
    let uv = pos.xy / dims;
    
    // Read the pixel from your computed texture!
    return textureSample(my_texture, my_sampler, uv); 
}