// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>

namespace xxx {

template <typename T>
  requires std::is_trivially_copyable_v<T>
class im_stack {
private:
  std::unique_ptr<T[]> data_;
  std::size_t capacity_ = 0;
  std::size_t size_ = 0;

public:
  constexpr im_stack(im_stack const& other) : capacity_(other.capacity_), size_(other.size_) {
    data_ = std::make_unique_for_overwrite<T[]>(capacity_);
    std::copy_n(other.data(), other.size(), data_.get());
  }

  constexpr im_stack& operator=(im_stack const& other) {
    if (this != &other) {
      this->~im_stack();
      new (this) im_stack(other);
    }
    return *this;
  }

  constexpr im_stack(im_stack&& other) noexcept
      : data_(std::move(other.data_)), capacity_(std::exchange(other.capacity_, 0)),
        size_(std::exchange(other.size_, 0)) {}

  constexpr im_stack& operator=(im_stack&& other) noexcept {
    if (this != &other) {
      this->~im_stack();
      new (this) im_stack(std::move(other));
    }
    return *this;
  }

  constexpr im_stack() = default;

  constexpr im_stack(std::size_t capacity) : capacity_(capacity) {
    assert(capacity > 0);
    data_ = std::make_unique_for_overwrite<T[]>(capacity);
  }

  constexpr ~im_stack() = default;

  constexpr void resize(std::size_t new_size) {
    if (new_size > capacity_) {
      throw std::bad_alloc();
    }
    size_ = new_size;
  }

  constexpr void resize(std::size_t new_size, T const& value) {
    if (new_size > capacity_) {
      throw std::bad_alloc();
    }
    if (size_ < new_size) {
      std::uninitialized_fill_n(this->data() + size_, new_size - size_, value);
    }
    size_ = new_size;
  }

  [[nodiscard]] constexpr auto empty() const noexcept -> bool {
    return size_ == 0;
  }

  [[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
    return size_;
  }

  [[nodiscard]] constexpr auto capacity() const noexcept -> std::size_t {
    return capacity_;
  }

  constexpr void clear() {
    size_ = 0;
  }

  [[nodiscard]] constexpr auto operator[](std::size_t i) const noexcept -> T const& {
    assert(i < size_);
    return this->data()[i];
  }

  [[nodiscard]] constexpr auto operator[](std::size_t i) noexcept -> T& {
    assert(i < size_);
    return this->data()[i];
  }

  [[nodiscard]] constexpr auto data() const noexcept -> T const* {
    return data_.get();
  }

  [[nodiscard]] constexpr auto data() noexcept -> T* {
    return data_.get();
  }

  [[nodiscard]] constexpr auto begin() const noexcept -> T const* {
    return this->data();
  }

  [[nodiscard]] constexpr auto begin() noexcept -> T* {
    return this->data();
  }

  [[nodiscard]] constexpr auto end() const noexcept -> T const* {
    return this->data() + size_;
  }

  [[nodiscard]] constexpr auto end() noexcept -> T* {
    return this->data() + size_;
  }

  [[nodiscard]] constexpr auto front() const noexcept -> T const& {
    assert(size_ > 0);
    return this->data()[0];
  }

  [[nodiscard]] constexpr auto front() noexcept -> T& {
    assert(size_ > 0);
    return this->data()[0];
  }

  [[nodiscard]] constexpr auto back() const noexcept -> T const& {
    assert(size_ > 0);
    return this->data()[size_ - 1];
  }

  [[nodiscard]] constexpr auto back() noexcept -> T& {
    assert(size_ > 0);
    return this->data()[size_ - 1];
  }

  constexpr void push_back(T const& value) {
    if (size_ == capacity_) [[unlikely]] {
      throw std::bad_alloc();
    }
    this->data()[size_++] = value;
  }

  template <typename... ArgsT>
  constexpr auto emplace_back(ArgsT&&... args) -> T& {
    if (size_ == capacity_) [[unlikely]] {
      throw std::bad_alloc();
    }
    this->data()[size_++] = T(std::forward<ArgsT>(args)...);
    return this->back();
  }

  constexpr void pop_back() {
    assert(size_ > 0);
    size_--;
  }
};

} // namespace xxx
