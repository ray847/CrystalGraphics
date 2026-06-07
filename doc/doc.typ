#import "@preview/cetz:0.5.0"
#import "TypstTemplate/lib.typ": *
#import "@preview/plotsy-3d:0.2.1": plot-3d-parametric-surface

#show: formal

#set document(
  title: "Crystal Graphics Document",
  author: "Veyl",
  date: datetime.today(),
)

#title()

= Physics

== Light Transport <physics-light_transport>

This section talks about how *physics* explain the behavior of light transport on a *macro* level. Sometimes we will dive to micro levels to explain how certain patterns develop but really we only care about what we can percieve as behaviors on a macro level.

=== Surface Level

This section talks about what happens when light meets the surface of objects.
Notice the object surface we talk about here can be either surface of solids, or the surface of a soap bubble. So it can be more precisely discribed as positions with sudden material changes.

In totality, these macro properties effect what happens when light hits the surface:
- Index of Refraction (a.k.a. IOR)
- Microgeometry (Microgeometry doesn't technically effect how light is reflected / refracted on a micro level but its characteristcs determine the macro behavior of a surface.)

The micro behavior of surface light transport can be described with #link(<eq:snells-law>)[Snell's Law] & #link(<eq:fresnel-equations>)[Fresnel Equations]:

#figure(
  cetz.canvas({
    import cetz.draw: *
    line((-4, 0), (4, 0), name: "surface")
    line((0, 3), (0, -3), stroke: (dash: "dashed"))
    content((-3, 0.5), $n_1$)
    content((-3, -0.5), $n_2$)

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
    content("in.end", [Incoming], anchor: "south-west")
    content("reflect.end", [Reflect], anchor: "south-east")
    content("transmission.end", [Transmit], anchor: "west")
  }),
  caption: [Micro Surface Level Light Transport],
)<micro-surface-level-light-transport>

/ Snell's Law <eq:snells-law>:

$
  cases(
    n_1/ n_2 = sin(theta_i) / sin(theta_t),
    theta_i = theta_r,
  )
$

/ Fresnel Equations <eq:fresnel-equations>:

$
  cases(
    R_s = abs((n_1cos(theta_i) - n_2cos(theta_t)) / (n_1cos(theta_i) + n_2cos(theta_t)))^2\, T_s = 1 - R_s,
    R_p = abs((n_1cos(theta_t) - n_2cos(theta_i)) / (n_1cos(theta_t) + n_2cos(theta_i)))^2\, T_p = 1 - R_p,
  ), \
  "where" \
  R = E_i / E_r, T = E_i / E_t
$

For *unpolarized* light,

$
  R = (R_s + R_p) / 2
$

It should be noted that in the case of metals, ior($n$) can be complex, hence the magnitude operator $||$ in the equations.

/ Microgeometry<microgeometry>:

The above physics model is already enough for simulating light transport on a micro level or with perfectly smooth macro surfaces. But it fails to model situations where the surface is not perfectly smooth.

#secondary[
  By perfectly smooth macro surfaces, we mean the surface normal has *uniform continuity*:
  $
    forall epsilon > 0, exists delta > 0, (forall arrow(x)_0, arrow(x) in Omega and |arrow(x)_0 - arrow(x)| < delta, |arrow(n)_0 - arrow(n)| < epsilon), \
    "where" Omega: F(arrow(x)) = 0 "represent the surface"
  $
]

In such cases, reflection & refraction can be *percieved* as having a *distribution* of directions on the macro level.

#figure(
  cetz.canvas({
    import cetz.draw: *
    line((-4, 0), (4, 0), name: "surface")
    line((-1.2, 3), (-0.8, -3), stroke: (dash: "dashed"))
    line((0.3, 3), (-0.3, -3), stroke: (dash: "dashed"))
    line((0.9, 3), (1.1, -3), stroke: (dash: "dashed"))

    line((-1, 0), (-4, 2), mark: (start: "straight"), name: "in")
    line((0, 0), (-3, 2), mark: (start: "straight"), name: "in")
    line((1, 0), (-2, 2), mark: (start: "straight"), name: "in")

    line((-1, 0), (1, 2), mark: (end: "straight"), name: "reflect")
    line((0, 0), (0.75, 0.5), mark: (end: (symbol: "x", stroke: red)), name: "reflect")
    line((1, 0), (3, 2), mark: (end: "straight"), name: "reflect")

    line((-1, 0), (-0.7, -0.5), mark: (end: (symbol: "x", stroke: red)), name: "transmission")
    line((0, 0), (1, -2), mark: (end: "straight"), name: "transmission")
    line((1, 0), (2.5, -2), mark: (end: "straight"), name: "transmission")
  }),
  caption: [Rough Surface Microgeometry],
)<rough-surface-microgeometry>

