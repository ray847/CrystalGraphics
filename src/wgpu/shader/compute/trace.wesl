import package::compute::lod;
import package::compute::math;
import package::compute::physics;
import package::compute::shading;
import package::compute::structural;

const MAX_TRACE_DEPTH: u32 = 8;

struct Stack {
    /* Argument */
    ray: math::Ray,
    lod: lod::LOD,
    /* Local */
    hit_info: structural::HitInfo,
    sampled_rays: shading::SampledRays,
    iter: u32,
    in_recurse: bool,
    /* Return */
    surface_radiance: physics::Radiance,
};

fn trace(ray: math::Ray) -> physics::Radiance {
    var ret: physics::Radiance;
    var stk = array<Stack, MAX_TRACE_DEPTH>(); // zero initialized
    var top: i32 = 0;
    /* Write first argument. */
    stk[top].ray = ray;
    stk[top].lod = lod::LOD(0);
    while top >= 0 {
        /* First Execution */
        if !stk[top].in_recurse {

            stk[top].hit_info = structural::rayHit(stk[top].ray);
            if !stk[top].hit_info.travel_info.hit {
                /* Return */
                ret = environmentRadiance(stk[top].ray.dir);
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
            } else {
                let curr_dir = -stk[top].ray.dir;
                stk[top].sampled_rays = shading::sampleRays(
                    curr_dir,
                    stk[top].hit_info.surface_pos,
                    stk[top].lod
                );
                stk[top].iter = 0;
                stk[top].in_recurse = true;
            }
        }
        /* Collect Return Value */
        else {
            stk[top].sampled_rays.ray_radiance[stk[top].iter - 1] = ret;
        }

        /* Sample Loop */
        if stk[top].iter < stk[top].sampled_rays.count {
            /* Recursion Call */
            stk[top + 1].ray = stk[top].sampled_rays.rays[stk[top].iter];
            stk[top + 1].lod = lod::decr(stk[top].lod);
            stk[top].iter += 1;
            top++;
            continue;
        } else {
            /* Return */
            /* Input radiance. */
            let curr_dir = -stk[top].ray.dir;
            ret = shading::collect(
                curr_dir,
                stk[top].sampled_rays
            );
            stk[top] = Stack(); // clear stack frame
            top--;
            continue;
        }
    }
    return ret;
}

fn environmentRadiance(dir: vec3f) -> physics::Radiance {
    let t = 0.5 * (dir.y + 1.0);
    return mix(
        physics::Radiance(0.7, 0.8, 1.0),
        physics::Radiance(0.05, 0.05, 0.06),
        1.0 - t
    );
}
