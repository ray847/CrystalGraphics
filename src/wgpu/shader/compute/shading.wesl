import package::compute::math;
import package::compute::structural;
import package::compute::physics;
import package::compute::lod;
import super::shading::{
  sample::{SampledRays, sampleRays, MAX_SAMPLE_COUNT},
  material::{material},
  collect::collect,
  emission::{emission},
};

struct ShadingPassContext {
    sampled_rays: SampledRays,
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
        emission(material)
    );
}

fn backwardPass(context: ShadingPassContext) -> physics::Radiance {
    return context.emission + collect(context.sampled_rays);
}