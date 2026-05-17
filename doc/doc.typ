#import "@preview/cetz:0.5.0"
#import "TypstTemplate/lib.typ": *
#import "@preview/plotsy-3d:0.2.1": plot-3d-parametric-surface

#show: formal
#set math.equation(numbering: "(1)")

#set document(
  title: "Crystal Graphics Document",
  author: "Ray",
  date: datetime.today(),
)

#title()

= Physics <physics>

#let wo = $arrow(w)_o$
#let wi = $arrow(w)_i$
#let norm = $arrow(n)$
#let angle(..args) = {
  let body = args.pos().join($,$)
  $ lr(chevron.l #body chevron.r) $
}

/ The Rendering Equation:

$
  L(wo) =
  L_e (wo) + integral_wi L(wi) f(wo, wi) cos angle(wi, norm) dif wi
$ <rendering-equation>

== Model Behavior

Generally speaking, the outward radiance $L(wo)$ is a cumulative product of 2 *types* of inward radiance $L(wi)$:
- Continuous: Diffuse
- Discrete: Specular & Transmission

Although both types are well expressed in math with the same form (@rendering-equation), we delliberately treat them differently since this form of arithmetic expression is not easily applicable in actual computation.

/ Continuous Models:

This type of reflection(scatter) model redirects light from a *range* of inward directions to the outward direction.
As the redirection involves a continuous range, a reflection(scatter) distribution function is commonly used to approximate how much a inward direction contributes to the outward direction.

$
  L_o (wo) = integral_wi L_i (wi) f(wo, wi) cos angle(wi, norm) dif wi
$

/ Discrete Models:

This type of reflection(scatter) model redirects light from certain inward angles.
Since the inward angles are discrete (and also of a limited number in most cases), we can just add the elements together without the need to estimate integrals #emoji.face.blush.

$
  L_o (wo) = sum_wi L_i (wi) f(wo, wi) cos angle(wi, norm)
$

== Reflection

In the real world most materials exhibit combinations of reflection & scatter behaviors, but it is easier to discuss them seperately since the underlying physical process is different and we use different approximation models.

=== Diffuse Reflection

#figure(
  image("asset/diffuse_reflection.svg"),
  caption: "diffuse reflection",
)

/ Lambertian Reflection:

$
  f = R / pi
$

$f$ is constant. And light is reflected from all inward directions in the same hemisphere.

$
  "Given"
  R & = integral.double_wo f(wi, wo) cos angle(wi, wo) dif wo ,                        &             \
  R & = f integral_0^(2pi) integral_0^(emph(pi / 2)) cos phi sin phi dif theta dif phi & (f equiv C) \
  R & = f dot 2 pi dot 1 / 4                                                           &             \
  f & = R / pi                                                                         &             \
$

=== Dielectric Specular Reflection

#align(center, cetz.canvas({
  import cetz.draw: *
  line((-4, 0), (4, 0), name: "surface")
  line((0, 3), (0, -3), stroke: (dash: "dashed"))
  content((-3, 0.5), $1.0$)
  content((-3, -0.5), $eta$)

  arc((0, 5mm), start: 90deg, stop: 150deg, radius: 5mm, mode: "PIE", fill: blue.lighten(50%), name: "in_angle")
  arc((0, 5mm), start: 90deg, stop: 30deg, radius: 5mm, mode: "PIE", fill: blue.lighten(70%), name: "reflect_angle")
  arc(
    (0, -5mm),
    start: -90deg,
    stop: -60deg,
    radius: 5mm,
    mode: "PIE",
    fill: blue.lighten(70%),
    name: "transmission_angle",
  )

  line((0, 0), (150deg, 4), mark: (start: "straight"), name: "in")
  line((0, 0), (30deg, 4), mark: (end: "straight"), name: "reflect")
  line((0, 0), (-60deg, 3), mark: (end: "straight"), name: "transmission")
  set-style(content: (frame: "rect", stroke: none, fill: none, padding: .1))

  content("in_angle", $theta_i$, anchor: "south-east")
  content("reflect_angle", $theta_r$, anchor: "south-west")
  content("transmission_angle", $theta_t$, anchor: "north-west")
  content("in.end", $R_i$, anchor: "south-west")
  content("reflect.end", $R_r$, anchor: "south-east")
  content("transmission.end", $R_t$, anchor: "west")
}))

