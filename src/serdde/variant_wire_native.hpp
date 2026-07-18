#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <variant>

template <typename Variant>
constexpr std::size_t serdde_variant_size() {
  return std::variant_size_v<Variant>;
}

template <typename Variant, typename Decoder, std::size_t Index = 0>
std::optional<Variant>
serdde_decode_variant(std::size_t selected, Decoder& decoder) {
  if constexpr (Index == std::variant_size_v<Variant>) {
    return std::nullopt;
  } else {
    if (selected == Index) {
      using Item = std::variant_alternative_t<Index, Variant>;
      auto decoded = decoder.template operator()<Item>();
      if (!decoded.has_value()) {
        return std::nullopt;
      }
      return Variant(std::in_place_index<Index>, std::move(decoded.value()));
    }
    return serdde_decode_variant<Variant, Decoder, Index + 1>(selected, decoder);
  }
}
