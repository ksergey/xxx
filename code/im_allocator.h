// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <bit>
#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>

namespace xxx {

// simple linear allocator
class im_allocator {
private:
  std::unique_ptr<std::byte[]> buffer_;
  void* begin_ = nullptr;
  void* end_ = nullptr;

public:
  im_allocator(im_allocator const&) = delete;
  im_allocator& operator=(im_allocator const&) = delete;

  im_allocator(im_allocator&& other) noexcept
      : buffer_(std::move(other.buffer_)), begin_(std::exchange(other.begin_, nullptr)),
        end_(std::exchange(other.end_, nullptr)) {}

  im_allocator& operator=(im_allocator&& other) noexcept {
    if (this != &other) {
      this->~im_allocator();
      new (this) im_allocator(std::move(other));
    }
    return *this;
  }

  im_allocator() = default;

  im_allocator(std::size_t capacity) {
    this->reserve(capacity);
  }

  ~im_allocator() = default;

  // invalidate allocations
  void reserve(std::size_t new_capacity) {
    auto const do_allocate = [&] {
      auto buffer = std::unique_ptr<std::byte[]>(new std::byte[new_capacity]);
      buffer_ = std::move(buffer);
      begin_ = buffer_.get();
      end_ = buffer_.get() + new_capacity;
    };

    if (begin_ == nullptr || end_ == nullptr) {
      return do_allocate();
    }

    auto const capacity = static_cast<std::size_t>(static_cast<std::byte*>(end_) - buffer_.get());
    if (new_capacity > capacity) {
      return do_allocate();
    }
  }

  void reset() noexcept {
    begin_ = buffer_.get();
  }

  template <typename T>
    requires std::is_trivially_destructible_v<T>
  [[nodiscard]] auto allocate(std::size_t count = 1) noexcept -> T* {
    return std::launder(reinterpret_cast<T*>(this->allocate<alignof(T)>(sizeof(T) * count)));
  }

  template <std::size_t Align>
    requires(std::popcount(Align) == 1)
  [[nodiscard]] auto allocate(std::size_t size) noexcept -> void* {
    auto space = std::size_t(static_cast<std::byte*>(end_) - buffer_.get());
    if (std::align(Align, size, begin_, space)) [[likely]] {
      auto const allocation_address = begin_;
      begin_ = static_cast<std::byte*>(begin_) + size;
      return allocation_address;
    }
    return nullptr;
  }
};

} // namespace xxx
