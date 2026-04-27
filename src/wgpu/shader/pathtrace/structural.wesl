alias Pos = vec3f;
alias Vec = vec3f;

struct TravelInfo {
    hit: bool,
    dist: f32,
};

struct SurfacePos {
    pos: Pos,
    norm: Vec,
    material_idx: u32,
};

struct HitInfo {
    travel_info: TravelInfo,
    surface_pos: SurfacePos,
};
