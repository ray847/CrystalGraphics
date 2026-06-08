import package::compute::math;
import package::compute::physics;
import package::compute::shading::smits;

@group(2) @binding(8) var environment_texture: texture_2d<f32>;

fn radiance(dir: vec3f, wavelength: physics::Wavelength) -> physics::Radiance {
    let env_uv = Uv(normalize(dir));
    return rgbToWavelengthRadiance(Texel(env_uv).rgb, wavelength);
}

fn Texel(uv: vec2f) -> vec4f {
    let dims = textureDimensions(environment_texture);
    let wrapped_u = uv.x - floor(uv.x);
    let clamped_v = clamp(1.0f - uv.y, 0.0f, 0.999999f);
    let x = min(dims.x - 1u, u32(wrapped_u * f32(dims.x)));
    let y = min(dims.y - 1u, u32(clamped_v * f32(dims.y)));
    return textureLoad(environment_texture, vec2i(i32(x), i32(y)), 0);
}

fn Uv(dir: vec3f) -> vec2f {
    let u = atan2(dir.y, dir.x) / (2.0f * math::PI) + 0.5f;
    let v = acos(clamp(dir.z, -1.0f, 1.0f)) / math::PI;
    return vec2f(u, v);
}

fn rgbToWavelengthRadiance(rgb: vec3f, wavelength: physics::Wavelength) -> physics::Radiance {
    var radiance: f32 = 0.0f;
    let r: f32 = rgb.r;
    let g: f32 = rgb.g;
    let b: f32 = rgb.b;
    if r <= g && g <= b {
        radiance += r * smits::whiteReflectance(wavelength);
        if g <= b {
            radiance += (g - r) * smits::cyanReflectance(wavelength);
            radiance += (b - g) * smits::blueReflectance(wavelength);
        } else {
            radiance += (b - r) * smits::cyanReflectance(wavelength);
            radiance += (g - b) * smits::greenReflectance(wavelength);
        }
    } else if g <= r && g <= b {
        radiance += g * smits::whiteReflectance(wavelength);
        if r <= b {
            radiance += (r - g) * smits::magentaReflectance(wavelength);
            radiance += (b - r) * smits::blueReflectance(wavelength);
        } else {
            radiance += (b - g) * smits::magentaReflectance(wavelength);
            radiance += (r - b) * smits::redReflectance(wavelength);
        }
    } else {
        radiance += b * smits::whiteReflectance(wavelength);
        if r <= g {
            radiance += (r - b) * smits::yellowReflectance(wavelength);
            radiance += (g - r) * smits::greenReflectance(wavelength);
        } else {
            radiance += (g - b) * smits::yellowReflectance(wavelength);
            radiance += (r - g) * smits::redReflectance(wavelength);
        }
    }
    return physics::Radiance(max(radiance, 0.0f));
}
