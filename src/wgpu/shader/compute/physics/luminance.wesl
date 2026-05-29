import super::radiance::Radiance;

alias Luminance = vec3f;

fn illuminate(radiance: Radiance) -> Luminance {
    let mapped = vec3f(1.0f) - exp(-max(radiance, vec3f(0.0f)));
    return Luminance(pow(mapped, vec3f(1.0f / 2.2f)));
}