The light transport behavior can be affected by the following phenomenons:

/ Normal Offsets:
#figure(
  cetz.canvas({
    import cetz.draw: *
    line((-4, 0), (4, 0), name: "surface")
    line((0, 4), (-0, -1), stroke: (dash: "dashed"))

    let r = 3
    for i in range(-9, 10, step: 1) {
      let x = i / 10 * r
      let y = calc.sqrt(r * r - x * x)
      line((0, 0), (x, y), mark: (end: "straight"))
    }
    content((0.2, r), $arrow(n)_mu$, anchor: "south-west")
  }),
  caption: [Normal Offsets],
)<normal-offsets>

/ Masking:
#figure(
  cetz.canvas({
    import cetz.draw: *
    line(
      (-4, 0),
      (0.5, 0),
      (1, 1),
      (2, 0),
      (4, 0),
      name: "surface",
    )
    line((0, 3), (0, -1), stroke: (dash: "dashed"))


    line((0, 0), (150deg, 4), mark: (start: "straight"), name: "in")
    line((0, 0), (30deg, 4), stroke: gray, mark: (end: "straight"), name: "reflect")
    line((0, 0), (30deg, 0.8), mark: (end: (symbol: "x", stroke: red)))
    set-style(content: (frame: "rect", stroke: none, fill: none, padding: .1))

    content("in.end", [Incoming], anchor: "south-west")
    content("reflect.end", [Reflect], anchor: "south-west")
  }),
)

=== Medium Level

This section talks about what happens when light travels through some medium.
For now we assume the properties of the medium doesn't change within a measurable volumn.

In the simplest scenario, when light travels through a vacume, nothing really happens.
However, the complexity scales substantially when scattering & absorption happens.

== Spectrum

The "percieved brighness", a.k.a. luminance, of a spectrum can be expressed with the following integral:

#let Luminance = $Y$
#let wavelength = $lambda$
$
  Luminance = integral_wavelength L(wavelength) V(wavelength) dif wavelength,
$
where function $V$ denotes the spectral response curve.

For humans, which have 3 types of #link("https://en.wikipedia.org/wiki/Photoreceptor_cell")[photoreceptor cells], there would be 3 response curves (except for the visually impaired).

#figure(
  image("asset/Cones_SMJ2_E.svg"),
  caption: [Human Spectral Response Curve],
)

= Before Implementation

#let wo = $arrow(w)_o$
#let wi = $arrow(w)_i$
#let norm = $arrow(n)$
#let angle(..args) = {
  let body = args.pos().join($,$)
  $ lr(chevron.l #body chevron.r) $
}
#let hemisphere = $upright(H)^2$
#let sphere = $upright(S)^2$
#let prob = $p$
#let pos = $p$
#let dir = $arrow(w)$
#let curr = $"curr"$
#let next = $"next"$

This section talks about mathematical and physics tricks used in the implementation. This section _should not_ contain any implementation related information but rather some techniques/approximations that makes computing possible.

/ The Rendering Equation:

$
  L(wo) =
  L_e (wo) + integral_wi L(wi) f(wo, wi) cos angle(wi, norm) dif wi
$ <eq:rendering-equation>

== Light Transport Model Behavior

Generally speaking, the outward radiance $L(wo)$ is a cumulative product of 2 *types* of inward radiance $L(wi)$:
- Continuous: Diffuse
- Discrete: Specular & Transmission

