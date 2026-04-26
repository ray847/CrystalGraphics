import package::pathtrace::{
    math::Ray,
    structural::HitInfo,
    bvh::ray_hit,
    lod::{LOD, reach_end, decr},
    radiance::{Radiance, radiate},
    sample::{Sample, sample},
};

const MAX_TRACE_DEPTH: u32 = 8;

struct Stack {
    /* Argument */
    ray: Ray,
    lod: LOD,
    /* Local */
    hit_info: HitInfo,
    sampled_rays: Sample,
    iter: u32,
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

            stk[top].hit_info = ray_hit(stk[top].ray);
            if !stk[top].hit_info.travel_info.hit {
                /* Return */
                ret = environment_radiance(stk[top].ray.dir);
                stk[top] = Stack(); // clear stack frame
                top--;
                continue;
            }

            if reach_end(stk[top].lod) || u32(top) == MAX_TRACE_DEPTH - 1 {
                /* Return */
                ret = radiate(stk[top].hit_info.surface_pos, stk[top].lod);
                stk[top] = Stack(); // clear stack frame
                top--;
                continue;
            } else {
                stk[top].sampled_rays = sample(
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
            stk[top].sampled_rays.ray_radiance[stk[top].iter - 1] = ret;
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
            /* Temporary Solution */
            if stk[top].sampled_rays.count == 0 {
                ret = Radiance(0.0);
                stk[top] = Stack();
                top--;
                continue;
            }
            ret = Radiance(0.0);
            for (var i: u32; i < stk[top].sampled_rays.count; i++) {
                ret += stk[top].sampled_rays.ray_radiance[i];
            }
            ret /= f32(stk[top].sampled_rays.count);
            // Temporary local/recursive blend until material throughput exists.
            // ret = (ret + radiate(stk[top].hit_info.surface_pos, stk[top].lod)) / 2.0;
            stk[top] = Stack(); // clear stack frame
            top--;
            continue;
        }
    }
    return ret;
}

fn environment_radiance(dir: vec3f) -> Radiance {
    let t = 0.5 * (dir.y + 1.0);
    return mix(Radiance(0.7, 0.8, 1.0), Radiance(0.05, 0.05, 0.06), 1.0 - t);
}
