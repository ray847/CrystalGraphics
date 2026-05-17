import package::pathtrace::{
  math::{Ray, randFloat, makeTangentSpace, PI, RAY_OFFSET},
  structural::SurfacePos,
  material::{Material, materials}
};

/* Sample Function */
fn specularDir(
    surface_pos: SurfacePos,
    out_dir: vec3f,
    material: Material
) -> vec3f {
    return reflect(out_dir, surface_pos.norm);
}

/* Distribution Function */
fn specularDist(
    surface_pos: SurfacePos,
    in_dir: vec3f,
    material: Material
) -> f32 {
}