$
  & cases(
      theta_r = theta_i,
      sin theta_t = eta sin theta_i #h(1em) "(Snell's Law)",
    ) \
  & cases(
      R_r = 1 / 2
      (
        ((eta cos theta_i - cos theta_t) / (eta cos theta_i + cos theta_t))^2
        +
        ((cos theta_i - eta cos theta_t) / (cos theta_i + eta cos theta_t))^2
      ) R_i,
      R_t = R_i - R_r
    )
$

=== Metal Reflection

=== Clearcoat

=== Sheen & Fuzz

=== Anisotropy

=== Iridescence

== Transmission

=== Specular Refraction

=== Rough Refraction

=== Absorbing Transmission

=== Diffuse Transmission

=== Volume Scatter

= Implementation

== Overview

The pathtracing process is divided into the following steps:

```py
void main:
  rays = camera.GenerateRays()
  for ray in rays:
    Render(ray)

Luminance Render(ray):
  return Luminate(Trace(ray))

Radiance Trace(ray):
  # 1. Hit Detection
  hit_pos = ray.Hit(scene)
  if not hit_pos.exist:
    return 0

  # 2. Sample Ray Generation
  generated_rays = generate(ray, hit_pos)

  # 3. Sample Tracing (Recursion)
  for generated_ray in generated_rays:
    radiance_in[ray] = Trace(generated_ray)

  # 4. Integral Estimation
  radiance_out = scatter(radiance_in)

  return travel(radiance_out)
```

== Hit Detection

=== Bounding Volumn Hierarchy (BVH)

==== Top Level Acceleration Structure (TLAS)

==== Bottom Level Acceleration Structure (BLAS)

== Sample Ray Generation

The generation of sampling rays is done with the knowledge of the material of the hit position, as tracing them are *very expensive*.
Knowing the material, we can make much higher quality samples with certain probability distributions that reduce noise and make life easier for the `Scatter` function (the integral estimator).

== GL Transmission Format (glTF)

The glTF format is used as the accepted file format.
Inorder to perform the physics based rendering techniuqes from @physics, we need to use the material information provided by the file format to determine which scatter model or combination of models to use. In addition, we also need to translate the material information to physics metrics to perform precise calculations:

/ glTF Material Information:
#grid(
  columns: (1fr, 1fr, 1fr),
  list(
    `alpha mode`,
    `base color`,
    `emission`,
    `metallic`,
    `roughness`,
  ),
  list(
    `anisotropy`,
    `clearcoat`,
    `dispersion`,
    `ior`,
    `iridescence`,
  ),
  list(
    `sheen`,
    `specular`,
    `transmission`,
    `volume`,
  ),
)
source: https://www.khronos.org/gltf/pbr/

We have this diagram that shows the possible combinations of active properties and the model we use for them:

#table(
  columns: 2,
  align: left,
  table.header([Physics Behavior], [Activation & Control]),
  [Diffuse Reflection], [`base color`, `roughness`, `metallic`],
  [(Dielectric) Specular Reflection], [`roughness`, `ior`, `specular`],
  [Metal Reflection], [`metallic`, `base color`, `specular`],
  [Clearcoat], [`clearcoat`],
  [Sheen], [`sheen`],
  [Anisotropy], [`anisotropy`],
  [Iridescence], [`iridescence`],
  [Transmission], [`transmission`, `ior`, `roughness`],
  [Refraction], [`transmission`, `ior`, `volume`],
  [Volumn Absorption], [`volume`, `thickness`],
  [Dispersion], [`dispersion`],
  [Emission], [`emission`],
)

== Tracing


