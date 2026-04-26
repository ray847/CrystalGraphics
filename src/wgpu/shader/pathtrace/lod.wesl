struct LOD {
    depth: i32,
};

fn reach_end(lod: LOD) -> bool {
    return lod.depth > 4;
}

fn decr(lod: LOD) -> LOD {
    let decr_lod = LOD(lod.depth + 1);
    return decr_lod;
}