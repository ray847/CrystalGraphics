#include "CrystalGraphics/scene.h"

#include <cgltf.h>

#include <filesystem>
#include <glm/trigonometric.hpp>

#include "CrystalGraphics/public.h"
#include "scene_impl.h"

namespace crystal::graphics {

namespace {

Material DefaultMaterial() {
  return Material{
    .base_color = { 1.0f, 1.0f, 1.0f },
    .emission_strength = 0.0f,
    .emission_color = { 0.0f, 0.0f, 0.0f },
    .roughness = 1.0f,
    .metallic = 0.0f,
    .transmission = 0.0f,
    .ior = 1.5f,
    .flags = 0,
  };
}

Material LoadMaterial(const cgltf_material& material) {
  Material res = DefaultMaterial();

  const auto& pbr = material.pbr_metallic_roughness;
  res.base_color = {
    pbr.base_color_factor[0],
    pbr.base_color_factor[1],
    pbr.base_color_factor[2],
  };
  res.metallic = pbr.metallic_factor;
  res.roughness = pbr.roughness_factor;

  res.emission_color = {
    material.emissive_factor[0],
    material.emissive_factor[1],
    material.emissive_factor[2],
  };
  if (material.has_emissive_strength)
    res.emission_strength = material.emissive_strength.emissive_strength;

  if (material.has_transmission)
    res.transmission = material.transmission.transmission_factor;
  if (material.has_ior) res.ior = material.ior.ior;

  return res;
}

}  // namespace

Scene::Scene(const std::filesystem::path& filepath) :
    filepath_(filepath), impl_(std::make_unique<Impl>()) {
}

Scene::~Scene() = default;

Scene::Scene(Scene&& other) noexcept :
    filepath_(std::move(other.filepath_)), impl_(std::move(other.impl_)) {
}

std::expected<Scene, Error> LoadScene(std::filesystem::path file) {
  Scene res{ file };
  VertexContainer& vertices = res.impl_->vertices_;
  std::vector<uint32_t>& indices = res.impl_->indices_;
  std::vector<Material>& materials = res.impl_->materials_;
  spatial::Space<Scene::Impl::SpaceDef>& space = res.impl_->space_;

  /* Open the file. */
  cgltf_options opts{};
  cgltf_data* data = nullptr;
  std::string file_str = file.generic_string();
  if (cgltf_parse_file(&opts, file_str.c_str(), &data) != cgltf_result_success)
    return std::unexpected("cgltf parse file failed.");
  if (cgltf_load_buffers(&opts, data, file_str.c_str())
      != cgltf_result_success) {
    cgltf_free(data);
    return std::unexpected("cgltf load buffers failed.");
  }
  if (cgltf_validate(data) != cgltf_result_success) {
    cgltf_free(data);
    return std::unexpected("cgltf load buffers failed.");
  }

  /* Extract materials. */
  materials.reserve(data->materials_count + 1);
  materials.push_back(DefaultMaterial());
  for (cgltf_size i = 0; i < data->materials_count; ++i)
    materials.push_back(LoadMaterial(data->materials[i]));

  /* Extrace Meshes & Vertices. */
  std::vector<std::vector<Primitive>> primitives(data->meshes_count);
  for (cgltf_size i = 0; i < data->meshes_count; ++i) {
    const cgltf_mesh& mesh = data->meshes[i];
    /* Record data starting point. */
    uint32_t current_vertex_offset = vertices.size();
    uint32_t current_index_offset = indices.size();
    uint32_t total_vertices_for_mesh = 0;
    uint32_t total_indices_for_mesh = 0;

    for (cgltf_size j = 0; j < mesh.primitives_count; ++j) {
      const cgltf_primitive& primitive = mesh.primitives[j];
      assert(primitive.type == cgltf_primitive_type_triangles
             && "Unrecongnized primitive.");
      /* Vertex Count */
      uint32_t primitive_vertex_count = 0;
      for (cgltf_size k = 0; k < primitive.attributes_count; ++k) {
        if (primitive.attributes[k].type == cgltf_attribute_type_position) {
          primitive_vertex_count = primitive.attributes[k].data->count;
          break;
        }
      }
      uint32_t primitive_vertex_offset = vertices.size();
      vertices.resize(vertices.size() + primitive_vertex_count);
      total_vertices_for_mesh += primitive_vertex_count;

      /* Vertex Data */
      for (cgltf_size k = 0; k < primitive.attributes_count; ++k) {
        const cgltf_attribute& attribute = primitive.attributes[k];
        cgltf_accessor* accessor = attribute.data;
        if (attribute.type == cgltf_attribute_type_position) {
          for (cgltf_size v = 0; v < accessor->count; ++v) {
            cgltf_accessor_read_float(
                accessor,
                v,
                &vertices[primitive_vertex_offset + v].position.x,
                3);
          }
        } else if (attribute.type == cgltf_attribute_type_normal) {
          for (cgltf_size v = 0; v < accessor->count; ++v) {
            cgltf_accessor_read_float(
                accessor,
                v,
                &vertices[primitive_vertex_offset + v].normal.x,
                3);
          }
        }
      }

      /* Indices */
      cgltf_accessor* indexAccessor = primitive.indices;
      size32_t primitive_index_offset = indices.size();
      if (indexAccessor) {
        total_indices_for_mesh += indexAccessor->count;
        for (cgltf_size k = 0; k < indexAccessor->count; ++k) {
          uint32_t local_index = cgltf_accessor_read_index(indexAccessor, k);
          indices.push_back(local_index + primitive_vertex_offset);
        }
      }

      size32_t material_idx =
          primitive.material ?
              static_cast<size32_t>(
                  cgltf_material_index(data, primitive.material) + 1) :
              0;

      /* Primitives */
      primitives[i].push_back(
          Primitive{ .vertex_offset = primitive_vertex_offset,
                     .vertex_count = primitive_vertex_count,
                     .index_offset = primitive_index_offset,
                     .index_count = static_cast<size32_t>(
                         primitive.indices ? primitive.indices->count : 0),
                     .material_idx = material_idx });
    }
  }

  /* Extract nodes. */
  auto root_ss = space.RootSubSpace();

  auto extract_node = [&](this auto&& self,
                          const cgltf_node* node,
                          decltype(root_ss) subspace) -> void {
    /* Extract transformation. */
    Scene::Impl::Trans trans;
    if (node->has_scale)
      trans.scale = { node->scale[0], node->scale[1], node->scale[2] };
    if (node->has_rotation)
      trans.rotation = { node->rotation[3],
                         node->rotation[0],
                         node->rotation[1],
                         node->rotation[2] };
    if (node->has_translation)
      trans.translate = { node->translation[0],
                          node->translation[1],
                          node->translation[2] };

    assert(!node->has_matrix && "Cannot extract transformation from matrix.");
    subspace.Trans() = trans;

    if (node->mesh) {
      uint32_t mesh_idx = node->mesh - data->meshes;

      for (const auto& prim : primitives[mesh_idx]) {
        (void)subspace.CreateObj<Primitive>(prim);
      }
    }

    /* Recursion. */
    for (cgltf_size i = 0; i < node->children_count; ++i)
      self(node->children[i], subspace.CreateChild());
  };

  const cgltf_scene* scene = data->scene;
  for (cgltf_size i = 0; i < scene->nodes_count; ++i)
    extract_node(scene->nodes[i], root_ss.CreateChild());

  cgltf_free(data);

  /* Convert glTF's y-up coordinates to z-up. */
  space.RootSubSpace().Trans().IncrRotate(glm::radians(90.0f), { 1, 0, 0 });

  return res;
}

}  // namespace crystal::graphics
