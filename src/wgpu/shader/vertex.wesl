struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}

@vertex
fn vertMain(@builtin(vertex_index) in_vertex_index: u32) -> VertexOutput {
    if in_vertex_index == 0u {
        return VertexOutput(vec4f(-1.0, -1.0, 0.0, 1.0), vec2f(0.0, 1.0));
    } else if in_vertex_index == 1u {
        return VertexOutput(vec4f(-1.0, 3.0, 0.0, 1.0), vec2f(0.0, -1.0));
    } else {
        return VertexOutput(vec4f(3.0, -1.0, 0.0, 1.0), vec2f(2.0, 1.0));
    }
}
