import package::compute::math;
import package::compute::structural;
import package::compute::physics;
import package::compute::lod;
@publish
import super::shading::{
  sample::{
    Sample,
    MAX_TRANSPORT_SAMPLE_COUNT,
    sample as sampleRays,
    EMISSION_HIT_EPSILON,
  },
  spawn_surface_ray::{spawnSurfaceRay},
  material::{material},
  collect::collect,
  emission::{emission},
};

struct ShadingPassContext {
    samples: Sample,
    transport_radiance: array<physics::Radiance, MAX_TRANSPORT_SAMPLE_COUNT>,
    emission: physics::Radiance,
};

fn forwardPass(
    curr_dir: math::Vec,
    surface_pos: structural::SurfacePos,
    lod: lod::LOD
) -> ShadingPassContext {
    let material = material(surface_pos.material_idx, surface_pos.uvs);
    return ShadingPassContext(
        sampleRays(curr_dir, surface_pos, material, lod),
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
