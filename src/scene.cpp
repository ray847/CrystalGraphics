#include <filesystem>

#include <cgltf.h>

#include "CrystalGraphics/vertex.h"
#include "glm/trigonometric.hpp"
#include "CrystalGraphics/scene.h"

namespace crystal::graphics {

std::expected<Scene, Error> LoadScene(std::filesystem::path file) {
  Scene res;
  VertexContainer& verticies = res.vertices_;
  std::vector<uint32_t>& indices = res.indicies_;
  spatial::Space<Scene::SpaceDef>& space = res.space_;

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

  /* Extrace Meshes & Vertices. */
  std::vector<Mesh> meshes;
  meshes.resize(data->meshes_count);
  for (cgltf_size i = 0; i < data->meshes_count; ++i) {
    const cgltf_mesh& mesh = data->meshes[i];
    /* Record data starting point. */
    uint32_t current_vertex_offset = verticies.size();
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
      uint32_t primitive_vertex_offset = verticies.size();
      verticies.resize(verticies.size() + primitive_vertex_count);
      total_vertices_for_mesh += primitive_vertex_count;
      
      /* Vertex Data */
      for (cgltf_size k = 0; k < primitive.attributes_count; ++k) {
        const cgltf_attribute& attribute = primitive.attributes[k];
        cgltf_accessor* accessor = attribute.data;
        if (attribute.type == cgltf_attribute_type_position) {
          for (cgltf_size v = 0; v < accessor->count; ++v) {
            cgltf_accessor_read_float(
                accessor, v, &verticies[primitive_vertex_offset + v].position.x, 3);
          }
        } else if (attribute.type == cgltf_attribute_type_normal) {
          for (cgltf_size v = 0; v < accessor->count; ++v) {
            cgltf_accessor_read_float(
                accessor, v, &verticies[primitive_vertex_offset + v].normal.x, 3);
          }
        }
      }
      
      /* Indices */
      cgltf_accessor* indexAccessor = primitive.indices;
      if (indexAccessor) {
        total_indices_for_mesh += indexAccessor->count;
        for (cgltf_size k = 0; k < indexAccessor->count; ++k) {
          uint32_t local_index = cgltf_accessor_read_index(indexAccessor, k);
          indices.push_back(local_index + primitive_vertex_offset);
        }
      }
    }
    /* Save data. */
    meshes[i] = Mesh{ .vertex_offset = current_vertex_offset,
                      .vertex_count = total_vertices_for_mesh,
                      .index_offset = current_index_offset,
                      .index_count = total_indices_for_mesh };
  }

  /* Extract nodes. */
  auto root_ss = space.RootSubSpace();
  
  auto extract_node = [&](this auto&& self,
                          const cgltf_node* node,
                          decltype(root_ss) subspace) -> void {
    /* Extract transformation. */
    Scene::Trans trans;
    if (node->has_scale)
      trans.scale = { node->scale[0], node->scale[1], node->scale[2] };
    if (node->has_rotation)
      trans.rotation = {
        node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]
      };
    if (node->has_translation)
      trans.translate = { node->translation[0], node->translation[1], node->translation[2] };
      
    assert(!node->has_matrix && "Cannot extract transformation from matrix.");
    subspace.Trans() = trans;
    
    if (node->mesh) {
      uint32_t mesh_idx = node->mesh - data->meshes;
      /* Meshes are indexed directly from the space. */
      (void)subspace.CreateObj<Mesh>(meshes[mesh_idx]);
    }

    /* Recursion. */
    for (cgltf_size i = 0; i < node->children_count; ++i)
      self(node->children[i], subspace.CreateChild());
  };
  
  const cgltf_scene* scene = data->scene;
  for (cgltf_size i = 0; i < scene->nodes_count; ++i)
    extract_node(scene->nodes[i], root_ss.CreateChild());

  cgltf_free(data);

  /* Coordinate system translate. */
  space.RootSubSpace().Trans().IncrRotate(glm::degrees(-90.0f), { 1, 0, 0 });

  return res;
}

} // namespace crystal::graphics