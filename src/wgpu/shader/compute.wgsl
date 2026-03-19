@group(0) @binding(0) var texture : texture_storage_2d<rgba8unorm, write>;

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
  let dimensions = textureDimensions(texture);
  let coords = global_id.xy;
  if (coords.x >= dimensions.x || coords.y >= dimensions.y) {
    return;
  }
  textureStore(
    texture,
    coords,
    vec4f(
      f32(coords.x) / f32(dimensions.x),
      f32(coords.y) / f32(dimensions.y),
      0.0f,
      1.0f
    )
  );
}