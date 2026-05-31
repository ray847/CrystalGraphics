import package::compute::math;
import package::compute::structural;
import super::super::material::{Material, IOR};
import super::super::model_sample::ModelSample;
import super::util::{relIor, fresnelReflectance, orientedNormal};

/* Sample Function */
fn sample(
    surface_pos: structural::SurfacePos,
    curr_dir: math::Vec,
    material: Material
) -> ModelSample {
    if material.volumn_transmission {
        let rel_ior: IOR = relIor(surface_pos.shading_norm, curr_dir, material);
        let dist = transmissionDist(
            curr_dir,
            surface_pos.shading_norm,
            material
        );
        return ModelSample(
            refractDir(surface_pos.shading_norm, curr_dir, rel_ior),
            dist
        );
    }

    let oriented_norm = orientedNormal(curr_dir, surface_pos.shading_norm);
    return ModelSample(
        curr_dir,
        transmissionDist(curr_dir, oriented_norm, material)
    );
}

fn transmissionDist(
    curr_dir: math::Vec,
    normal: math::Vec,
    material: Material
) -> vec3f {
    return material.transmission
        * (1 - fresnelReflectance(curr_dir, normal, material.ior));
}

fn refractDir(
    shading_norm: math::Vec,
    curr_dir: math::Vec,
    rel_ior: IOR
) -> math::Vec {
    let rel_ior_re: f32 = (rel_ior.r.re + rel_ior.g.re + rel_ior.b.re) / 3;
    let normal = select(
        shading_norm,
        -shading_norm,
        dot(curr_dir, shading_norm) > 0.0f,
    );
    return refract(curr_dir, normal, rel_ior_re);
}
