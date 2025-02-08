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

#include <dlfcn.h>

#include <format>
#include <iostream>

#include <elfio/elfio.hpp>

#include "modules.hxx"
#include "symbols.hxx"

namespace Interject::Symbols {

static void
lookupInSymbolSection(const std::string file_name,
    const ELFIO::const_symbol_section_accessor &symbols, uintptr_t base_addr,
    std::span<const std::string_view> names, std::span<Symbols::Descriptor> descriptors) {

  const ELFIO::Elf_Xword symbol_count = symbols.get_symbols_num();
  for (ELFIO::Elf_Xword i = 0; i < symbol_count; i++) {
    std::string name;
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    ELFIO::Elf_Half section_index;
    unsigned char bind, type, other;
    if (!symbols.get_symbol(i, name, value, size, bind, type, section_index, other)) {
      std::cerr << "failed loading symbol" << std::endl;
      continue;
    }

    if (section_index == ELFIO::SHN_UNDEF || value == 0 || size == 0) {
      // skip undefined and empty symbols
      continue;
    }

    for (size_t idx = 0; idx < names.size(); idx++) {
      if (names[idx] == name) {
        auto &descriptor = descriptors[idx];
        descriptor.addr = base_addr + value;
        descriptor.size = size;
        // Add a reference to the loaded module to ensure it does not get unloaded once we've
        // returned the symbol address to the caller. The reference is released with dlclose in
        // the Descriptor destructor.
        descriptor.module_handle = ::dlopen(file_name.c_str(), RTLD_NOW);
        break;
      }
    }
  }
}

static void
lookupInELFFile(const std::string &file_name, const ELFIO::elfio &reader, uintptr_t base_addr,
    std::span<const std::string_view> names, std::span<Symbols::Descriptor> descriptors) {

  const auto section_count = reader.sections.size();
  for (ELFIO::Elf_Half i = 0; i < section_count; i++) {
    const ELFIO::section *section = reader.sections[i];
    if (section->get_type() != ELFIO::SHT_SYMTAB &&
        section->get_type() != ELFIO::SHT_DYNSYM) {
      continue;
    }

    const ELFIO::const_symbol_section_accessor symbols(reader, section);
    lookupInSymbolSection(file_name, symbols, base_addr, names, descriptors);
  }
}

void
lookup(std::span<const std::string_view> names, std::span<Descriptor> descriptors) {

  Modules::forEach([&](std::string_view obj_name, uintptr_t base_addr) {
    if (obj_name.find("vdso") != std::string_view::npos) {
      return; // ignore vdso since it is not backed by a file we can load/parse
    }

    std::string file_name(obj_name);
    ELFIO::elfio reader;
    if (!reader.load(file_name)) {
      std::cerr << std::format("failed to load {} as an ELF file", file_name) << std::endl;
      return;
    }

    lookupInELFFile(file_name, reader, base_addr, names, descriptors);
    });
}

Descriptor::~Descriptor() {
  if (this->module_handle) {
    ::dlclose(this->module_handle);
  }
}

};