Although both types are well expressed in math with the same form (#link(<eq:rendering-equation>)[Rendering Equation]), we delliberately treat them differently since this form of arithmetic expression is not easily applicable in actual computation.

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

== Light Transport Models

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

=== Specular Reflection & Refraction

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

We can derive these functions from the #link(<eq:fresnel-equations>)[fresnel-equations]:
$
  cases(
    F_r (omega_i) = R_r / R_i = 1 / 2
    (
      ((eta cos theta_i - cos theta_t) / (eta cos theta_i + cos theta_t))^2
      +
      ((cos theta_i - eta cos theta_t) / (cos theta_i + eta cos theta_t))^2
    ),
    F_t (omega_i) = 1 - F_r,
  )
$

=== Rough Reflection & Refracion

The rough reflection model approximates the behavior of micro surface changes, a.k.a. microgeometry.
The model is usually represented in the form of a *normal distribution function* & a *masking function*:
$
  cases(
    D(norm_mu): integral_Omega D(norm_mu) dif norm_mu = prob[norm_mu in Omega] (Omega subset.eq hemisphere),
    G(omega_1, omega_2): cases(
      G(omega_1, omega_2) = g(omega_1) g(omega_2),
      integral_hemisphere D(norm_mu) g(omega) max{0, omega dot norm_mu} dif norm_mu = cos theta
    )
  ) \
  "where" norm_mu "denotes the micro surface normal."
$

#secondary[
  It should be noted that $prob(norm_mu in Omega)$ means the probability of some position $p$ on the _micro surface_ having a normal in the range of $Omega$.

  Conceptually, the masking function $G$ bridges the gap between the micro surface area and the macro surface area, owing reductions in the micro surface area to micro level blocking.
]

Therefore, such distributions must have the following properties:
$
  integral D(norm_mu) (norm dot norm_mu) dif norm_mu = 1
$
proof:
$
  |dif A| & = integral_(dif A) cos angle(norm, norm_mu) dif A_mu \
          & = integral_hemisphere cos angle(norm, norm_mu) |dif A(norm_mu)| dif norm_mu \
          & = integral_hemisphere cos angle(norm, norm_mu) D(norm_mu) |dif A| dif norm_mu \
          & = |dif A| integral_hemisphere D(norm_mu) (norm dot norm_mu) dif norm_mu \
       => & integral D(norm_mu) (norm dot norm_mu) dif norm_mu = 1
$

/ BRDF:
We then derive the complete reflection distribution function:
$
  f_r (omega_o, omega_i) = (D(norm_mu) G(omega_o, omega_i) F_r (omega_o, omega_i)) / (4 cos theta_i cos theta_o) \
  f_t (omega_o, omega_i) = abs(((omega_i dot norm_mu) (omega_o dot norm_mu)) / (cos theta_i cos theta_o))
  (D(norm_mu) F_t (omega_i) G(omega_o, omega_i)) / (omega_o dot norm_mu + (omega_i dot norm_mu) / eta)^2 \
$

/ PDF:
We can also combine the 2 equations to 1 *visible normal distribution* function.
This function is used to sample normals given the input angle so that the reflected ray is guaranteed to be visible:
$
  D(norm_mu | omega) = (D(norm_mu) g(omega) max{0, norm_mu dot omega}) / (cos theta)
$
Then the reflected & refracted PDF:
$
  p_r (w_o) = D(norm_mu | omega_i) (dif norm_mu) / (dif omega_o)
  = D(norm_mu | omega_i) / (4(omega_i dot norm_mu)) \
  p_t (w_o) = D(norm_mu | omega_i) (dif norm_mu) / (dif omega_o)
  = (D(norm_mu | omega_i) |omega_i dot norm_mu|) / (omega_o dot norm_mu + (omega_i dot norm_mu) / eta)^2 \
$

==== GGX Microfacet Model

The GGX (Ground Glass Unknown) model uses this equation as the *normal distribution function* (Trowbridge-Reitz):
$
  D(norm_mu) = 1 / (pi alpha_x alpha_y cos^4theta_mu (1 + tan^2 theta_mu ((cos^2 phi_mu)/alpha_x^2 + (sin^2 phi_mu)/alpha_y^2))^2)
$
/ Parameters:
- $alpha_x in [0, 1]$: roughness along the x axis
- $alpha_y in [0, 1]$: roughness along the y axis
- $theta_mu, phi_mu$: inward angle

