#ifndef CRYSTALGRAPHICS_SRC_PATHTRACING_ALIAS_TABLE_H_
#define CRYSTALGRAPHICS_SRC_PATHTRACING_ALIAS_TABLE_H_

#include <algorithm>
#include <concepts>
#include <numeric>
#include <ranges>
#include <vector>

#include "CrystalGraphics/public.h"

namespace crystal::graphics {

struct alignas(8) AliasTableEntry {
  size32_t alias;
  float prob;
};

class AliasTable {
 public:
  AliasTable() = default;

  /* Constructor */
  template <std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, double>
  explicit AliasTable(Range&& weights) {
    std::vector<double> bined_probs;
    for (auto weight : weights)
      bined_probs.push_back(std::max(0.0, static_cast<double>(weight)));

    size32_t count = static_cast<size32_t>(bined_probs.size());
    entries_.resize(count);
    if (count == 0) return;

    double total = std::accumulate(bined_probs.begin(), bined_probs.end(), 0.0);
    if (total <= 0.0) return;

    std::vector<size32_t> large;
    std::vector<size32_t> small;
    large.reserve(count);
    small.reserve(count);
    for (size32_t i = 0; i < count; ++i) {
      bined_probs[i] = bined_probs[i] / total * count;
      if (bined_probs[i] >= 1.0) large.push_back(i);
      else small.push_back(i);
    }

    while (!large.empty() && !small.empty()) {
      size32_t i = small.back(), j = large.back();
      small.pop_back();
      large.pop_back();
      entries_[i].prob = bined_probs[i]; // < 1.0
      entries_[i].alias = j;
      bined_probs[j] -= 1.0 - entries_[i].prob;
      if (bined_probs[j] < 1.0) small.push_back(j);
      else large.push_back(j);
    }
    while (!large.empty()) {
      size32_t i = large.back();
      large.pop_back();
      entries_[i].prob = 1.0;
      entries_[i].alias = i;
    }
    while (!small.empty()) {
      size32_t i = small.back();
      small.pop_back();
      entries_[i].prob = 1.0;
      entries_[i].alias = i;
    }
  }

  const std::vector<AliasTableEntry>& Data() const {
    return entries_;
  }

 private:
  /* Fields */
  std::vector<AliasTableEntry> entries_;
};

static_assert(sizeof(AliasTableEntry) == 8);
static_assert(alignof(AliasTableEntry) == 8);

} // namespace crystal::graphics

#endif
