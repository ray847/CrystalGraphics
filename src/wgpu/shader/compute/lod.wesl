const MAX_DEPTH = 2;

struct LOD {
    depth: i32,
};

fn reachEnd(lod: LOD) -> bool {
    return lod.depth >= MAX_DEPTH;
}

fn decr(lod: LOD) -> LOD {
    let decr_lod = LOD(lod.depth + 1);
    return decr_lod;
}

fn heuristic(lod: LOD) -> f32 {
    return f32(4 - lod.depth) / 4.0;
}