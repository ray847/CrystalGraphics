@vertex
fn vert_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
  if (in_vertex_index == 0u) {
    return vec4f(-1.0, -1.0, 0.0, 1.0);
  } else if (in_vertex_index == 1u) {
    return vec4f(-1.0, 3.0, 0.0, 1.0);
  } else {
    return vec4f(3.0, -1.0, 0.0, 1.0);
  }
}