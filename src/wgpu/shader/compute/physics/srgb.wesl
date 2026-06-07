import super::luminance::Luminance;

alias SRGB = vec3f;

fn luminanceToSrgb(luminance: Luminance) -> SRGB {
    let xyz_to_srgb = mat3x3f(
        vec3f(2.36461385f, -0.51516621f, 0.0052037f),
        vec3f(-0.89654057f, 1.4264081f, -0.01440816f),
        vec3f(-0.46807328f, 0.0887581f, 1.00920446f)
    );
    let linear_srgb = xyz_to_srgb * luminance;
    return SRGB(vec3f(
        encodeChannel(linear_srgb.r),
        encodeChannel(linear_srgb.g),
        encodeChannel(linear_srgb.b)
    ));
}

fn encodeChannel(linear: f32) -> f32 {
    let value = max(linear, 0.0f);
    let encoded = select(
        1.055f * pow(value, 1.0f / 2.4f) - 0.055f,
        12.92f * value,
        value <= 0.0031308f
    );
    return clamp(encoded, 0.0f, 1.0f);
}
