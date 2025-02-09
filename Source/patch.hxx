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

#include <array>
#include <cstdint>

namespace Interject::Patch {

#if defined(__x86_64__) || defined(_M_X64)

// x86_64 machine code for: jmp [target]
constexpr uint8_t JUMP_INSTRS[] = {
    0x48, 0xB8,                   // mov rax, target
    0,    0,    0, 0, 0, 0, 0, 0, // target address (filled below)
    0xFF, 0xE0                    // jmp rax
};
constexpr std::size_t JUMP_ADDR_BYTE_OFFSET = 2;

#elif defined(__aarch64__) || defined(_M_ARM64)

// AArch64 machine code for: mov x16, target; br x16
constexpr uint32_t JUMP_INSTRS[] = {
    // ldr reg, [pc, #8]  ; load address into x16
    0x58000000 | 16 | ((sizeof(std::uintptr_t) / 4) << 5),
    // br reg             ; branch to address in x16
    0xD61F0000 | (16 << 5), 0x00000000, 0x00000000 // target address
};
constexpr std::size_t JUMP_ADDR_BYTE_OFFSET = 8;

#else
#error "only arm64 and x86_64 archtectures are supported"
#endif

static inline constexpr size_t jumpToSize() { return sizeof(JUMP_INSTRS); }

static inline std::array<uint8_t, jumpToSize()>
createJumpTo(std::uintptr_t targetAddr) {
  std::array<uint8_t, sizeof(JUMP_INSTRS)> patch;
  std::memcpy(patch.data(), JUMP_INSTRS, sizeof(JUMP_INSTRS));
  std::memcpy(patch.data() + JUMP_ADDR_BYTE_OFFSET, &targetAddr,
              sizeof(targetAddr));
  return patch;
}

}; // namespace Interject::Patch