As well as a *masking function* on the assumption that $G(omega, omega_mu) eq.triple G(omega)$ (Smith's approximation):
$
  integral_hemisphere D(norm_mu) g(omega, norm_mu) max{0, omega dot norm_mu} dif norm_mu = omega dot norm = cos theta \
  => g(omega) = (cos theta) / (integral_hemisphere D(norm_mu) max{0, omega dot norm_mu} dif norm_mu) = 1 / (1 + Lambda(omega)) \
$
where function $Lambda$ can be solved to be:
$
  Lambda(omega) = (sqrt(1 + alpha tan^2 theta) - 1) / 2, (alpha = sqrt(alpha_x^2 cos^2 phi + alpha_y^2sin^2 phi))
$

We use the approximation $G(omega_1, omega_2) = 1 / (1 + Lambda(omega_1) + Lambda(omega_2))$
rather than $G(omega_1, omega_2) = g(omega_1) g(omega_2)$ since the latter one underestimates and the prior one appears to be more accurate in practice.

=== Light Sampling

Since rays coming from light sources contribute greatly to a surface's received radiance despite the sparse solid angles they take up, it is of great benefit to deliberately sample rays that point to the light sources rather than relying on rays sampled from the transport model probability distributions.

In this project we implemented power light sampling, which samples rays that point to light sources based on the light's strength:

/ Power Light Sampling:

$
  prob(arrow(w)_i) = (P_i) / (sum P), \
  "where" P "denotes the power of light" i "."
$

Light sampling techniques only have a PDF since the BSDF used should come from the transporte models.

== Multiple Importance Sampling (MIS)

== Spectrum



We use the CIE XYZ color space as the source of truth for outputing RGB colors.

To be more specific, the conversion process from randiance from discretely sampled wavelengths to RGB look something like this:

#align(center, rect(
  inset: 15pt,
  grid(
    columns: 7,
    align: horizon,
    [RGB (Material)],
    sym.arrow.long,
    wavelength,
    stack("Monte Carlo", sym.arrow.long, "Estimator"),
    [XYZ],
    sym.arrow.long,
    [RGB],
  ),
))

== Summary

/ Radiance:

To sum up the above modeling, the complete rendering equation we simulate looks something like this:

$
  #let sampleModel = $scr(s)$
  #let transportModel = $scr(t)$
  L(dir_curr, pos)
  & = L_e (p) + integral_sphere L(dir_next, pos) f(dir_curr, dir_next) cos angle(dir_next, norm) dif dir_next \
  & = L_e (p) + sum^"MIS" integral_sphere L(dir_next, pos) f(dir_curr, dir_next) cos angle(dir_next, norm) dif dir_next \
  & approx L_e (p) + sum_sampleModel 1 / n_sampleModel sum_(dir_next) alpha_sampleModel (dir_next) (sum_transportModel f_transportModel (dir_next)) / (prob_sampleModel (dir_curr, dir_next)), \
  "where" & sampleModel in {"Sample Model"} and transportModel in {"Transport Model"}
$

/ Spectrum:

And the spectrum part:

$
  X = integral_wavelength overline(x)(wavelength)L(wavelength) dif wavelength \
  Y = integral_wavelength overline(y)(wavelength)L(wavelength) dif wavelength \
  Z = integral_wavelength overline(z)(wavelength)L(wavelength) dif wavelength \
$

where $overline(x), overline(y), overline(z)$ denote the color matching functions:

#figure(
  image(width: 60%, "asset/CIE_1931_XYZ_Color_Matching_Functions.svg"),
  caption: [CIE 1931 XYZ Color Matching Functions],
)<fig:CIE_1931_XYZ_Color_Matching_Functions>

Then calculate the RGB values from the XYZ color space:

$
  vec(R, G, B) =
  mat(
    2.36461385, -0.89654057, -0.46807328;
    -0.51516621, 1.4264081, 0.0887581;
    0.0052037, -0.01440816, 1.00920446
  ) vec(X, Y, Z)
$

#secondary[
  source: https://en.wikipedia.org/wiki/CIE_1931_color_space
]

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
Inorder to perform the physics based rendering techniuqes from @physics-light_transport, we need to use the material information provided by the file format to determine which scatter model or combination of models to use. In addition, we also need to translate the material information to physics metrics to perform precise calculations. It is helpful to think of the process into 2 steps:
#align(center, rect[glTF material #sym.arrow physics properties #sym.arrow reflection models])

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

We have this table that shows how physics properties derive from the material information:

