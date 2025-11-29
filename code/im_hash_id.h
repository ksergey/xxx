// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cassert>
#include <string_view>
#include <tuple>
#include <utility>

#include "hash.h"
#include "im_stack.h"

namespace xxx {

enum class im_id : std::uint32_t {};

class im_hash_id {
private:
  im_stack<im_id> hash_id_stack_ = im_stack<im_id>(32);

public:
  im_hash_id() = default;

  void reset(std::uint32_t seed = 0) {
    hash_id_stack_.clear();
    hash_id_stack_.push_back(im_id(seed));
  }

  [[nodiscard]] auto make(std::string_view value) noexcept -> im_id {
    assert(!hash_id_stack_.empty());
    auto const result = xxx::hash(value, std::to_underlying(hash_id_stack_.back()));
    return im_id(result);
  }

  [[nodiscard]] auto make(int value) noexcept -> im_id {
    assert(!hash_id_stack_.empty());
    auto const result = xxx::hash(value, std::to_underlying(hash_id_stack_.back()));
    return im_id(result);
  }

  auto push_id(std::string_view value) -> im_id {
    return hash_id_stack_.emplace_back(make(value));
  }

  auto push_id(int value) -> im_id {
    return hash_id_stack_.emplace_back(make(value));
  }

  void pop_id() {
    if (hash_id_stack_.size() > 1) [[likely]] {
      hash_id_stack_.pop_back();
    }
  }

  // split string into content and string key
  // i.e.
  //   "hello##1234" -> ("hello", "1234")
  //   "world" -> ("world", "world")
  [[nodiscard]] static constexpr auto split_str_key(
      std::string_view str) -> std::tuple<std::string_view, std::string_view> {
    auto const found = str.rfind("##");
    if (found != str.npos) {
      return std::make_tuple(str.substr(0, found), str.substr(found));
    } else {
      return std::make_tuple(str, str);
    }
  }
};

} // namespace xxx
