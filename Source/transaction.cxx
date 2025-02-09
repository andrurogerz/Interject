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

#include "transaction.hxx"
#include "memory_map.hxx"
#include "symbols.hxx"

#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>

#include <sys/mman.h>
#include <unistd.h>

#include "patch.hxx"

namespace Interject {

std::size_t Transaction::_pageSize =
    static_cast<std::size_t>(sysconf(_SC_PAGESIZE));

Transaction::ResultCode Transaction::prepare() {
  if (_state != TxnInitialized) {
    return ErrorInvalidState;
  }

  std::vector<Symbols::Descriptor> descriptors(_names.size());
  Symbols::lookup(_names, descriptors);

  MemoryMap map;
  if (!map.load()) {
    return ErrorUnexpected;
  }

  std::unordered_map<uintptr_t, int> pagePermissions;
  for (size_t idx = 0; idx < _names.size(); idx++) {
    auto descriptor = descriptors[idx];

    if (Patch::jumpToSize() > descriptor.size) {
      std::cerr << std::format("function {} too small to patch", _names[idx])
                << std::endl;
      return ErrorFunctionBodyTooSmall;
    }

    const auto addr = descriptor.addr;
    if (addr == 0) {
      return ErrorSymbolNotFound;
    }

    const auto page_addr = addr & ~(_pageSize - 1);
    if (pagePermissions.find(page_addr) != pagePermissions.end()) {
      // This page is already present.
      continue;
    }

    const auto region = map.find(page_addr);
    if (!region) {
      return ErrorSymbolNotFound;
    }

    pagePermissions[page_addr] = region->permissions;

    // TODO: allocate trampoline and copy function preamble to trampoline
    // location
  }

  _state = TxnPrepared;
  _descriptors = std::move(descriptors);
  _pagePermissions = std::move(pagePermissions);
  return Success;
}

Transaction::ResultCode Transaction::commit() {
  // Prepare the first page of every hooked function to be writable.
  ResultCode result = Success;

  for (const auto &pair : _pagePermissions) {
    const int prot = pair.second | PROT_WRITE;
    if (::mprotect(reinterpret_cast<void *>(pair.first), _pageSize, prot) !=
        0) {
      std::cerr << "failed setting PROT_EXEC on page starting at 0x" << std::hex
                << pair.first << std::endl;
      result = ErrorMProtectFailure;
      goto exit;
    }
  }

  // TODO: pause all other threads and ensure none is executing the target
  // instructino sequence before patching.

  // Patch each function with jump to hook location.
  for (size_t idx = 0; idx < _descriptors.size(); idx++) {
    const auto &descriptor = _descriptors[idx];
    const auto targetAddr = reinterpret_cast<char *>(descriptor.addr);
    const uintptr_t hookAddr = _hooks[idx];

    auto instrBytes = Patch::createJumpTo(hookAddr);
    std::memcpy(targetAddr, &instrBytes, sizeof(instrBytes));

    // clear icache
    __builtin___clear_cache(targetAddr,targetAddr + sizeof(instrBytes));
  }

  result = Success;

exit:
  // Restore original memory protection.
  for (const auto &pair : _pagePermissions) {
    if (::mprotect(reinterpret_cast<void *>(pair.first), _pageSize,
                   pair.second) != 0) {
      std::cerr << "failed to restore permissions on page starting at 0x"
                << std::hex << pair.first << std::endl;
    }
  }

  return result;
}

Transaction::ResultCode Transaction::abort() {

  // TODO: restore original memory protection

  return Success;
}

}; // namespace Interject
