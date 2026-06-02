import package::compute::math;
import package::compute::physics;
import package::compute::structural;
import package::compute::lod;
import super::material::{Material, material};

fn emission(
    material: Material,
) -> vec3f {
    return material.emission;
}