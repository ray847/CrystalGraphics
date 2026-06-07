import package::compute::math;
import package::compute::structural;
import package::compute::physics;
import package::compute::lod;
import super::shading::{
  sample::{
    sample as sampleRays,
  },
  material::{material},
  collect::collect,
  emission::{emission},
};

@publish
import super::shading::sample::{
  Sample,
  MAX_TRANSPORT_SAMPLE_COUNT,
  EMISSION_HIT_EPSILON,
};

@publish
import super::shading::spawn_surface_ray::{spawnSurfaceRay};

struct ShadingPassContext {
    samples: Sample,
    transport_radiance: array<physics::Radiance, MAX_TRANSPORT_SAMPLE_COUNT>,
    emission: physics::Radiance,
};

fn forwardPass(
    curr_dir: math::Vec,
    wavelength: physics::Wavelength,
    surface_pos: structural::SurfacePos,
    lod: lod::LOD
) -> ShadingPassContext {
    let material = material(
        wavelength,
        surface_pos.material_idx,
        surface_pos.uvs
    );
    return ShadingPassContext(
        sampleRays(curr_dir, wavelength, surface_pos, material, lod),
        array<physics::Radiance, MAX_TRANSPORT_SAMPLE_COUNT>(),
        emission(material)
    );
}

fn backwardPass(context: ShadingPassContext) -> physics::Radiance {
    return context.emission + collect(
        context.samples,
        context.transport_radiance
    );
}