#table(
  columns: (1fr, 2fr),
  table.header([Physics Property (Notation)], [Formula]),
  [Diffuse Reflectance ($R$)], $"Smits Reconstruction" dot (1 - #raw("metallic")) dot (1 - #raw("transmission"))$,

  [ior ($eta$)],
  $eta & = n + k i, \
  "where" & cases(
    n = cases(#raw("ior") "(w extension)", 1.5),
    k_i = #raw("metallic") dot sqrt((4n) / (1 - i / (r + b + g)) - (n + 1)^2)
  ),
  \
  & (i in {r, g, b} and vec(r, g, b) = #raw("base color"))$,

  [Microfacet Roughness ($alpha = alpha_x = alpha_y$)], $#raw("roughness")^2$,
)

#secondary[
  Since metal ior varies meaningfully based on the wavelength, we are actually interested in ior values on seperate rgb channels. The ior approximation is inferred from the assumption that:
  - A non-metallic ($#raw("metallic") = 0$) material should have the normal non-complex fresnel reflectance across all rgb channels:
  $
    F_(0, r) = F_(0, g) = F_(0, b) =
    F_0 |_(#raw("metallic") = 0) =
    ((eta - 1) / (eta + 1))^2
  $
  - A fully metallic ($#raw("metallic") = 1$) material should have the fresnel reflectance equal to the base color:
  $
    lr(vec(F_(0, r), F_(0, g), F_(0, b))|)_(#raw("metallic") = 1) = #raw("base color") \
    => F_(0, r) + F_(0, g) + F_(0, b) = 1.0
  $
  Therefore we can use this as the approximation:
  $
    #h(1fr) k_i & = sqrt((4n) / (1 - #raw("base color") _i) - (n + 1)^2), \
                & "and" \
            eta & = n + #raw("metallic") dot k i
  $
  Inference:$ F_(0, i)|_(#raw("metallic") = 1) =
  #raw("base color") _i     & = ((eta - 1) / (eta + 1))^2 \
      #raw("base color") _i & =((n + k_i i - 1) / (n + k_i i + 1))^2 = ((n - 1)^2 + k_i^2) / ((n + 1)^2 + k_i^2) \
  1 - #raw("base color") _i & = ((n + 1)^2 - (n - 1)^2) / ((n + 1)^2 + k_i^2) = (4n) / ((n + 1)^2 + k_i^2) \
                      k_i^2 & = (4n) / (1 - #raw("base color") _i) - (n + 1)^2 \ $
]

/ Smits Reconstruction<smits-reconstruction>:

Suppose ${c_1, c_2, c_3} in {r, g, b} and c_1 <= c_2 <= c_3$,

$
  "Smits"(wavelength) = c_1 S_"white"(wavelength) + (c_2 - c_1)S_(c_2 times c_3)(wavelength) + (c_3 - c_2)S_(c_3)(wavelength)
$
where $S_c$ denote the spectral curve for color $c$ and $c_2 times c_3$ denote the color you get by mixing $c_2$ and $c_3$.
The actual curve data is extracted from the `colour` python package.

== Spectrum Data Conversion

We use an approximation of the CIE color matching functions in @fig:CIE_1931_XYZ_Color_Matching_Functions for computation:

$
  overline(x)(wavelength) & = 0.362 G_s (wavelength; 442.0, 1 / 0.0624, 1 / 0.0374) \
                          & + 1.056 G_s (wavelength; 599.8, 1 / 0.0264, 1 / 0.0323) \
                          & - 0.065 G_s (wavelength; 501.1, 1 / 0.0490, 1 / 0.0382) \
  overline(y)(wavelength) & = 0.821 G_s (wavelength; 568.8, 1 / 0.0213, 1 / 0.0247) \
                          & + 0.286 G_s (wavelength; 530.9, 1 / 0.0613, 1 / 0.0322) \
  overline(z)(wavelength) & = 1.217 G_s (wavelength; 437.0, 1 / 0.0845, 1 / 0.0278) \
                          & + 0.681 G_s (wavelength; 459.0, 1 / 0.0385, 1 / 0.0725)
$
where
$
  G_s (x; mu, sigma_1, sigma_2) = cases(
    e^(- 1 / 2 ((x - mu) / sigma_1)^2)\; x < mu,
    e^(- 1 / 2 ((x - mu) / sigma_2)^2)\; x >= mu,
  )
$

#secondary[
  source: Simple Analytic Approximations
  to the CIE XYZ Color Matching Functions
]
