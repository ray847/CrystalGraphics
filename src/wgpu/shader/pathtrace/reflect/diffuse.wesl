import package::pathtrace::{
  math::{Ray, randFloat, makeTangentSpace, PI, RAY_OFFSET},
  structural::SurfacePos,
  material::{Material, baseColor}
};

/* Sample Function */
fn diffuseDir(
    surface_pos: SurfacePos,
    out_dir: vec3f,
    material: Material
) -> vec3f {
    let in_dir = makeTangentSpace(surface_pos.norm) * cosineWeightedSample();
    return in_dir;
}
fn cosineWeightedSample() -> vec3f {
    let r = sqrt(randFloat());
    let angle = 2 * PI * randFloat();
    let dir = vec3f(
        r * cos(angle),
        r * sin(angle),
        sqrt(max(0.0f, 1 - r * r))
    );
    return dir;
}

/* Distribution Function */
fn diffuseDist(
    surface_pos: SurfacePos,
    in_dir: vec3f,
    material: Material
) -> vec3f {
    let projection_factor = dot(in_dir, surface_pos.norm);
    return lambertianDist(material, surface_pos) * projection_factor;
}
fn lambertianDist(material: Material, surface_pos: SurfacePos) -> vec3f {
    return baseColor(material, surface_pos.uv).rgb;
}
