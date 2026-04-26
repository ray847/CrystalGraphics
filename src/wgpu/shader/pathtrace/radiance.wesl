import package::pathtrace::{
    structural::{Pos, Vec, SurfacePos},
    lod::LOD,
};

alias Radiance = vec3f;

fn radiate(surface_pos: SurfacePos, lod: LOD) -> Radiance {
    return Radiance(); // no lights for now
}
