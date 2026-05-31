#ifndef CRYSTALGRAPHICS_SRC_CACHE_H_
#define CRYSTALGRAPHICS_SRC_CACHE_H_

#include <optional>
#include <type_traits>

namespace crystal::graphics {

template <typename Data, typename Tagger>
class CacheTag : std::optional<std::invoke_result_t<Tagger, const Data&>> {
 public:
  using Tag = std::invoke_result_t<Tagger, const Data&>;
  /* Constructor */
  CacheTag(const Tagger& tagger = {}) : tagger_(tagger) {
  }
  CacheTag(const Data& data, const Tagger& tagger = {}) : tagger_(tagger) {
    *this = tagger(data);
  }
  /* Functions */
  bool Match(const Data& obj) const {
    return this->has_value() && this->value() == std::invoke(tagger_, obj);
  }
  void Write(const Data& obj) {
    this->emplace(std::invoke(tagger_, obj));
  }

 private:
  Tagger tagger_;
};

} // namespace crystal::graphics

#endif