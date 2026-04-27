# Documents

## Bindgroup Setup

| Resource | BindGroup | Shader Stage |
|-|-|-|
| Target Texture | Group0 | Comp & Vert & Frag |
| Target Texture Sampler | Group0 | Frag |
| Camera Uniform | Group1 | Comp |
| VBO | Group2 | Comp |

## Pathtracing Logic

```pseudo code
Luminance Render(ray, lod):
  return Illuminate(Trace(ray, lod))

Luminance Illuminate(radiance):
  ...

Radiance Trace(ray, lod):

  hit_info = HitBVH(ray)
  if !Hit(hit_info):
    return 0

  Radiance surface_radiance = 0

  if ReachEnd(lod):
    surface_radiance = Radiate(hit_info.surface_pos)

  else:
    sampled_rays = Sample(hit_info.surface_pos, lod)
    surface_radiance = Radiate(hitinfo.pos) + ave_radiance(sampled_rays) / 2

  return Travel(surface_radiance, hit_info.travel)

SampledRays Sample(surface_pos, lod):
  sampled_rays = Sample(surface_pos, lod)
  for sampled_ray in generated:
    sampled_ray.radiance = Trace(generated_ray, decr(lod))
  return sampled_rays
```

# Reflection Model

Goal is to approximate the **Distribution Function**:
$$
f(w_o, w_i)
$$

A distribution function should confine to these constraints:
$$
\begin{cases}
\forall w_o, w_i, f(w_o, w_i) > 0 \\
\forall w_o, w_i, f(w_o, w_i) = f(w_i, w_o) \\
\forall w_i, \int_\Omega f(w_o, w_i)d\theta \leq 0
\end{cases}
$$

## Lambertian Model

$f$ is constant, meaning it is a diffuse reflection model.
Given the reflection coefficient $R$, meaning $L_o = R \cdot L_i$.
$$

f(w_o, w_i) = \frac{R}{\pi}
$$

