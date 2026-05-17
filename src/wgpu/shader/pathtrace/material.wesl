const GLTF_ALPHA_OPAQUE: u32 = 0u;
const GLTF_ALPHA_MASK: u32 = 1u;
const GLTF_ALPHA_BLEND: u32 = 2u;

const MATERIAL_FLAG_DOUBLE_SIDED: u32 = 1u;
const MATERIAL_FLAG_UNLIT: u32 = 2u;

struct MaterialTextureInfo {
    index: i32,
    tex_coord: u32,
    scale: f32,
    strength: f32,
}

/* Getters for Material Info */
fn baseColor(material: Material, uv: vec2f) -> vec4f {
    var color = material.base_color_factor;
    if material.base_color_texture.index >= 0 {
        color *= textureSampleLevel(
            material_textures,
            material_texture_sampler,
            uv,
            material.base_color_texture.index,
            0.0f,
        );
    }
    return color;
}

struct Material {
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

@group(2) @binding(5) var<storage, read> materials: array<Material>;

@group(2) @binding(6) var material_textures: texture_2d_array<f32>;
@group(2) @binding(7) var material_texture_sampler: sampler;
