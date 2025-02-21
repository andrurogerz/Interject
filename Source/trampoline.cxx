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

#include "trampoline.hxx"

#include <cstring>
#include <sys/mman.h>

namespace Interject {

std::optional<Trampoline> Trampoline::create(Symbols::Descriptor symbol) {
  const std::size_t _origSize = symbol.size;
  const std::size_t _allocSize = symbol.size;

  void *mem = ::mmap(nullptr, _allocSize, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (mem == nullptr) {
    return std::nullopt;
  }

  // TODO: for now, we're not actually creating a trampoline. Instead we just
  // copy the entire function to a new location.
  std::memcpy(mem, reinterpret_cast<void *>(symbol.addr), _origSize);

  // Update the copy to be read-only and executable.
  if (::mprotect(mem, symbol.size, PROT_EXEC | PROT_READ) != 0) {
    ::munmap(mem, symbol.size);
    return std::nullopt;
  }

  return Trampoline(reinterpret_cast<std::uintptr_t>(mem), _allocSize,
                    _origSize);
}

Trampoline::~Trampoline() {
  if (_addr != 0 && _allocSize != 0) {
    ::munmap(reinterpret_cast<void *>(_addr), _allocSize);
  }
}

}; // namespace Interject
