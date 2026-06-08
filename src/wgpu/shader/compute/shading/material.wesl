import package::compute::physics;
import super::smits;

const GLTF_ALPHA_OPAQUE: u32 = 0u;
const GLTF_ALPHA_MASK: u32 = 1u;
const GLTF_ALPHA_BLEND: u32 = 2u;

const MATERIAL_FLAG_DOUBLE_SIDED: u32 = 1u;
const MATERIAL_FLAG_UNLIT: u32 = 2u;

fn material(
    wavelength: physics::Wavelength,
    material_idx: u32,
    uvs: array<vec2f, 2>
) -> Material {
    return decode(wavelength, materials[material_idx], uvs);
}

fn decode(
    wavelength: physics::Wavelength,
    packed_material: PackedMaterial,
    uvs: array<vec2f, 2>
) -> Material {
    let base_color: vec4f = baseColor(packed_material, uvs);
    let reflectance: f32 = rgbToWavelengthReflectance(
        base_color.rgb,
        wavelength
    );
    let roughness: f32 = roughness(packed_material, uvs);
    let metallic: f32 = metallic(packed_material, uvs);
    let transmission: f32 = transmission(packed_material, uvs);
    let raw_ior: f32 = packed_material.ior;
    let real_ior: f32 = select(
        raw_ior,
        dispersedIor(raw_ior, packed_material.dispersion, wavelength),
        metallic == 0 && transmission > 0
    );
    let ior: IOR = materialIor(
        real_ior,
        reflectance,
        metallic,
    );
    let is_transmissive = metallic == 0.0f && transmission > 0.0f;
    let raw_emission: vec3f = emission(packed_material, uvs);
    return Material(
        (1.0f - metallic) * (1.0f - transmission) * reflectance,
        roughness * roughness,
        ior,
        transmission,
        packed_material.thickness_factor > 0,
        rgbToWavelengthReflectance(raw_emission, wavelength)
    );
}

struct IOR {
    re: f32,
    im: f32,
};

struct Material {
    diffuse_reflectance: f32,
    microfacet_roughness: f32,
    ior: IOR,
    transmission: f32,
    volumn_transmission: bool,
    emission: f32,
};

fn materialIor(
    real_ior: f32,
    reflectance: f32,
    metallic: f32,
) -> IOR {
    return IOR(
        real_ior,
        metallic * imaginaryIorFromReflectance(real_ior, reflectance)
    );
}

fn dispersedIor(
    nd: f32,
    dispersion: f32,
    wavelength: physics::Wavelength
) -> f32 {
    if dispersion <= 0.0f {
        return nd;
    }

    // glTF stores dispersion = 20 / Vd, so 1/Vd = dispersion / 20.
    let inv_abbe = dispersion / 20.0f;

    return max(
        nd + (nd - 1.0f) * inv_abbe *
             (523655.0f / (wavelength * wavelength) - 1.5168f),
        1.0f
    );
}

fn imaginaryIorFromReflectance(
    real_ior: f32,
    channel_color: f32,
) -> f32 {
    return sqrt(
        clamp(
            (4 * real_ior) / (1 - channel_color) - pow(real_ior + 1, 2),
            0.0f,
            10.0f
        )
    );
}

struct MaterialTextureInfo {
    index: i32,
    tex_coord: u32,
    scale: f32,
    strength: f32,
};

struct PackedMaterial {
    base_color_factor: vec4f,
    emissive_factor: vec3f,
    emissive_strength: f32,
    metallic_factor: f32,
    roughness_factor: f32,
    alpha_cutoff: f32,
    alpha_mode: u32,
    ior: f32,
    dispersion: f32,
    transmission_factor: f32,
    thickness_factor: f32,
    attenuation_color: vec3f,
    attenuation_distance: f32,
    anisotropy_strength: f32,
    anisotropy_rotation: f32,
    clearcoat_factor: f32,
    clearcoat_roughness_factor: f32,
    iridescence_factor: f32,
    iridescence_ior: f32,
    iridescence_thickness_minimum: f32,
    iridescence_thickness_maximum: f32,
    sheen_color_factor: vec3f,
    sheen_roughness_factor: f32,
    specular_color_factor: vec3f,
    specular_factor: f32,
    flags: u32,
    _padding0: u32,
    _padding1: u32,
    _padding2: u32,
    base_color_texture: MaterialTextureInfo,
    metallic_roughness_texture: MaterialTextureInfo,
    normal_texture: MaterialTextureInfo,
    occlusion_texture: MaterialTextureInfo,
    emissive_texture: MaterialTextureInfo,
    anisotropy_texture: MaterialTextureInfo,
    clearcoat_texture: MaterialTextureInfo,
    clearcoat_roughness_texture: MaterialTextureInfo,
    clearcoat_normal_texture: MaterialTextureInfo,
    iridescence_texture: MaterialTextureInfo,
    iridescence_thickness_texture: MaterialTextureInfo,
    sheen_color_texture: MaterialTextureInfo,
    sheen_roughness_texture: MaterialTextureInfo,
    specular_texture: MaterialTextureInfo,
    specular_color_texture: MaterialTextureInfo,
    transmission_texture: MaterialTextureInfo,
    thickness_texture: MaterialTextureInfo,
};

