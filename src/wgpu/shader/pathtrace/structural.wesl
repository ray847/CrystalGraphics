alias Pos = vec3f;
alias Vec = vec3f;

struct TravelInfo {
    hit: bool,
    dist: f32,
};

struct SurfacePos {
    pos: Pos,
    norm: Vec,
};

struct HitInfo {
    travel_info: TravelInfo,
    surface_pos: SurfacePos,
};
