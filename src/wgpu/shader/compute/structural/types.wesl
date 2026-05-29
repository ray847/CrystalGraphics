import package::compute::math;

struct TravelInfo {
    hit: bool,
    dist: f32,
};

struct SurfacePos {
    pos: math::Pos,
    shading_norm: math::Vec,
    geometry_norm: math::Vec,
    material_idx: u32,
    uvs: array<vec2f, 2>,
};

struct HitInfo {
    travel_info: TravelInfo,
    surface_pos: SurfacePos,
};
