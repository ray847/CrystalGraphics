import package::pathtrace::{
    math::Ray,
    structural::HitInfo,
    bvh::rayHit,
    lod::{LOD, reachEnd, decr},
    radiance::{Radiance, radiate},
    sample::{SampledRays, sampleRays, MAX_SAMPLE_COUNT},
    collect::collect,
};

const MAX_TRACE_DEPTH: u32 = 8;

struct Stack {
    /* Argument */
    ray: Ray,
    lod: LOD,
    /* Local */
    hit_info: HitInfo,
    sampled_rays: SampledRays,
    sampled_ray_radiance: array<Radiance, MAX_SAMPLE_COUNT>,
    iter: i32,
    in_recurse: bool,
    /* Return */
    surface_radiance: Radiance,
};

fn trace(ray: Ray) -> Radiance {
    var ret: Radiance;
    var stk = array<Stack, MAX_TRACE_DEPTH>(); // zero initialized
    var top: i32 = 0;
    /* Write first argument. */
    stk[top].ray = ray;
    stk[top].lod = LOD(0);
    while top >= 0 {
        /* First Execution */
        if !stk[top].in_recurse {

            stk[top].hit_info = rayHit(stk[top].ray);
            if !stk[top].hit_info.travel_info.hit {
                /* Return */
                ret = environmentRadiance(stk[top].ray.dir);
                stk[top] = Stack(); // clear stack frame
                top--;
                continue;
            }

            if reachEnd(stk[top].lod) || u32(top) == MAX_TRACE_DEPTH - 1 {
                /* Return */
                ret = radiate(stk[top].hit_info.surface_pos, stk[top].lod);
                stk[top] = Stack(); // clear stack frame
                top--;
                continue;
            } else {
                stk[top].sampled_rays = sampleRays(
                    stk[top].ray.dir,
                    stk[top].hit_info.surface_pos,
                    stk[top].lod
                );
                stk[top].iter = 0;
                stk[top].in_recurse = true;
            }
        }
        /* Collect Return Value */
        else {
            stk[top].sampled_ray_radiance[stk[top].iter - 1] = ret;
        }

        /* Sample Loop */
        if stk[top].iter < stk[top].sampled_rays.count {
            /* Recursion Call */
            stk[top + 1].ray = stk[top].sampled_rays.rays[stk[top].iter];
            stk[top + 1].lod = decr(stk[top].lod);
            stk[top].iter += 1;
            top++;
            continue;
        } else {
            /* Return */
            /* Input radiance. */
            ret = collect(
                stk[top].ray.dir,
                stk[top].sampled_rays,
                stk[top].sampled_ray_radiance
            );
            stk[top] = Stack(); // clear stack frame
            top--;
            continue;
        }
    }
    return ret;
}

fn environmentRadiance(dir: vec3f) -> Radiance {
    let t = 0.5 * (dir.y + 1.0);
    return mix(Radiance(0.7, 0.8, 1.0), Radiance(0.05, 0.05, 0.06), 1.0 - t);
}