@group(2) @binding(5) var<storage, read> materials: array<PackedMaterial>;

@group(2) @binding(6) var material_textures: texture_2d_array<f32>;
@group(2) @binding(7) var material_texture_sampler: sampler;

/* Getters for Material Info */
fn textureUv(uvs: array<vec2f, 2>, tex_coord: u32) -> vec2f {
    if tex_coord == 1u {
        return uvs[1];
    }
    return uvs[0];
}

fn baseColor(material: PackedMaterial, uvs: array<vec2f, 2>) -> vec4f {
    var color = material.base_color_factor;
    if material.base_color_texture.index >= 0 {
        color *= textureSampleLevel(
            material_textures,
            material_texture_sampler,
            textureUv(uvs, material.base_color_texture.tex_coord),
            material.base_color_texture.index,
            0.0f,
        );
    }
    return color;
}
fn roughness(material: PackedMaterial, uvs: array<vec2f, 2>) -> f32 {
    var roughness = material.roughness_factor;
    if material.metallic_roughness_texture.index >= 0 {
        roughness *= textureSampleLevel(
            material_textures,
            material_texture_sampler,
            textureUv(uvs, material.metallic_roughness_texture.tex_coord),
            material.metallic_roughness_texture.index,
            0.0f,
        )[1];
    }
    return clamp(roughness, 0.0f, 1.0f);
}

fn metallic(material: PackedMaterial, uvs: array<vec2f, 2>) -> f32 {
    var metallic = material.metallic_factor;
    if material.metallic_roughness_texture.index >= 0 {
        metallic *= textureSampleLevel(
            material_textures,
            material_texture_sampler,
            textureUv(uvs, material.metallic_roughness_texture.tex_coord),
            material.metallic_roughness_texture.index,
            0.0f,
        )[2];
    }
    return clamp(metallic, 0.0f, 1.0f);
}

fn transmission(material: PackedMaterial, uvs: array<vec2f, 2>) -> f32 {
    var transmission = material.transmission_factor;
    if material.transmission_texture.index >= 0 {
        transmission *= textureSampleLevel(
            material_textures,
            material_texture_sampler,
            textureUv(uvs, material.transmission_texture.tex_coord),
            material.transmission_texture.index,
            0.0f,
        ).r;
    }
    return clamp(transmission, 0.0f, 1.0f);
}

fn specular(material: PackedMaterial, uvs: array<vec2f, 2>) -> f32 {
    var strength = material.specular_factor;
    if material.specular_texture.index >= 0 {
        strength *= textureSampleLevel(
            material_textures,
            material_texture_sampler,
            textureUv(uvs, material.specular_texture.tex_coord),
            material.specular_texture.index,
            0.0f,
        ).a;
    }
    return clamp(strength, 0.0f, 1.0f);
}

fn emission(material: PackedMaterial, uvs: array<vec2f, 2>) -> vec3f {
    var strength = material.emissive_factor;
    if material.emissive_texture.index >= 0 {
        strength *= textureSampleLevel(
            material_textures,
            material_texture_sampler,
            textureUv(uvs, material.emissive_texture.tex_coord),
            material.emissive_texture.index,
            0.0f,
        ).rgb;
    }
    return max(strength * material.emissive_strength, vec3f(0));
}

fn rgbToWavelengthReflectance(rgb: vec3f, wavelength: physics::Wavelength) -> f32 {
    var reflectance: f32 = 0.0f;
    let r: f32 = rgb.r;
    let g: f32 = rgb.g;
    let b: f32 = rgb.b;
    if r <= g && g <= b {
        reflectance += r * smits::whiteReflectance(wavelength);
        if g <= b {
            reflectance += (g - r) * smits::cyanReflectance(wavelength);
            reflectance += (b - g) * smits::blueReflectance(wavelength);
        } else {
            reflectance += (b - r) * smits::cyanReflectance(wavelength);
            reflectance += (g - b) * smits::greenReflectance(wavelength);
        }
    } else if g <= r && g <= b {
        reflectance += g * smits::whiteReflectance(wavelength);
        if r <= b {
            reflectance += (r - g) * smits::magentaReflectance(wavelength);
            reflectance += (b - r) * smits::blueReflectance(wavelength);
        } else {
            reflectance += (b - g) * smits::magentaReflectance(wavelength);
            reflectance += (r - b) * smits::redReflectance(wavelength);
        }
    } else {
        reflectance += b * smits::whiteReflectance(wavelength);
        if r <= g {
            reflectance += (r - b) * smits::yellowReflectance(wavelength);
            reflectance += (g - r) * smits::greenReflectance(wavelength);
        } else {
            reflectance += (g - b) * smits::yellowReflectance(wavelength);
            reflectance += (r - g) * smits::redReflectance(wavelength);
        }
    }
    return clamp(0.94f * reflectance, 0.0f, 1.0f);
}
