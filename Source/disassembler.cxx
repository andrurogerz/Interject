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

#include "disassembler.hxx"
#include "scope_guard.hxx"

#include <format>
#include <iostream>

// Leverate the open source capstone disassemlber for the heavy lifting.
#include <capstone/capstone.h>

namespace Interject::Disassembler {

std::optional<std::span<const uint8_t>> copyInstrs(std::uintptr_t startAddr,
                                                   std::size_t maxCopySize,
                                                   std::size_t minCopySize) {
  ::csh csHandle;
#if defined(__x86_64__) || defined(_M_X64)
  if (::cs_open(CS_ARCH_X86, CS_MODE_64, &csHandle) != CS_ERR_OK) {
#elif defined(__aarch64__) || defined(_M_ARM64)
  if (::cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &csHandle) != CS_ERR_OK) {
#else
#error "only arm64 and x86_64 archtectures are supported"
#endif
    return std::nullopt;
  }
  const auto csHandleGuard =
      ScopeGuard::create([&]() { ::cs_close(&csHandle); });

  // Enable instruction details, which is required to quickly identify
  // instruction sequences containing instructions we cannot relocate
  // elsewhere such as relative branches.
  ::cs_option(csHandle, CS_OPT_DETAIL, CS_OPT_ON);

  // Allocate memory cache for 1 insnuction, to be used by cs_disasm_iter.
  ::cs_insn *insn = ::cs_malloc(csHandle);
  if (insn == nullptr) {
    return std::nullopt;
  }
  const auto csInstGuard = ScopeGuard::create([&]() { ::cs_free(insn, 1); });

  uint8_t *codeStart = reinterpret_cast<uint8_t *>(startAddr);
  const uint8_t *code = codeStart;
  size_t codeSize = maxCopySize;
  uint64_t addr = 0;
  size_t copySize = minCopySize;

  while (::cs_disasm_iter(csHandle, &code, &codeSize, &addr, insn)) {
    const auto detail = insn->detail;
    for (size_t idx = 0; idx < detail->groups_count; idx++) {
      if (detail->groups[idx] != CS_GRP_BRANCH_RELATIVE) {
        continue;
      }

      // TODO: this is a relative branch instruction. We need to copy all
      // instructions through the relative branch target address. For now, we
      // will just copy the entire function when we encounter this.
      copySize = maxCopySize;
    }
  }
  return std::move(std::span<const uint8_t>(codeStart, copySize));
}

}; // namespace Interject::Disassembler
