import package::compute::lod;
import package::compute::math;
import package::compute::physics;
import package::compute::shading;
import package::compute::structural;
import package::compute::environment;

const MAX_TRACE_DEPTH: u32 = 64u;

struct Stack {
    /* Argument */
    ray: math::Ray,
    lod: lod::LOD,
    /* Local */
    hit_info: structural::HitInfo,
    shading_pass_context: shading::ShadingPassContext,
    iter: u32,
    in_recurse: bool,
};

fn trace(ray: math::Ray, wavelength: physics::Wavelength) -> physics::Radiance {
    var ret: physics::Radiance;
    var stk = array<Stack, MAX_TRACE_DEPTH>(); // zero initialized
    var top: i32 = 0;
    /* Write first argument. */
    stk[top].ray = ray;
    while top >= 0 {
        if !stk[top].in_recurse { // first exection

            stk[top].hit_info = structural::rayHit(stk[top].ray);

            if !stk[top].hit_info.travel_info.hit {
                /* Return */
                ret = environment::radiance(stk[top].ray.dir, wavelength);
                stk[top] = Stack(); // clear stack frame
                top--;
                continue;
            }

            if lod::reachEnd(stk[top].lod) || u32(top) == MAX_TRACE_DEPTH - 1 {
                /* Return */
                ret = physics::Radiance();
                stk[top] = Stack(); // clear stack frame
                top--;
                continue;
            }

            else {
                /* Shading forward pass. */
                stk[top].shading_pass_context = shading::forwardPass(
                    stk[top].ray.dir,
                    wavelength,
                    stk[top].hit_info.surface_pos,
                    stk[top].lod
                );
                stk[top].iter = 0;
                stk[top].in_recurse = true;
                solveEmission(
                    stk[top].hit_info.surface_pos,
                    &(stk[top].shading_pass_context)
                );
            }
        } else { // in loop, collect return value
            stk[top].shading_pass_context.transport_radiance[stk[top].iter - 1] = ret;
        }

        /* Sample Loop */
        if stk[top].iter <
            stk[top].shading_pass_context.samples.transport_count { // recursion
            stk[top + 1].ray = shading::spawnSurfaceRay(
                stk[top].hit_info.surface_pos,
                stk[top].shading_pass_context.samples.transport_samples[stk[top].iter].dir
            );
            stk[top + 1].lod = lod::decr(stk[top].lod);
            stk[top].iter += 1;
            top++;
            continue;
        } else { // return
            ret = shading::backwardPass(stk[top].shading_pass_context);
            stk[top] = Stack(); // clear stack frame
            top--;
            continue;
        }
    }
    return ret;
}

fn solveEmission(
    surface_pos: structural::SurfacePos,
    shading_pass_context: ptr<function, shading::ShadingPassContext>
) {
    for (var i: u32; i < (*shading_pass_context).samples.emission_count; i++) {
        let emission_sample = \
            (*shading_pass_context).samples.emission_samples[i];
        let sampled_light_pos = surface_pos.pos
            + emission_sample.dir * emission_sample.expected_dist;
        let shadow_ray = shading::spawnSurfaceRay(
            surface_pos,
            emission_sample.dir
        );
        let expected_dist = distance(shadow_ray.pos, sampled_light_pos);
        let hit_info = structural::rayHit(shadow_ray);
        if hit_info.travel_info.hit &&
            hit_info.travel_info.dist <
            (expected_dist - shading::EMISSION_HIT_EPSILON) {
            (*shading_pass_context).samples.emission_samples[i].radiance = \
                physics::Radiance();
        }
    }
}
