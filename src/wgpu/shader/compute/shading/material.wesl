const GLTF_ALPHA_OPAQUE: u32 = 0u;
const GLTF_ALPHA_MASK: u32 = 1u;
const GLTF_ALPHA_BLEND: u32 = 2u;

const MATERIAL_FLAG_DOUBLE_SIDED: u32 = 1u;
const MATERIAL_FLAG_UNLIT: u32 = 2u;

fn material(material_idx: u32, uvs: array<vec2f, 2>) -> Material {
    return decode(materials[material_idx], uvs);
}

fn decode(packed_material: PackedMaterial, uvs: array<vec2f, 2>) -> Material {
    let base_color = baseColor(packed_material, uvs);
    let roughness = roughness(packed_material, uvs);
    let metallic = metallic(packed_material, uvs);
    let transmission = transmission(packed_material, uvs);
    let raw_ior = packed_material.ior;
    let ior: IOR = materialIor(raw_ior, base_color.rgb, metallic);
    let is_transmissive = metallic == 0.0f && transmission > 0.0f;
    return Material(
        (1.0f - metallic) * (1.0f - transmission) * base_color,
        roughness * roughness,
        ior,
        transmission,
        is_transmissive,
        false
    );
}

struct IORChannel {
    re: f32,
    im: f32,
};

struct IOR {
    r: IORChannel,
    g: IORChannel,
    b: IORChannel,
};

struct Material {
    diffuse_reflectance: vec4f,
    microfacet_roughness: f32,
    ior: IOR,
    transmission: f32,
    thin_transmission: bool,
    volumn_transmission: bool,
};

fn materialIor(real_ior: f32, base_color: vec3f, metallic: f32) -> IOR {
    return IOR(
        IORChannel(
            real_ior,
            metallic *
            imaginaryIorFromReflectance(real_ior, base_color.r, base_color)
        ),
        IORChannel(
            real_ior,
            metallic *
            imaginaryIorFromReflectance(real_ior, base_color.g, base_color)
        ),
        IORChannel(
            real_ior,
            metallic *
            imaginaryIorFromReflectance(real_ior, base_color.b, base_color)
        )
    );
}

fn imaginaryIorFromReflectance(
    real_ior: f32,
    channel_color: f32,
    base_color: vec3f
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
