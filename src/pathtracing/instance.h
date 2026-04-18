#include <cstdint>

#include "CrystalGraphics/public.h"
#include "glm/ext/matrix_float4x4.hpp"


namespace crystal::graphics {

struct alignas(16) Instance {
  /**
   * From global space to local space.
   */
  alignas(16) glm::mat4 inv_trans;
  size32_t blas_root_idx;
  size32_t material_idx;
  uint64_t pad;
};

} // namespace crystal::graphics