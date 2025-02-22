/*
 * Copyright 2025 Andrew Rogers
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "symbols.hxx"

#include <cstdint>
#include <optional>
#include <span>

namespace Interject {

class Trampoline final {
public:
  static std::optional<Trampoline> create(Symbols::Descriptor symbol);

  Trampoline() : _addr(0), _allocSize(0), _origSize(0) {};
  ~Trampoline();

  Trampoline(const Trampoline &) = delete;
  Trampoline &operator=(const Trampoline &) = delete;

  Trampoline(Trampoline &&other) noexcept
      : _addr(other._addr), _allocSize(other._allocSize),
        _origSize(other._origSize) {
    other._addr = other._allocSize = other._origSize = 0;
  }

  Trampoline &operator=(Trampoline &&other) noexcept {
    if (this != &other) {
      _addr = other._addr;
      _allocSize = other._allocSize;
      _origSize = other._origSize;
      other._addr = other._allocSize = other._origSize = 0;
    }
    return *this;
  }

  std::uintptr_t start() const noexcept { return _addr; }

  std::span<const uint8_t> orig() const noexcept {
    return std::span<const uint8_t>(reinterpret_cast<uint8_t *>(_addr),
                                    _origSize);
  }

private:
  std::uintptr_t _addr;
  std::size_t _allocSize;
  std::size_t _origSize;

  Trampoline(std::uintptr_t _addr, std::size_t _allocSize,
             std::size_t _origSize) noexcept
      : _addr(_addr), _allocSize(_allocSize), _origSize(_origSize) {};
};

}; // namespace Interject
